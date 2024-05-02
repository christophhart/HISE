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


#if 0

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
					dll::DynamicLibraryHostFactory f(fh->projectDll);

					auto id = dcg->getInstanceId();

					for (int i = 0; i < f.getNumNodes(); i++)
					{
						if (id.toString() == f.getId(i))
						{
							found = f.initOpaqueNode(&dllNode, i, fh->isPolyphonic());
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

				auto rootNode = np->getActiveOrDebuggedNetwork()->getRootNode();

				np->getActiveOrDebuggedNetwork()->setExternalDataHolder(&getParent()->getTestData());

				for (auto p : NodeBase::ParameterIterator(*rootNode))
				{
					scriptnode::parameter::data d;

					auto f = [](void* obj, double value)
					{
						auto typed = static_cast<scriptnode::NodeBase::Parameter*>(obj);
						typed->setValueAsync(value);
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
	if (interpreter != nullptr)
	{
		if (auto p = interpreter->getRootNode()->getParameterFromIndex(parameterIndex))
			p->setValueAsync(value);
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

Result DspNetworkCompileHandler::prepareTest(PrepareSpecs ps, const Array<WorkbenchData::TestData::ParameterEvent>& initialParameters)
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
    
    return Result::ok();
}

void DspNetworkCompileHandler::processTest(ProcessDataDyn& data)
{
	if (dllNode.getObjectPtr() != nullptr)
		dllNode.process(data);
	else if (interpreter != nullptr)
	{
		DspNetwork::VoiceSetter svs(*interpreter, 0);

		interpreter->process(data);
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
					interpreter = np->getActiveOrDebuggedNetwork();
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
            PrepareSpecs ps;
            ps.sampleRate = getMainController()->getMainSynthChain()->getSampleRate();
            ps.blockSize = getMainController()->getMainSynthChain()->getLargestBlockSize();
            ps.numChannels = 2;
            
			testData.initProcessing(ps);
			testData.processTestData(getParent());
		}
	}
}

DspNetworkCodeProvider::DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, DspNetwork::Holder* networkHolder, const File& fileToWriteTo) :
	CodeProvider(d),
	DspNetworkSubBase(d, mc, networkHolder),
	connectedFile(fileToWriteTo)
{
	setMillisecondsBetweenUpdate(100);
}

void DspNetworkCodeProvider::initNetwork()
{
	DspNetwork* n = nullptr;

	if (auto xml = XmlDocument::parse(getXmlFile()))
	{
		currentTree = ValueTree::fromXml(*xml);
		n = np->getOrCreate(currentTree);
	}
	else
	{
		n = np->getOrCreate(getXmlFile().getFileNameWithoutExtension());
		currentTree = np->getActiveOrDebuggedNetwork()->getValueTree();
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

	if (auto xml = chain.createXml())
	{
		getTestNodeFile().replaceWithText(xml->createDocument(""));

        
        
		snex::cppgen::ValueTreeBuilder v(chain, snex::cppgen::ValueTreeBuilder::Format::JitCompiledInstance);

		v.setCodeProvider(new BackendDllManager::FileCodeProvider(getMainController()));

		auto r = v.createCppCode();

		return r.code;
	}

	return {};
}

void DspNetworkCodeProvider::anythingChanged(valuetree::AnyListener::CallbackType d)
{
	if (!getParent()->getGlobalScope().isDebugModeEnabled())
		return;

	WeakReference<DspNetworkCodeProvider> safeThis(this);



	auto f = [safeThis, d](Processor* p)
	{
		if (safeThis.get() == nullptr)
			return SafeFunctionCall::OK;

		auto t = safeThis.get();

		if (t->getParent() != nullptr)
		{
			DBG("CHANGE");
			t->getParent()->triggerPostCompileActions();
		}
        
        return SafeFunctionCall::OK;
	};

	getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}

namespace RoutingIcons
{
	static const unsigned char monoIcon[] = { 110,109,209,34,192,66,164,112,85,65,98,51,243,195,66,193,202,181,64,92,207,209,66,0,0,0,0,205,76,226,66,0,0,0,0,98,100,187,245,66,0,0,0,0,197,192,2,67,127,106,252,64,197,192,2,67,242,210,140,65,98,197,192,2,67,80,141,218,65,100,187,245,66,248,211,12,
66,205,76,226,66,248,211,12,66,98,53,222,210,66,248,211,12,66,244,189,197,66,23,217,241,65,244,253,192,66,43,135,186,65,108,68,75,163,66,43,135,186,65,98,68,139,158,66,23,217,241,65,2,107,145,66,248,211,12,66,106,252,129,66,248,211,12,66,98,166,27,101,
66,248,211,12,66,35,219,74,66,23,217,241,65,35,91,65,66,43,135,186,65,108,43,7,9,66,43,135,186,65,98,86,14,255,65,23,217,241,65,80,141,202,65,248,211,12,66,242,210,140,65,248,211,12,66,98,127,106,252,64,248,211,12,66,0,0,0,0,80,141,218,65,0,0,0,0,242,
210,140,65,98,0,0,0,0,127,106,252,64,127,106,252,64,0,0,0,0,242,210,140,65,0,0,0,0,98,193,202,206,65,0,0,0,0,172,28,3,66,193,202,181,64,119,190,10,66,164,112,85,65,108,215,163,63,66,164,112,85,65,98,162,69,71,66,193,202,181,64,238,252,98,66,0,0,0,0,106,
252,129,66,0,0,0,0,98,219,121,146,66,0,0,0,0,4,86,160,66,193,202,181,64,102,38,164,66,164,112,85,65,108,209,34,192,66,164,112,85,65,99,101,0,0 };

	static const unsigned char polyIcon[] = { 110,109,252,233,129,66,0,0,189,66,98,6,65,149,66,0,0,189,66,63,245,164,66,182,179,204,66,63,245,164,66,68,11,224,66,98,63,245,164,66,209,98,243,66,6,65,149,66,68,139,1,67,252,233,129,66,68,139,1,67,98,221,36,93,66,68,139,1,67,106,188,61,66,209,98,243,
66,106,188,61,66,68,11,224,66,98,106,188,61,66,182,179,204,66,221,36,93,66,0,0,189,66,252,233,129,66,0,0,189,66,99,109,156,132,162,66,4,86,196,65,108,139,44,210,66,254,212,68,66,98,12,2,215,66,4,214,63,66,113,125,220,66,25,4,61,66,205,76,226,66,25,4,
61,66,98,100,187,245,66,25,4,61,66,197,192,2,67,98,144,92,66,197,192,2,67,201,182,129,66,98,197,192,2,67,221,36,149,66,100,187,245,66,133,235,164,66,205,76,226,66,133,235,164,66,98,139,108,222,66,133,235,164,66,39,177,218,66,193,74,164,66,57,52,215,66,
203,33,163,66,108,133,107,164,66,57,180,215,66,98,84,163,162,66,98,80,208,66,61,138,158,66,111,210,201,66,33,240,152,66,61,10,197,66,108,221,228,197,66,6,129,150,66,98,104,209,195,66,139,172,147,66,109,39,194,66,37,134,144,66,244,253,192,66,215,35,141,
66,108,68,75,163,66,215,35,141,66,98,68,139,158,66,82,248,154,66,2,107,145,66,133,235,164,66,106,252,129,66,133,235,164,66,98,166,27,101,66,133,235,164,66,35,219,74,66,82,248,154,66,35,91,65,66,215,35,141,66,108,43,7,9,66,215,35,141,66,98,154,153,6,66,
139,172,144,66,123,20,3,66,188,244,147,66,248,83,253,65,203,225,150,66,108,219,121,87,66,4,86,196,66,98,111,18,76,66,115,232,200,66,68,139,67,66,182,51,207,66,225,122,63,66,145,109,214,66,108,2,43,183,65,74,76,163,66,98,205,204,169,65,29,90,164,66,117,
147,155,65,133,235,164,66,242,210,140,65,133,235,164,66,98,127,106,252,64,133,235,164,66,0,0,0,0,221,36,149,66,0,0,0,0,201,182,129,66,98,0,0,0,0,98,144,92,66,127,106,252,64,25,4,61,66,242,210,140,65,25,4,61,66,98,127,106,163,65,25,4,61,66,156,196,184,
65,20,174,63,66,33,176,203,65,121,105,68,66,108,203,161,67,66,229,208,200,65,98,16,216,73,66,74,12,227,65,182,243,83,66,59,223,248,65,201,118,96,66,215,163,3,66,108,29,90,4,66,145,237,97,66,98,209,34,7,66,92,15,103,66,205,76,9,66,117,147,108,66,119,190,
10,66,59,95,114,66,108,215,163,63,66,59,95,114,66,98,162,69,71,66,113,189,83,66,238,252,98,66,25,4,61,66,106,252,129,66,25,4,61,66,98,219,121,146,66,25,4,61,66,4,86,160,66,113,189,83,66,102,38,164,66,59,95,114,66,108,209,34,192,66,59,95,114,66,98,236,
209,192,66,72,225,108,66,117,211,193,66,221,164,103,66,172,28,195,66,125,191,98,66,108,147,152,148,66,170,113,2,66,98,106,188,154,66,227,165,245,65,197,160,159,66,184,30,223,65,156,132,162,66,4,86,196,65,99,109,252,233,129,66,0,0,0,0,98,6,65,149,66,0,
0,0,0,63,245,164,66,100,59,251,64,63,245,164,66,14,45,140,65,98,63,245,164,66,68,139,217,65,6,65,149,66,20,46,12,66,252,233,129,66,20,46,12,66,98,221,36,93,66,20,46,12,66,106,188,61,66,68,139,217,65,106,188,61,66,14,45,140,65,98,106,188,61,66,100,59,
251,64,221,36,93,66,0,0,0,0,252,233,129,66,0,0,0,0,99,101,0,0 };

	static const unsigned char arrowIcon[] = { 110,109,0,0,0,0,229,208,147,65,108,0,0,0,0,4,86,14,65,108,188,116,32,66,4,86,14,65,108,188,116,32,66,0,0,0,0,108,51,51,87,66,219,249,90,65,108,188,116,32,66,231,251,218,65,108,188,116,32,66,229,208,147,65,108,0,0,0,0,229,208,147,65,99,101,0,0 };

}

struct WorkbenchBottomComponent::RoutingSelector : public Component,
												 public PathFactory
{
	RoutingSelector()
	{
		setRepaintsOnMouseActivity(true);
	}

	void resized() override
	{
		auto b = getLocalBounds().toFloat().reduced(10.0f);

		p = createPath("poly");
		a = createPath("arrow");
		m = createPath("mono");
		
		pArea = b.removeFromLeft(32.0f);

		scalePath(p, pArea);
		b.removeFromLeft(5.0f);
		scalePath(a, b.removeFromLeft(20.0f));
		b.removeFromLeft(5.0f);

		mArea = b.removeFromLeft(32.0f);

		scalePath(m, mArea);
	};

	Rectangle<float> pArea, mArea;

	int hoverIndex = -1;

	int getIndex(const MouseEvent& e) const
	{
		auto pos = e.getPosition().toFloat();

		if (pArea.contains(pos))
			return 0;

		if (mArea.contains(pos))
			return 1;

		return -1;
	}

	void mouseMove(const MouseEvent& e) override
	{
		hoverIndex = getIndex(e);
		repaint();
	}

	void mouseExit(const MouseEvent& e) override
	{
		hoverIndex = getIndex(e);
		repaint();
	}

	bool hasPoly = false;
	bool hasMono = false;

	void mouseDown(const MouseEvent& e) override
	{
		auto swe = findParentComponentOfClass<SnexWorkbenchEditor>();

		hoverIndex = getIndex(e);
		repaint();

		swe->setSynthMode(hoverIndex == 0);
	}

	void mouseDoubleClick(const MouseEvent& e)
	{
		auto isPolyClick = getIndex(e) == 0;

		if (isPolyClick)
			hasPoly = !hasPoly;
		else
			hasMono = !hasMono;

		repaint();
	}

	bool hasSomething(bool poly) const
	{
		return poly ? hasPoly : hasMono;
	}

	bool isViewed(bool poly) const
	{
		auto swe = findParentComponentOfClass<SnexWorkbenchEditor>();
		return swe->getSynthMode() == poly;
	}

	void paint(Graphics& g) override
	{
		auto pAlpha = 0.0f;
		auto mAlpha = 0.0f;

		if (hasSomething(true))
			pAlpha += 0.2f;
		
		if (isViewed(true))
			pAlpha *= 2.0f;

		if (hasSomething(false))
			mAlpha += 0.2f;

		if (isViewed(false))
			mAlpha *= 2.0f;

		auto pc = MultiOutputDragSource::getFadeColour(0, 2).withAlpha(pAlpha);
		auto mc = MultiOutputDragSource::getFadeColour(1, 2).withAlpha(mAlpha);

		float clickAlpha = 0.05f;

		if (isMouseButtonDown())
			clickAlpha += 0.05f;

		if (hoverIndex == 0)
		{
			g.setColour(Colours::white.withAlpha(clickAlpha));
			g.fillRoundedRectangle(pArea, 3.0f);
		}
		
		if (hoverIndex == 1)
		{
			g.setColour(Colours::white.withAlpha(clickAlpha));
			g.fillRoundedRectangle(mArea, 3.0f);
		}

		g.setColour(pc);
		g.fillPath(p);
		g.setColour(mc);
		g.fillPath(m);
		
		float alpha = 0.8f;

		g.setColour(Colours::white.withAlpha(alpha));

		g.fillPath(a);

		g.setColour(mc.withAlpha(isViewed(false) ? 0.9f : 0.3f));
		g.strokePath(m, PathStrokeType(1.0f));
		g.setColour(pc.withAlpha(isViewed(true) ? 0.9f : 0.3f));
		g.strokePath(p, PathStrokeType(1.0f));
	}

	Path p, m, a;

	String getId() const override { return {}; }

	Path createPath(const String& url) const override
	{
		Path p;
		LOAD_PATH_IF_URL("poly", RoutingIcons::polyIcon);
		LOAD_PATH_IF_URL("mono", RoutingIcons::monoIcon);
		LOAD_PATH_IF_URL("arrow", RoutingIcons::arrowIcon);
		return p;
	}
};

WorkbenchBottomComponent::WorkbenchBottomComponent(MainController* mc) :
	keyboard(mc),
	routingSelector(new RoutingSelector())
{
	keyboard.setUseVectorGraphics(true, false);
	addAndMakeVisible(keyboard);
	addAndMakeVisible(routingSelector);
}

void WorkbenchBottomComponent::paint(Graphics& g)
{
	g.setGradientFill(ColourGradient(Colour(0xFF363636), 0.0f, 0.0f, Colour(0xFF323232), 0.0f, (float)getHeight(), false));

	g.fillAll();
	g.setColour(Colour(0xFF050505));
	g.drawHorizontalLine(0, 0.0f, (float)getWidth());
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

	auto xml = v.createXml();

	mainLogoColoured = juce::Drawable::createFromSVG(*xml).release();
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

#endif

}
