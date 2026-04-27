/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;


struct InvertableParameterRange
{
	InvertableParameterRange(double start, double end) :
		rng(start, end),
		inv(false)
	{};

	InvertableParameterRange() :
		rng(0.0, 1.0),
		inv(false)
	{};

	bool operator==(const InvertableParameterRange& other) const;

	InvertableParameterRange(const ValueTree& v);

	void store(ValueTree& v, UndoManager* um);

	InvertableParameterRange(double start, double end, double interval, double skew = 1.0) :
		rng(start, end, interval, skew),
		inv(false)
	{};

	double convertFrom0to1(double input, bool applyInversion) const;

	InvertableParameterRange inverted() const
	{
		auto copy = *this;
		copy.inv = !copy.inv;
		return copy;
	}

	InvertableParameterRange withCentreSkew(double c) const
	{
		auto copy = *this;
		copy.rng.setSkewForCentre(c);
		return copy;
	}

	double convertTo0to1(double input, bool applyInversion) const;

	Range<double> getRange() const
	{
		return rng.getRange();
	}

	double snapToLegalValue(double v) const
	{
		return rng.snapToLegalValue(v);
	}

	void setSkewForCentre(double value)
	{
		rng.setSkewForCentre(value);
	}

	void checkIfIdentity();

	juce::NormalisableRange<double> rng;
	bool inv = false;

	bool isNonDefault() const { return !isIdentity; }

private:

	bool isIdentity = false;
};

/** A base class for the scriptnode nodes that connect to a value tree and have a undo manager. */
struct ObjectWithValueTree
{
	virtual ~ObjectWithValueTree() = default;

	/** Override this method and return the value tree that represents this object. */
	virtual ValueTree getValueTree() const = 0;

	/** Override this method and return the undo manager associated to this object. */
	virtual UndoManager* getUndoManager() const = 0;

	virtual snex::Types::PrepareSpecs getLastPrepareSpecs() const = 0;
};

struct ParameterSourceObject: public ObjectWithValueTree
{
	/** Override this method and return the parameter value. This might be a modulated parameter or the value tree value. */
	virtual double getParameterValue(int index) const = 0;

	virtual InvertableParameterRange getParameterRange(int index) const = 0;
};

struct MultiOutputBase
{
	virtual ~MultiOutputBase() {};

	void initialiseOutputs(ObjectWithValueTree* o)
	{
		nodeData = o->getValueTree();
		jassert(nodeData.getType() == PropertyIds::Node);
	}

	int getNumOutputs() const
	{
		return nodeData.getChildWithName(PropertyIds::SwitchTargets).getNumChildren();
	}

	static Colour getFadeColour(int index, int numPaths)
	{
		if (numPaths == 0)
			return Colours::transparentBlack;

		auto hue = (float)index / (float)numPaths;

		auto saturation = JUCE_LIVE_CONSTANT_OFF(0.3f);
		auto brightness = JUCE_LIVE_CONSTANT_OFF(1.0f);
		auto minHue = JUCE_LIVE_CONSTANT_OFF(0.2f);
		auto maxHue = JUCE_LIVE_CONSTANT_OFF(0.8f);
		auto alpha = JUCE_LIVE_CONSTANT_OFF(0.4f);

		hue = jmap(hue, minHue, maxHue);

		return Colour::fromHSV(hue, saturation, brightness, alpha);
	}

	ValueTree getValueTree() { return nodeData; }

private:

	ValueTree nodeData;
};

/** A NodeProperty is a non-realtime controllable property of a node.

	It is saved as ValueTree property. The actual property ID in the ValueTree
	might contain the node ID as prefix if this node is part of a hardcoded node.
*/
struct NodeProperty
{
	NodeProperty(const Identifier& baseId_, const var& defaultValue_, bool isPublic_);;

	virtual ~NodeProperty();;

	/** Call this in the initialise() function of your node as well as in the createParameters() function (with nullptr as argument).

		This will automatically initialise the proper value tree ID at the best time.
	*/
	bool initialise(ObjectWithValueTree* o);

	/** Callback when the initialisation was successful. This might happen either during the initialise() method or after all parameters
		are created. Use this callback to setup the listeners / the logic that changes the property.
	*/
	virtual void postInit() = 0;

	/** Returns the ID in the ValueTree. */
	Identifier getValueTreePropertyId() const;

	ValueTree getPropertyTree() const;

	juce::Value asJuceValue();

private:

	UndoManager* um = nullptr;
	ValueTree d;
	Identifier valueTreePropertyid;
	Identifier baseId;
	var defaultValue;
	bool isPublic;
};

template <class T, int Value> struct StaticProperty
{
	constexpr T getValue() const { return Value; }

};

template <class T> struct NodePropertyT : public NodeProperty
{
	NodePropertyT(const Identifier& id, T defaultValue);;

	void postInit() override;

	void storeValue(const T& newValue, UndoManager* um);

	void update(Identifier id, var newValue);

	void setAdditionalCallback(const valuetree::PropertyListener::PropertyCallback& c, bool callWithValue=false);

	T getValue() const;

private:

	valuetree::PropertyListener::PropertyCallback additionalCallback;

	T value;
	valuetree::PropertyListener updater;
};


extern template struct NodePropertyT<int>;
extern template struct NodePropertyT<String>;
extern template struct NodePropertyT<bool>;

#if HISE_INCLUDE_SCRIPTNODE_UI
struct ComboBoxWithModeProperty : public ComboBox,
                                  public ComboBoxListener
{
	ComboBoxWithModeProperty(String defaultValue, const Identifier& id=PropertyIds::Mode);

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

	void valueTreeCallback(Identifier id, var newValue);

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void initModes(const StringArray& modes, ObjectWithValueTree* o);

	bool initialised = false;
	UndoManager* um;
	NodePropertyT<String> mode;
	ScriptnodeComboBoxLookAndFeel plaf;
    JUCE_DECLARE_WEAK_REFERENCEABLE(ComboBoxWithModeProperty);
};
#endif



}
