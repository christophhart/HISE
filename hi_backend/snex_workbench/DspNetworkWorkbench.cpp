/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise {
using namespace juce;

snex::ui::WorkbenchData::CompileResult DspNetworkCompileHandler::compile(const String& codeToCompile)
{
	WorkbenchData::CompileResult r;

	using namespace scriptnode;

	if (auto dcg = dynamic_cast<DspNetworkCodeProvider*>(getParent()->getCodeProvider()))
	{
		if (dcg->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
		{
			auto rootNode = np->getActiveNetwork()->getRootNode();

			np->getActiveNetwork()->setExternalDataHolder(&getParent()->getTestData());

			auto pList = rootNode->getValueTree().getChildWithName(PropertyIds::Parameters);
			snex::cppgen::ParameterEncoder pe(pList);

			for (auto& i : pe)
			{
				WorkbenchData::CompileResult::DynamicParameterData d;
				d.data = i;

				auto scriptnodeParameter = rootNode->getParameter(d.data.index);

				if (scriptnodeParameter != nullptr)
				{
					d.f = [scriptnodeParameter, this](double v)
					{
						scriptnodeParameter->setValueAndStoreAsync(v);
					};
				}

				r.parameters.add(d);
			}
		}
		else
		{
			auto instanceId = dcg->getInstanceId();

			if (instanceId.isValid())
			{
				Compiler::Ptr cc = createCompiler();

				auto numChannels = getParent()->getTestData().testSourceData.getNumChannels();

				if (numChannels == 0)
					numChannels = 2;

				r.lastNode = new JitCompiledNode(*cc, codeToCompile, instanceId.toString(), numChannels);

				r.assembly = cc->getAssemblyCode();
				r.compileResult = r.lastNode->r;
				r.obj = r.lastNode->getJitObject();

				for (const auto& i : r.lastNode->getParameterList())
				{
					WorkbenchData::CompileResult::DynamicParameterData d;
					d.data = i.data;
					d.f = (void(*)(double))i.function;
					r.parameters.add(d);
				}

				return r;
			}

			r.compileResult = Result::fail("Didn't specify file");
		}
	}

	return r;
}

DspNetworkCodeProvider::DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, const File& fileToWriteTo) :
	CodeProvider(d),
	DspNetworkSubBase(d, mc),
	connectedFile(fileToWriteTo)
{
	//setMillisecondsBetweenUpdate(600);

	setForwardCallback(valuetree::AnyListener::PropertyChange, false);

	initRoot();

	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(getXmlFile()))
	{
		currentTree = ValueTree::fromXml(*xml);
		np->getOrCreate(currentTree);
	}
	else
	{
		np->getOrCreate(getXmlFile().getFileNameWithoutExtension());

		np->prepareToPlay(np->getSampleRate(), np->getLargestBlockSize());
		currentTree = np->getActiveNetwork()->getValueTree();
	}

	
		

	source = SourceMode::InterpretedNode;
	setRootValueTree(currentTree);
}

}
