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


/** A NodeProperty is a non-realtime controllable property of a node.

	It is saved as ValueTree property. The actual property ID in the ValueTree
	might contain the node ID as prefix if this node is part of a hardcoded node.
*/
struct NodeProperty
{
	NodeProperty(const Identifier& baseId_, const var& defaultValue_, bool isPublic_) :
		baseId(baseId_),
		defaultValue(defaultValue_),
		isPublic(isPublic_)
	{};

	virtual ~NodeProperty() {};

	/** Call this in the initialise() function of your node as well as in the createParameters() function (with nullptr as argument).

		This will automatically initialise the proper value tree ID at the best time.
	*/
	bool initialise(NodeBase* n);

	/** Callback when the initialisation was successful. This might happen either during the initialise() method or after all parameters
		are created. Use this callback to setup the listeners / the logic that changes the property.
	*/
	virtual void postInit(NodeBase* n) = 0;

	/** Returns the ID in the ValueTree. */
	Identifier getValueTreePropertyId() const;

	ValueTree getPropertyTree() const { return d; }

	juce::Value asJuceValue()
	{
		return d.getPropertyAsValue(PropertyIds::Value, um);
	}

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
	NodePropertyT(const Identifier& id, T defaultValue) :
		NodeProperty(id, defaultValue, false),
		value(defaultValue)
	{};

	void postInit(NodeBase* ) override
	{
		updater.setCallback(getPropertyTree(), { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(NodePropertyT::update));
	}

	void storeValue(const T& newValue, UndoManager* um)
	{
		if(getPropertyTree().isValid())
			getPropertyTree().setPropertyExcludingListener(&updater, PropertyIds::Value, newValue, um);

		value = newValue;
	}

	void update(Identifier id, var newValue)
	{
		value = newValue;

		if (additionalCallback)
			additionalCallback(id, newValue);
	}

	void setAdditionalCallback(const valuetree::PropertyListener::PropertyCallback& c, bool callWithValue=false)
	{
		additionalCallback = c;

		if (callWithValue && additionalCallback)
			additionalCallback(PropertyIds::Value, var(value));
	}

	T getValue() const { return value; }

private:

	valuetree::PropertyListener::PropertyCallback additionalCallback;

	T value;
	valuetree::PropertyListener updater;
};


struct ComboBoxWithModeProperty : public ComboBox,
	public ComboBoxListener
{
	ComboBoxWithModeProperty(String defaultValue, const Identifier& id=PropertyIds::Mode) :
		ComboBox(),
		mode(id, defaultValue)
	{
		addListener(this);
		setLookAndFeel(&plaf);
		setColour(ColourIds::textColourId, Colour(0xFFAAAAAA));
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		if (initialised)
			mode.storeValue(getText(), um);
	}

	void valueTreeCallback(Identifier id, var newValue)
	{
        SafeAsyncCall::call<ComboBoxWithModeProperty>(*this, [newValue](ComboBoxWithModeProperty& c)
        {
            c.setText(newValue.toString(), dontSendNotification);
        });
	}

	void initModes(const StringArray& modes, NodeBase* n)
	{
		if (initialised)
			return;

		clear(dontSendNotification);
		addItemList(modes, 1);

		um = n->getUndoManager();
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(ComboBoxWithModeProperty::valueTreeCallback), true);
		initialised = true;
	}

	bool initialised = false;
	UndoManager* um;
	NodePropertyT<String> mode;
	ScriptnodeComboBoxLookAndFeel plaf;
    JUCE_DECLARE_WEAK_REFERENCEABLE(ComboBoxWithModeProperty);
};


}
