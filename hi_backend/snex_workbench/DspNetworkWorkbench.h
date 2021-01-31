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

#pragma once


namespace scriptnode
{
	namespace dll
	{
		struct FunkyHostFactory : public scriptnode::NodeFactory
		{
			FunkyHostFactory(DspNetwork* n, dll::ProjectDll::Ptr dll);

			Identifier getId() const override
			{
				RETURN_STATIC_IDENTIFIER("project");
			}

			HostFactory dllFactory;
		};

	}
}



namespace hise {
using namespace juce;






struct DspNetworkProcessor : public ProcessorWithScriptingContent,
							 public MasterEffectProcessor,
							 public scriptnode::DspNetwork::Holder
{
	SET_PROCESSOR_NAME("DspNetworkProcessor", "DspNetworkProcessor", "Internally used by the SNEX workbench");

	DspNetworkProcessor(MainController* mc, const String& id) :
		ProcessorWithScriptingContent(mc),
		MasterEffectProcessor(mc, id)
	{
		finaliseModChains();
	};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		if (activeNetwork == nullptr)
			return;

		SimpleReadWriteLock::ScopedReadLock sl(lock);
		activeNetwork->prepareToPlay(sampleRate, samplesPerBlock);
	}

	SET_PROCESSOR_CONNECTOR_TYPE_ID("ScriptProcessor");

	bool hasTail() const override { return false; }

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override
	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);

		if (activeNetwork == nullptr)
			return;

		jassert(startSample == 0);
		jassert(numSamples == b.getNumSamples());
		activeNetwork->process(b, &eventBuffer);
		eventBuffer.clear();
	}

	int getControlCallbackIndex() const override { return 0; }

	Processor* getChildProcessor(int processorIndex) override { return nullptr; }
	const Processor* getChildProcessor(int processorIndex) const override { return nullptr; }

	int getNumChildProcessors() const override { return 0; }

	ProcessorEditorBody* createEditor(ProcessorEditor* parentEditor) override
	{
		return new hise::EmptyProcessorEditorBody(parentEditor);
	}

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		if (activeNetwork == nullptr)
			return;

		if (auto p = activeNetwork->getRootNode()->getParameter(parameterIndex))
			p->setValueAndStoreAsync(newValue);
	}

	float getAttribute(int parameterIndex) const override
	{
		if (activeNetwork == nullptr)
			return 0.0f;

		if (auto p = activeNetwork->getRootNode()->getParameter(parameterIndex))
			return p->getValue();
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		
	}

	ValueTree exportAsValueTree() const override
	{
		return MasterEffectProcessor::exportAsValueTree();
	}

	void handleHiseEvent(const HiseEvent &m) override
	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);
		eventBuffer.addEvent(m);
	}

	hise::SimpleReadWriteLock lock;
	HiseEventBuffer eventBuffer;

	int getActiveNetworkIndex() const
	{
		return networks.indexOf(activeNetwork);
	}

	scriptnode::DspNetwork* getActiveNetwork() const
	{
		return  activeNetwork;
	}



	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetworkProcessor);
};

using namespace snex::ui;


struct DspNetworkSubBase: public ControlledObject,
						  public valuetree::AnyListener
{
	virtual ~DspNetworkSubBase() {}

	DspNetworkSubBase(WorkbenchData* d, MainController* mc):
		ControlledObject(mc),
		AnyListener(valuetree::AsyncMode::Synchronously)
	{
		
	}

	void initRoot()
	{
		np = ProcessorHelpers::getFirstProcessorWithType<DspNetworkProcessor>(getMainController()->getMainSynthChain());
	}

	WeakReference<DspNetworkProcessor> np;
};



