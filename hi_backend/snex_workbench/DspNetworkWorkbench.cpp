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
	lastResult = {};

	using namespace scriptnode;

	

	if (auto dcg = dynamic_cast<DspNetworkCodeProvider*>(getParent()->getCodeProvider()))
	{
		dllNode.callDestructor();

		auto mode = dcg->source;

		if (mode == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
		{
			bool found = false;

			if (auto fh = ProcessorHelpers::getFirstProcessorWithType<DspNetwork::Holder>(getMainController()->getMainSynthChain()))
			{
				if (auto d = fh->projectDll)
				{
					dll::HostFactory f(fh->projectDll);

					auto id = dcg->getInstanceId();

					for (int i = 0; i < f.getNumNodes(); i++)
					{
						if (id.toString() == f.getId(i))
						{
							found = f.initOpaqueNode(&dllNode, i);
							break;
						}
					}
				}
			}

			if (found)
			{
				dllNode.createParameters(lastResult.parameters);
			}
			else
				lastResult.compileResult = Result::fail("Can't find node in dll");

			return lastResult;
		}

		if (dcg->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
		{
			if (np != nullptr)
			{

				auto rootNode = np->getActiveNetwork()->getRootNode();

				np->getActiveNetwork()->setExternalDataHolder(&getParent()->getTestData());

				for (int i = 0; i < rootNode->getNumParameters(); i++)
				{
					auto p = rootNode->getParameter(i);

					scriptnode::parameter::data d;

					auto f = [](void* obj, double value)
					{
						auto typed = static_cast<scriptnode::NodeBase::Parameter*>(obj);
						typed->setValueAndStoreAsync(value);
					};

					d.info = scriptnode::parameter::pod(p->data);

					d.callback.referTo(p, f);
					lastResult.parameters.add(d);
				}
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

				lastResult.lastNode = new JitCompiledNode(*cc, codeToCompile, instanceId.toString(), numChannels);
				lastResult.assembly = cc->getAssemblyCode();
				lastResult.compileResult = lastResult.lastNode->r;
				lastResult.obj = lastResult.lastNode->getJitObject();
				lastResult.parameters.addArray(lastResult.lastNode->getParameterList());

				return lastResult;
			}

			lastResult.compileResult = Result::fail("Didn't specify file");
		}
	}

	return lastResult;
}



void DspNetworkCompileHandler::processTestParameterEvent(int parameterIndex, double value)
{
#if 0
	if (dllNode.getObjectPtr() != nullptr)
	{
		dllNode.parameterFunctions[parameterIndex](dllNode.getObjectPtr(), value);
	}
#endif

	if (interpreter != nullptr)
	{
		if (auto p = interpreter->getRootNode()->getParameter(parameterIndex))
			p->setValueAndStoreAsync(value);
	}

	if (isPositiveAndBelow(parameterIndex, lastResult.parameters.size()))
		lastResult.parameters.getReference(parameterIndex).callback.call(value);
}

void DspNetworkCompileHandler::initExternalData(ExternalDataHolder* h)
{
	if (dllNode.getObjectPtr() != nullptr)
	{
		dllNode.initExternalData(h);
	}
	if (jitNode != nullptr)
	{
		jitNode->setExternalDataHolder(h);
	}

}

void DspNetworkCompileHandler::prepareTest(PrepareSpecs ps, const Array<WorkbenchData::TestData::ParameterEvent>& initialParameters)
{
	if (dllNode.getObjectPtr() != nullptr)
		dllNode.prepare(ps);
	else if (interpreter != nullptr)
		interpreter->prepareToPlay(ps.sampleRate, ps.blockSize);
	else if (jitNode != nullptr)
		jitNode->prepare(ps);

	for (auto& p : initialParameters)
	{
		if (p.timeStamp == -1)
			processTestParameterEvent(p.parameterIndex, p.valueToUse);
	}

	if (dllNode.getObjectPtr() != nullptr)
		dllNode.reset();
	else if (interpreter != nullptr)
		interpreter->getRootNode()->reset();
	else if (jitNode != nullptr)
		jitNode->reset();
}

void DspNetworkCompileHandler::processTest(ProcessDataDyn& data)
{
	if (dllNode.getObjectPtr() != nullptr)
		dllNode.process(data);
	else if (interpreter != nullptr)
	{
		ScopedLock sl(interpreter->getConnectionLock());
		interpreter->getRootNode()->process(data);
	}
	else if (jitNode != nullptr)
		jitNode->process(data);
}

void DspNetworkCompileHandler::postCompile(WorkbenchData::CompileResult& lastResult)
{
	if (lastResult.compiledOk())
	{
		auto& testData = getParent()->getTestData();

		if(auto ct = testData.getCustomTest())
		{
			ct->triggerTest(lastResult);
			return;
		}

		if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(getParent()->getCodeProvider()))
		{
			if (dnp->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
			{
				jitNode = nullptr;

				if (holder != nullptr)
					interpreter = holder->getActiveNetwork();
			}
			else if (dnp->source == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
			{
				interpreter = nullptr;
				jitNode = nullptr;
			}
			else
			{
				interpreter = nullptr;
				jitNode = lastResult.lastNode;
			}
		}

		if (testData.shouldRunTest())
		{
			testData.initProcessing(getMainController()->getMainSynthChain()->getLargestBlockSize(), getMainController()->getMainSynthChain()->getSampleRate());
			testData.processTestData(getParent());
		}
	}
}

DspNetworkCodeProvider::DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, const File& fileToWriteTo) :
	CodeProvider(d),
	DspNetworkSubBase(d, mc),
	connectedFile(fileToWriteTo)
{
	//setMillisecondsBetweenUpdate(600);

	setForwardCallback(valuetree::AnyListener::PropertyChange, false);

	initRoot();
}

void DspNetworkCodeProvider::initNetwork()
{
	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(getXmlFile()))
	{
		currentTree = ValueTree::fromXml(*xml);
		np->getOrCreate(currentTree);
	}
	else
	{
		np->getOrCreate(getXmlFile().getFileNameWithoutExtension());

		
		currentTree = np->getActiveNetwork()->getValueTree();
	}

	np->prepareToPlay(np->getSampleRate(), np->getLargestBlockSize());

	source = SourceMode::InterpretedNode;
	setRootValueTree(currentTree);
}

}




scriptnode::dll::FunkyHostFactory::FunkyHostFactory(DspNetwork* n, ProjectDll::Ptr dll) :
	NodeFactory(n),
	dllFactory(dll)
{
	auto numNodes = dllFactory.getNumNodes();

	for (int i = 0; i < numNodes; i++)
	{
		NodeFactory::Item item;

		item.id = Identifier(dllFactory.getId(i));
		item.cb = [this, i](DspNetwork* p, ValueTree v)
		{
			auto t = new scriptnode::InterpretedNode(p, v);
			t->initFromDll(dllFactory, i);
			return t;
		};

		monoNodes.add(item);
	}
}
