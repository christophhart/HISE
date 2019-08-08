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

namespace scriptnode
{
using namespace juce;
using namespace hise;


void JitNode::logMessage(const String& message)
{
	String s;

	s << getId() << ": " << message;



	debugToConsole(getProcessor(), s);
}

JitNode::JitNode(DspNetwork* parent, ValueTree d) :
	HiseDspNodeBase<core::jit>(parent, d)
{
	dynamic_cast<core::jit*>(getInternalT())->scope.addDebugHandler(this);

	parameterUpdater.setCallback(getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString()),
		{ PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(JitNode::updateParameters));
}

void JitNode::updateParameters(Identifier id, var newValue)
{
	auto* obj = dynamic_cast<core::jit*>(wrapper.getInternalT());

	Array<HiseDspBase::ParameterData> l;
	obj->createParameters(l);

	StringArray foundParameters;

	for (int i = 0; i < getNumParameters(); i++)
	{
		auto pId = getParameter(i)->getId();

		bool found = false;

		for (auto& p : l)
		{
			if (p.id == pId)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			auto v = getParameter(i)->data;
			v.getParent().removeChild(v, getUndoManager());
			removeParameter(i--);
		}

	}

	for (const auto& p : l)
	{
		foundParameters.add(p.id);

		if (auto param = getParameter(p.id))
		{
			param->setCallback(p.db);

			p.db(param->getValue());
		}
		else
		{
			auto newTree = p.createValueTree();
			getParameterTree().addChild(newTree, -1, nullptr);

			auto newP = new Parameter(this, newTree);
			newP->setCallback(p.db);
			addParameter(newP);
		}
	}
}

}