struct DspNetworkCodeProvider : public WorkbenchData::CodeProvider,
								public DspNetworkSubBase
{
	enum class SourceMode
	{
		InterpretedNode,
		JitCompiledNode,
		DynamicLibrary,
		CustomCode,
		numSourceModes
	};

	struct IconFactory: public PathFactory
	{
		static String getSourceName(DspNetworkCodeProvider::SourceMode m)
		{
			if (m == DspNetworkCodeProvider::SourceMode::CustomCode)
				return "code";

			if (m == DspNetworkCodeProvider::SourceMode::InterpretedNode)
				return "scriptnode";

			if (m == DspNetworkCodeProvider::SourceMode::JitCompiledNode)
				return "jit";

			if (m == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
				return "dll";
		}

		String getId() const override { return {}; }

		Path createPath(const String& id) const override
		{
			Path p;

			auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

			LOAD_PATH_IF_URL("dll", HnodeIcons::exportIcon);
			LOAD_PATH_IF_URL("code", HiBinaryData::SpecialSymbols::scriptProcessor);
			LOAD_PATH_IF_URL("jit", HnodeIcons::jit);
			LOAD_PATH_IF_URL("scriptnode", ScriptnodeIcons::splitIcon);
			LOAD_PATH_IF_URL("parameters", HiBinaryData::SpecialSymbols::macros);

			if(url == "console")
				return FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::Console);
			
			if (url == "signal")
				return FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::Plotter);

			return p;
		}
	};

	

	struct OverlayComponent: public Component,
						     public TooltipClient
	{
		
		OverlayComponent(SourceMode m, WorkbenchData::Ptr wb_) :
			mode(m),
			wb(wb_),
			switchButton("Switch Mode")
		{
			IconFactory f;
			p = f.createPath(f.getSourceName(mode));
			setInterceptsMouseClicks(true, true);

			if (m == SourceMode::InterpretedNode)
				switchButton.setTooltip("Activate JIT compilation");
			else
				switchButton.setTooltip("Activate interpreted graph");

			switchButton.onClick = [this]()
			{
				if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(wb->getCodeProvider()))
				{
					if (mode == SourceMode::InterpretedNode)
						dnp->setSource(SourceMode::JitCompiledNode);
					else
						dnp->setSource(SourceMode::InterpretedNode);
				}
			};

			addAndMakeVisible(switchButton);
			switchButton.setLookAndFeel(&blaf);
		}

		String getTooltip() override
		{
			if (mode == SourceMode::InterpretedNode)
			{
				return "JIT compilation is deactivated";
			}
			else
			{
				return "JIT compilation is active";
			}
		}

		void resized() override
		{
			PathFactory::scalePath(p, getLocalBounds().toFloat().withSizeKeepingCentre(50.0f, 50.0f));

			auto b = p.getBounds().removeFromBottom(20.0f).translated(0.0, 30.0f).expanded(40.0f, 2.0f);

			switchButton.setBounds(b.toNearestInt());

		}

		void paint(Graphics& g) override
		{
			g.setColour(HiseColourScheme::getColour(HiseColourScheme::EditorBackgroundColourId).withAlpha(0.6f));
			g.fillAll();

			g.setColour(Colours::black.withAlpha(0.4f));

			auto tb = p.getBounds().expanded(20.0f);


			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			


			g.fillPath(p);
		}

		WorkbenchData::Ptr wb;
		BlackTextButtonLookAndFeel blaf;
		TextButton switchButton;

		SourceMode mode;
		Path p;
	};

	DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, const File& fileToWriteTo);

	void initNetwork();

	void setSource(SourceMode m)
	{
		if (m == SourceMode::InterpretedNode)
		{
			setMillisecondsBetweenUpdate(1000);
		}
		else
		{
			setMillisecondsBetweenUpdate(0);
		}
		
		source = m;
		getParent()->triggerRecompile();
	}

	Identifier getInstanceId() const override
	{
		return Identifier(connectedFile.getFileNameWithoutExtension());
	}

	String loadCode() const override
	{
		if (source == SourceMode::CustomCode)
		{
			if (customCode.isNotEmpty())
				return customCode;
		}

		auto s = createCppForNetwork();

		return s;
	}

	bool saveCode(const String& s) override
	{
		customCode = s;
		source = SourceMode::CustomCode;

		return true;
	}

	File getTestNodeFile() const
	{
		auto p = GET_HISE_SETTING(dynamic_cast<const Processor*>(getMainController()->getMainSynthChain()), HiseSettings::Compiler::HisePath);
		File root(p);
		return root.getChildFile("tools/snex_playground/test_files/node.xml");
	}

	String createCppForNetwork() const
	{
		auto chain = currentTree.getChildWithName(scriptnode::PropertyIds::Node);

		if (ScopedPointer<XmlElement> xml = chain.createXml())
		{
			getTestNodeFile().replaceWithText(xml->createDocument(""));

			snex::cppgen::ValueTreeBuilder v(chain, snex::cppgen::ValueTreeBuilder::Format::JitCompiledInstance);

			auto r = v.createCppCode();

			if (r.r.wasOk())
				return r.code;
		}
		
		return {};
	}

	File getXmlFile() const
	{
		return connectedFile;
	}

	void anythingChanged()
	{
		if (getParent() != nullptr)
		{
			if (source == SourceMode::InterpretedNode)
			{
				getParent()->triggerRecompile();
			}
			else
			{
				getParent()->triggerPostCompileActions();
			}
		}
	}

