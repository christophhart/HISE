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
#if 0
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
#endif
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

		DspNetwork::VoiceSetter svs(*interpreter, 0);

		if (interpreter->getExceptionHandler().isOk())
		{
			interpreter->getRootNode()->process(data);
		}
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

				if (np != nullptr)
					interpreter = np->getActiveNetwork();
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

DspNetworkCodeProvider::DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, DspNetwork::Holder* networkHolder, const File& fileToWriteTo) :
	CodeProvider(d),
	DspNetworkSubBase(d, mc, networkHolder),
	connectedFile(fileToWriteTo)
{
	setMillisecondsBetweenUpdate(300);
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

	auto asP = dynamic_cast<Processor*>(np.get());

	jassert(asP != nullptr);

	asP->prepareToPlay(asP->getSampleRate(), asP->getLargestBlockSize());

	source = SourceMode::InterpretedNode;
	setRootValueTree(currentTree);
}

String DspNetworkCodeProvider::createCppForNetwork() const
{
	auto chain = currentTree.getChildWithName(scriptnode::PropertyIds::Node);

	if (ScopedPointer<XmlElement> xml = chain.createXml())
	{
		getTestNodeFile().replaceWithText(xml->createDocument(""));

		snex::cppgen::ValueTreeBuilder v(chain, snex::cppgen::ValueTreeBuilder::Format::JitCompiledInstance);

		v.setCodeProvider(new BackendDllManager::FileCodeProvider(getMainController()));

		auto r = v.createCppCode();

		if (r.r.wasOk())
			return r.code;
	}

	return {};
}

void DspNetworkCodeProvider::anythingChanged(valuetree::AnyListener::CallbackType d)
{
	return;

	if (getParent() != nullptr)
	{
		if (source == SourceMode::InterpretedNode && d != valuetree::AnyListener::PropertyChange)
			getParent()->triggerRecompile();
		else
			getParent()->triggerPostCompileActions();
	}
}

