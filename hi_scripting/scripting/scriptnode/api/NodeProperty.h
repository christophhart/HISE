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
	bool init(NodeBase* n, HiseDspBase* parent);

	/** Callback when the initialisation was successful. This might happen either during the initialise() method or after all parameters
		are created. Use this callback to setup the listeners / the logic that changes the property.
	*/
	virtual void postInit(NodeBase* n) = 0;

	/** Returns the ID in the ValueTree. */
	Identifier getValueTreePropertyId() const;

	NodeBase* pendingNode = nullptr;
	HiseDspBase* pendingParent = nullptr;

	ValueTree getPropertyTree() { return d; }

private:

	ValueTree d;

	Identifier valueTreePropertyid;
	Identifier baseId;
	var defaultValue;
	bool isPublic;
};

struct ScriptFunctionManager : public hise::GlobalScriptCompileListener,
	public NodeProperty
{
	ScriptFunctionManager() :
		NodeProperty(PropertyIds::Callback, "undefined", true) {};

	~ScriptFunctionManager();

	void scriptWasCompiled(JavascriptProcessor *processor) override;
	void updateFunction(Identifier, var newValue);
	double callWithDouble(double inputValue);

	void postInit(NodeBase* n) override;

	valuetree::PropertyListener functionListener;
	WeakReference<JavascriptProcessor> jp;
	Result lastResult = Result::ok();

	var functionName;
	var function;
	var input[8];
	bool ok = false;

	MainController* mc;
};

template <class T> struct NodePropertyT : public NodeProperty
{
	NodePropertyT(const Identifier& id, T defaultValue) :
		NodeProperty(id, defaultValue, false),
		value(defaultValue)
	{};

	void postInit(NodeBase* n) override
	{
		updater.setCallback(getPropertyTree(), { PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(NodePropertyT::update));
	}

	void update(Identifier id, var newValue)
	{
		value = newValue;

		if (additionalCallback)
			additionalCallback(id, newValue);
	}

	void setAdditionalCallback(const valuetree::PropertyListener::PropertyCallback& c)
	{
		additionalCallback = c;
	}

	T getValue() const { return value; }

private:

	valuetree::PropertyListener::PropertyCallback additionalCallback;

	T value;
	valuetree::PropertyListener updater;
};

}
