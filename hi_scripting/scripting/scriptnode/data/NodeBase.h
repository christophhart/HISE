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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{

using namespace juce;
using namespace hise;

class DspNetwork;
class NodeComponent;

struct NodeBase : public ConstScriptingObject
{
	using Ptr = WeakReference<NodeBase>;
	using List = Array<WeakReference<NodeBase>>;

	struct Parameter: public ConstScriptingObject
	{
		Identifier getObjectName() const override { return PropertyIds::Parameter; }

		Parameter(NodeBase* parent_, ValueTree& data_);;

		void setValueAndStoreAsync(double newValue);
		void addModulationValue(double newValue);
		void multiplyModulationValue(double newValue);


		void addConnectionTo(var dragDetails);

		String getId() const;
		double getValue() const;
		DspHelpers::ParameterCallback& getReferenceToCallback();
		DspHelpers::ParameterCallback getCallback() const;
		void setCallback(const DspHelpers::ParameterCallback& newCallback);
		double getModValue() const;

		StringArray valueNames;
		NodeBase::Ptr parent;
		ValueTree data;

	private:

		static void nothing(double) {};
		void storeValue();

		valuetree::PropertyListener opTypeListener;
		DspHelpers::ParameterCallback db;
		LockFreeUpdater valueUpdater;

		double modAddValue = 0.0;
		double modMulValue = 1.0;
		double currentDoublePrecisionValue = 0.0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Parameter);
	};

	NodeBase(DspNetwork* rootNetwork, ValueTree data, int numConstants);;
	virtual ~NodeBase() {}

	/** Overwrite this method and implement the processing. */
	virtual void process(ProcessData& data) = 0;

	/** This will be called before calls to process. */
	virtual void prepare(double sampleRate, int blockSize) = 0;

	/** The default implementation calls process with a sample amount of 1, but you can override
	    this method if there's a faster way. */
	virtual void processSingle(float* frameData, int numChannels);

	/** Override this to reset your node. */
	virtual void reset() = 0;

	/** Override this method and create a Cpp class. */
	virtual String createCppClass(bool isOuterClass);

	/** Override this method and return a custom component. */
	virtual NodeComponent* createComponent();

	/** Override this method and return the bounds in the canvas for the topLeft position. */
	virtual Rectangle<int> getPositionInCanvas(Point<int> topLeft) const;
	
	void setBypassed(bool shouldBeBypassed);
	bool isBypassed() const noexcept;

	bool isConnected() const { return data.getParent().isValid(); }
	void setProperty(const Identifier& id, const var value);
	void setDefaultValue(const Identifier& id, var newValue);
	
	void addConnectionToBypass(var dragDetails);

	DspNetwork* getRootNetwork() const;
	ValueTree getParameterTree() { return data.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager()); }
	
	NodeBase* getParentNode() const;
	ValueTree getValueTree() const;
	String getId() const;
	UndoManager* getUndoManager();

	Rectangle<int> reduceHeightIfFolded(Rectangle<int> originalHeight) const;

	void setNumChannels(int newNumChannels)
	{
		if (!data[PropertyIds::LockNumChannels])
			setProperty(PropertyIds::NumChannels, newNumChannels);
	}

	bool hasFixChannelAmount() const;

	int getNumChannelsToProcess() const { return (int)data[PropertyIds::NumChannels]; };

	int getNumParameters() const;;
	Parameter* getParameter(const String& id) const;
	Parameter* getParameter(int index) const;
	void addParameter(Parameter* p);
	void removeParameter(int index);

protected:

	ValueTree data;

private:

	CachedValue<bool> bypassed;
	bool pendingBypassState;
	LockFreeUpdater bypassUpdater;

	ReferenceCountedArray<Parameter> parameters;

	WeakReference<ConstScriptingObject> parent;
	WeakReference<NodeBase> parentNode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeBase);
};


}