WorkbenchInfoComponent::WorkbenchInfoComponent(WorkbenchData* d) :
	AnyListener(valuetree::AsyncMode::Synchronously),
	codeButton(DspNetworkCodeProvider::SourceMode::CustomCode),
	jitNodeButton(DspNetworkCodeProvider::SourceMode::JitCompiledNode),
	scriptnodeButton(DspNetworkCodeProvider::SourceMode::InterpretedNode),
	dllButton(DspNetworkCodeProvider::SourceMode::DynamicLibrary),
	consoleButton("console", this, f),
	parameterButton("parameters", this, f),
	signalButton("signal", this, f)
{
	signalButton.setToggleModeWithColourChange(true);
	consoleButton.setToggleModeWithColourChange(true);
	consoleButton.setToggleStateAndUpdateIcon(true);
	signalButton.setToggleStateAndUpdateIcon(true);

	addAndMakeVisible(consoleButton);
	addAndMakeVisible(parameterButton);
	addAndMakeVisible(signalButton);

	addAndMakeVisible(tooltips);
	addAndMakeVisible(scriptnodeButton);
	addAndMakeVisible(jitNodeButton);
	addAndMakeVisible(codeButton);
	addAndMakeVisible(dllButton);

	static const String svgData = "3173.nT6K8C1CdzsX.nAvzpwKP71.L..nH...EAjqLc2hFV1fTsRlSJm.psDlhe5LyLyTxIyqflPygkRfpLkC1926AjX.6EfoRqeMsB8cjhHWq8ZIrbdEhnu5UFuqTC45HpdnMd0eb45yJls0TGljo0PaXQScqHinmZxxRAHKcnJ4QIkUOKWU6TDs7VQqKCIzMTqyNsLgkins7Qxt0YtPRRIg9vx9yGkHwr0Fcb51RRd9iVNdyfldRMqT0bd1E8pkU0TQm5p2RkTpQ2ZTKa8cbpT3AGFAFLj.H3+G58i49CbAVnhIr.U.Y+37GFcehrJcTF5pJIcbjmmFuxZZhNdQNwBh2flpSVWKK5hnpoqRqTlEZMljBm86TzSmFBRGtubadkujj28qyHR1jpCcdkJoCcNBhzgc0VQtYLNoIpniYqZ8j8NQ5Ndu9hgoWRNEMOe8L8iCMeIY7EpVcKBo0uBVt9gQnySgG..J..XTA.vBxPB3.CIflfDSTQDWHwFfgNfhJl.FjfhLHBLHwDQPDZ.EUbgEYPhIfPBpPEn.CQngHlPDSzAUDASLgDXvCTvDevEVvPBa.EPXEvAFXfDQngJhvhHpHCOb.NvHBHrfLXwDjvt6a.GXnAXPhI7fIXAMXgDlYHIXgJnv0F7F1vFjfDlPFt3hK7.C3.CKvvDafGv39O7fCLv.ERXQEQnAFJQ14jgLOf.NvvCnXB1PDRngKtfEjHhKvvAl4OL.Nv3BWTAI1fA3.CKnAKRotbrNj5tyiuqmxRkwirVehMmopozFaEjLeD5N9TFI3M2YwTScRnu4YkPEY7XGukxtr71.qw4gd0hPhq4hgMnuGH.GX7ALvS90CEfCL7fGbfwEPDQEGHQVEst3X7W00JlFIkrHeGzo7VMJIhdxHzSVpSE4mjpNcH4nlxRYaRGpEZEQ24hFe4+8nFZMsDw8WorQwx9dUREcxjn1b1PzWV4Yl2Tmz+3kJojEIbd444qbojxz82VaZImSol4uOkoZLe+rRPc8VvM8GzEt9KYMOaAy7JklxFqZwVaVRJszgF+uo7QKYL0BcW80qRpOUN9aYu4jp7LeaiogkGGSKEdP.XfCEoi9ZjChX5qHmNNpFKaWmU4FZhL7bVzKkYjkDY8J2zUOsEdGhEcIcGWEpZhXAKemrZY6ErwqzyIZVagdqk1bLq350JhTZp5QzmQrTMhlnQSIqym5zH1dVMxUjqhoERVOpY7Z3XzJkYlnZMxLz2gR2iUpgLdT38VUfPaQo0ebF4zJGIr7ORlSpXPkVo1ngJpgZdo56H1g6ni9WDdzUOSEKEha136YKc5QFOhlp46Jls9G4lyXodczy0.sFZdzqJWuFAuWkyTUOqNayQpaqTE55vlw+B.RcqSx7hScGY8D7kOD2yyUmq51GjVeGnldnRdTiIpAM248ju5X8k7lUBpp2Btn+btR0kU9d4tiokkceux+ND7nxrdBYk2W3XdnPnQsenB2p1q5cMwbQzL7lyhc1MNIG9ZQiDYqgUdTGeEszQQ8pqH05Ii1xVb0yVo9oqMBUWKpP0W4JiRTIZdNK5OcWuppzJ4rzmnjvidI00OXZtnYGxyI0cL8V1e0JwzbqzXwx6ijH84rQR8TURhbQRTOLQ8Ry1LIx4X69b7KTlqt1MGqZJOkSYdOMNamF1mxt++wLR+2sNjtibXRTZpTSs+VxPz0cak7PaDkLhpB8Uv499tps+XIQxFQ6UVgYciEZLxxmoU9HKt6o1RxFwHTWzEAuUzcUn72clHJspt1guf0bRQ+3dpA1t9UvHeZBBFFMtfP.fnBIf3zzLxGS54T2dOG9RKMt6zxHUsAox6AoaYUsRyFclgKoJWUQVoPEbBuLoSu7K2mdPhvW9KNEMWSFZVgtphLpr4XjdrHFdVYa8QeQqH5dh1WJu55uTuRMSYjS7HcUxUipqrId1MhYR9QCkEZWsetVtlWNJWxPMIulhen6H+PWExT+Z1c8sjRMz3Vnkr04R1h7ta5Y2yzcjYSO8qu4kCyemURE4Z5sr2osQMylVmSgGB.PnQd9J5Nu4pe5mqPHpXkoRTlz+ZyhuN8kMUuTCyLeMrE2Bu6oHqZ4Uj2QeYkHemZuBkn2C8FmDgNhtSG+RdSMvbxiLhqMh7+QcQEivzDGz3R8icLsSGiX4CR3Qmh2Wl2opRUgDzJyvIwUOuG2KOkRSmWiRuuFXkHRevDqqrqjEcKt0X73GITlnREr9ZBI6NmKceQU0FRds1PBcGdnjUlyiaklVKlZdZ7MWDMdpUjXJcechkZMhOz3uZPmklpyK8GUNZ1sM3TnJZdjxRqCQ1HA..AjHij.JtRJDCsAHZFgUXNSD.gDPfdofj9qrrfkPI32tIA+UfpgRQ2Cm4iCGGPOYVHJecRWmQZLlkpCgmXCb+VoiOWLts0FLIZmetRAkTIcM8RrRvBze.gtJYb1wrT4YOmcXQYYTsobdZZJhUdJBHn3rqAUoICJmpmi0fcAA3pIJHLIphEZMTDIQzJTvczq3BNYJ1+BpUjOJrFFBlJ8Em5CQjJ6HMfXVP4vpk9ECnogbAARguysLR751jqgwmVyppgSYD+d2hBSvxBH3mQhmG3YqyRNWl93sSmVGyu+KiqlsIJOTn.F9UOoUrrCl3tTiaPwkH5c+vGmgGU.LnsNizYcslyF1czcdBIYXEvxwxWBJSg.nDTXEoTFjp4fdIsOEy0cja3RzZf1xjll+3OJ7lx98nz618pBxPMhPFHbSvAvUn.fkyQ8NWzk.+fsU56i6xxZMf10zH4lS4sLGrWnJSnRKaZIrbXzYH.LySqkUrPBo7NAMSDINDHOvHrac4wmuxp1Lt3buIssEt0yfVYu6+J2igYhJEzoEcEpDtjqQV9o2BF3gXXsuIiw3aBv5JehtMZSdzQaoHmDmNnVQOR9HL3DAt+FKHEeccULbuoTIEQHTwVYh2VApgAIUdkrhMRRv7O3pOybmcXur6ue.tdAFcJjo7uvUWQWvvBWb5Xz1CN.PEXZ7eTFcwBvVh348X3BUQrn80I4vsFm3bcTa9FMav+fiYHHlFcuDlkphOpGusht5kl8tjiv0tErXRSEnODQmfs8YWsef2gaGP4VCKiSX3gSa3WXYbjO5W0sTMnRS7t8JM3gWrs9g+hCm+mp9PNt7TKzDTVFQ4th5fZ+MldKCVgmKzBashHli0KzQ6x3s2QHUSDJyZRnlHYodedLpD8ul3jQC7.0n5El6rkkm26TralPzpH0sfMW8EUwnAtTC5QIOWu5LFxXPunP+t.ifQtQxixl0Er+R1rMA9IfkijqIOHw1ApaIMkAI0uaRwRvr09oUxfdyNuhq38eQRu+MhaOwAjf8my4V6SgFztoJj9E.yQ.TVpPZcexEDE+OiXBpdffyfE912ltRGMtaTSbGaOYWPRn6ayzHEXbuApfmrGarTqAjIviDr.R8S5TaHh.N.NXFwqJj+PTFgyADJTSu29UgET80Kqz4gUwCQ2OW5u5GL7wKseVDeUkiXzyRUOE3WswpEJWCUo2AApW71gv1VG4Rw1wi2ihJR6LmORZspzdpLmZbSELPBernURMEzuq8ZL5j1E3SU43V2IXGs22DGz.TdbxpBdTAqm08cZ5uc6TLxEQaqv8wQivvD.R6y7vNIB7ZwLFXQpovwWB+KJCZ+mCQRZcErGVWHrDPSITtqPew30zfEnL2H82pf2Aj+EGjUBAXdHmzqBP1tFx.1sc+wEMiIMw2mwA4iviFG3O16Qf3CalFJ22.sImqJDWLmqjAg7NIxmMHAWR026I1s6ApiF3Cz9vFDjnm4VQrgMtQUqSM3xBwUo3nILdL01nc3IIogOJvIBfGCaHkqFytppM.ESynrfDyQwidaiVnfTJ6.7XIkaGAqONT5Pb7LaEEH3EyEk+WWvrpFPZ6smeqHEALlrVdK7BijyBEoqX3uDk499GFeBygstbalPdwFR3aii+O.jN5faE1d1MQwaPnFnzk3OJPnpkSnuV0OJ4up25EzkjCaI3GMHRqCmHscJUCM+tOUHhNo1zSAyV2OgoDlKxK0PYTaKoikq3fbSmvvnG.HV61bYg5wPpmWN8.1YUahQam3CSOe0Kfl8pruMwxpagVSavNKtOnCPSJKLiRzBJNfW5v0U0p5FPDR24L8g8w4UQef.moiFz0KTXMF8eKTHt99VJTdIzZemDB0EpFzD15obiq5RJamdptbcL2dP7mquVw18j3GYfYK26PNYeWg1p7A";

	MemoryBlock mb;
	mb.fromBase64Encoding(svgData);
	zstd::ZDefaultCompressor comp;
	ValueTree v;
	comp.expand(mb, v);

	ScopedPointer<XmlElement> xml = v.createXml();

	mainLogoColoured = juce::Drawable::createFromSVG(*xml);
}