#if 0
	void setProjectFactory(DynamicLibrary* dll, scriptnode::DspNetwork* n)
	{
		if (dll == nullptr || n == nullptr)
		{
			MessageManagerLock mm;

			setSource(SourceMode::InterpretedNode);
			projectDllFactory = nullptr;
		}
		else
		{
			projectDllFactory = new scriptnode::dll::FunkyHostFactory(n, dll);
		}
	}
#endif

	ValueTree currentTree;

	String customCode;

	SourceMode source = SourceMode::InterpretedNode;

	File connectedFile;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetworkCodeProvider);
};


struct WorkbenchInfoComponent : public snex::ui::WorkbenchComponent,
	public valuetree::AnyListener,
	public ButtonListener
{

	struct CompileSourceButton : public Component
	{
		CompileSourceButton(DspNetworkCodeProvider::SourceMode m) :
			Component(DspNetworkCodeProvider::IconFactory::getSourceName(m)),
			mode(m)
		{
			DspNetworkCodeProvider::IconFactory f;
			p = f.createPath(getName());

			setRepaintsOnMouseActivity(true);
		}

		void mouseDown(const MouseEvent& e)
		{
			auto dnp = findParentComponentOfClass<WorkbenchComponent>()->getWorkbench()->getCodeProvider();
			
			dynamic_cast<DspNetworkCodeProvider*>(dnp)->setSource(mode);
		}


		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.1f));

			if (isMouseOver(true))
				g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);

			if (isMouseButtonDown(true))
				g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);

			g.setColour(active ? Colour(SIGNAL_COLOUR) : Colours::white.withAlpha(0.5f));

			if (changed)
			{
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText("*", getLocalBounds().toFloat(), Justification::topRight);
			}

			g.fillPath(p);
		}

		void resized() override
		{
			PathFactory::scalePath(p, getLocalBounds().toFloat().reduced(3.0f));
		}

		Path p;

		

		void setMode(DspNetworkCodeProvider::SourceMode m)
		{
			active = m == mode;
			repaint();
		}

		void setChanged(bool shouldBeChanged)
		{
			changed = shouldBeChanged;
			repaint();
		}

		bool changed = false;
		bool active = false;

		const DspNetworkCodeProvider::SourceMode mode;
	};

	DspNetworkCodeProvider::IconFactory f;

	WorkbenchInfoComponent(WorkbenchData* d) :
		WorkbenchComponent(d),
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
		addAndMakeVisible(cpuMeter);

		d->addListener(this);

		cpuMeter.setColour(VuMeter::backgroundColour, Colours::transparentBlack);
		cpuMeter.setColour(VuMeter::ColourId::ledColour, Colours::white.withAlpha(0.45f));
		cpuMeter.setColour(VuMeter::ColourId::outlineColour, Colours::white.withAlpha(0.4f));
		cpuMeter.setOpaque(false);
	}

	void anythingChanged() override
	{
		scriptnodeButton.setChanged(true);
	}

	~WorkbenchInfoComponent()
	{
		if (getWorkbench() != nullptr)
			getWorkbench()->removeListener(this);
	}

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("SnexWorkbenchInfo"); }

	void postPostCompile(WorkbenchData::Ptr wb) override
	{
		auto& r = wb->getTestData();

		cpuUsage = r.cpuUsage;

		cpuMeter.setPeak(cpuUsage);
		repaint();
	}

	void paint(Graphics& g) override
	{
	}

	void paintOverChildren(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_FONT().withHeight(10.0f));

		String c;
		c << "CPU: " << String(cpuUsage * 100.0, 2) << "%";
		g.drawText(c, cpuMeter.getBounds(), Justification::centred, true);
	}

	void recompiled(WorkbenchData::Ptr wb) override
	{
		if (auto dncp = dynamic_cast<DspNetworkCodeProvider*>(wb->getCodeProvider()))
		{
			codeButton.setMode(dncp->source);
			jitNodeButton.setMode(dncp->source);
			scriptnodeButton.setMode(dncp->source);

			setRootValueTree(dncp->currentTree);
		}
	}

	void buttonClicked(Button* b) override
	{
		if (b == &parameterButton)
		{
			auto np = new ParameterList(getWorkbench());
			np->setSize(jmax(1, np->sliders.size()) * 160, 48);
			np->setName("Parameters");

			findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(np, &parameterButton, parameterButton.getLocalBounds().getCentre());
		}

		if (b == &signalButton)
		{
			auto rootTile = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();
			using PanelType = snex::ui::SnexWorkbenchPanel<snex::ui::Graph>;
			auto on = signalButton.getToggleState();

			rootTile->forEach<PanelType>([on](PanelType* pl)
			{
				auto pt = pl->getParentShell();
				pt->getLayoutData().setVisible(on);
				pt->getParentContainer()->refreshLayout();
				return true;
			});
		}

		if (b == &consoleButton)
		{
			auto rootTile = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();
			using PanelType = ConsolePanel;
			auto on = consoleButton.getToggleState();

			rootTile->forEach<PanelType>([on](PanelType* pl)
				{
					auto pt = pl->getParentShell();
					pt->getLayoutData().setVisible(on);
					pt->getParentContainer()->refreshLayout();
					return true;
				});
		}
	}

	void resized() override
	{
		auto b = getLocalBounds();

		tooltips.setBounds(b.removeFromLeft(250));

		auto mid = b.withSizeKeepingCentre((getHeight() + 10) * 3, getHeight());

		signalButton.setBounds(mid.removeFromLeft(getHeight() + 10));
		consoleButton.setBounds(mid.removeFromRight(getHeight() + 10));
		parameterButton.setBounds(mid);

		auto r = b.removeFromRight(250);

		scriptnodeButton.setBounds(r.removeFromLeft(b.getHeight()));
		jitNodeButton.setBounds(r.removeFromLeft(b.getHeight()));
		codeButton.setBounds(r.removeFromLeft(b.getHeight()));
		dllButton.setBounds(r.removeFromLeft(b.getHeight()));

		cpuMeter.setBounds(r);
	}

	Path scriptnodePath;
	Path codePath;

	
	CompileSourceButton scriptnodeButton;
	CompileSourceButton jitNodeButton;
	CompileSourceButton codeButton;
	CompileSourceButton dllButton;

	HiseShapeButton consoleButton;
	HiseShapeButton parameterButton;
	HiseShapeButton signalButton;

	double cpuUsage = 0.0;
	hise::VuMeter cpuMeter;

	TooltipBar tooltips;
};

