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

namespace scriptnode {
using namespace juce;
using namespace hise;


ScriptFunctionManager::~ScriptFunctionManager()
{
	if (mc != nullptr)
		mc->removeScriptListener(this);
}

bool NodeProperty::init(NodeBase* n, HiseDspBase* parent)
{
	if (n == nullptr && pendingNode == nullptr)
		return false;

	if (n == nullptr)
		n = pendingNode;

	if (parent == nullptr)
		parent = pendingParent;


	valueTreePropertyid = baseId;

	auto propTree = n->getPropertyTree();

	d = propTree.getChildWithProperty(PropertyIds::ID, getValueTreePropertyId().toString());

	if (!d.isValid())
	{
		d = ValueTree(PropertyIds::Property);
		d.setProperty(PropertyIds::ID, getValueTreePropertyId().toString(), nullptr);
		d.setProperty(PropertyIds::Value, defaultValue, nullptr);
		d.setProperty(PropertyIds::Public, isPublic, nullptr);
		propTree.addChild(d, -1, n->getUndoManager());
	}

	postInit(n);

	return true;
}

juce::Identifier NodeProperty::getValueTreePropertyId() const
{
	return valueTreePropertyid;
}

void ScriptFunctionManager::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (jp == processor)
	{
		updateFunction(PropertyIds::Callback, functionName);
	}
}

void ScriptFunctionManager::updateFunction(Identifier, var newValue)
{
	if (jp != nullptr)
	{
		auto functionId = Identifier(newValue.toString());

		functionName = newValue;

		function = jp->getScriptEngine()->getInlineFunction(functionId);
		ok = function.getObject() != nullptr;
	}
	else
		ok = false;
}

double ScriptFunctionManager::callWithDouble(double inputValue)
{
	if (ok)
	{
		input[0] = inputValue;
		return (double)jp->getScriptEngine()->executeInlineFunction(function, input, &lastResult, 1);
	}

	return inputValue;
}

void ScriptFunctionManager::postInit(NodeBase* n)
{
	mc = n->getScriptProcessor()->getMainController_();
	mc->addScriptListener(this);

	jp = dynamic_cast<JavascriptProcessor*>(n->getScriptProcessor());

	functionListener.setCallback(getPropertyTree(), { PropertyIds::Value },
		valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(ScriptFunctionManager::updateFunction));
}



}