void WorkbenchInfoComponent::paint(Graphics& g)
{
	g.setGradientFill(ColourGradient(Colour(0xFF363636), 0.0f, 0.0f, Colour(0xFF323232), 0.0f, (float)getHeight(), false));

	g.fillAll();
	g.setColour(Colour(0xFF050505));
	g.drawHorizontalLine(getHeight()-1, 0.0f, (float)getWidth());
	
	if (mainLogoColoured != nullptr)
		mainLogoColoured->drawWithin(g, getLocalBounds().toFloat().translated(0.0f, 2.0f), RectanglePlacement::onlyReduceInSize, 1.0f);
}

void WorkbenchInfoComponent::resized()
{
	auto b = getLocalBounds();

	auto hasWorkbench = getWorkbench() != nullptr;

	parameterButton.setVisible(hasWorkbench);
	dllButton.setVisible(hasWorkbench);
	scriptnodeButton.setVisible(hasWorkbench);

	tooltips.setBounds(b.removeFromRight(250).reduced(0, 12));
	dllButton.setBounds(b.removeFromRight(b.getHeight()).reduced(4));
	scriptnodeButton.setBounds(b.removeFromRight(b.getHeight()).reduced(4));
	parameterButton.setBounds(b.removeFromRight(b.getHeight()).reduced(4));
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