struct DspNetworkCompileHandler : public WorkbenchData::CompileHandler,
							      public DspNetworkSubBase
{
	DspNetworkCompileHandler(WorkbenchData* d, MainController* mc):
		CompileHandler(d),
		DspNetworkSubBase(d, mc)
	{
		initRoot();

		holder = ProcessorHelpers::getFirstProcessorWithType<scriptnode::DspNetwork::Holder>(getMainController()->getMainSynthChain());
	}

	void anythingChanged() override
	{
		jassertfalse;

		if(auto p = getParent() && getParent()->getCompileHandler() == this)
			getParent()->triggerPostCompileActions();
	}

	WorkbenchData::CompileResult compile(const String& codeToCompile) override;

	void initExternalData(ExternalDataHolder* h) override;

	void processTestParameterEvent(int parameterIndex, double value) override;

	void prepareTest(PrepareSpecs ps, const Array<WorkbenchData::TestData::ParameterEvent>& ) override;

	void processTest(ProcessDataDyn& data) override;

	void postCompile(WorkbenchData::CompileResult& lastResult) override
	{
		if (lastResult.compiledOk())
		{
			auto& testData = getParent()->getTestData();

			if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(getParent()->getCodeProvider()))
			{
				if (dnp->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
				{
					jitNode = nullptr;
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

#if 0
			int bs = 1024;
			double sampleRate = 44100.0;

			testBuffer.setSize(2, bs);
			testBuffer.clear();

			np->prepareToPlay(sampleRate, bs);


			double t = Time::getMillisecondCounterHiRes();
			np->applyEffect(testBuffer, 0, 1024);
			t = Time::getMillisecondCounterHiRes() - t;

			auto delta = t * 0.001;
			auto calculatedSeconds = (double)bs / sampleRate;

			lastResult.cpuUsage = delta / calculatedSeconds;
#endif
		}
	};

	WeakReference<scriptnode::DspNetwork::Holder> holder;

	WeakReference<scriptnode::DspNetwork> interpreter;

	scriptnode::OpaqueNode dllNode;

	snex::ui::WorkbenchData::CompileResult lastResult;

	JitCompiledNode::Ptr jitNode;
};


}
