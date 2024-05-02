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





namespace hise {
using namespace juce;

#if 0
struct DebuggableSnexProcessor : public snex::ui::WorkbenchManager::WorkbenchChangeListener,
	public snex::ui::WorkbenchData::Listener
{
	DebuggableSnexProcessor(MainController* mc):
		mc_(mc)
	{
		auto wb = static_cast<WorkbenchManager*>(mc->getWorkbenchManager());
		wb->addListener(this);
	}

	void workbenchChanged(WorkbenchData::Ptr newWorkbench) override
	{
		if (newWorkbench != nullptr)
		{
			auto wb = static_cast<WorkbenchManager*>(mc_->getWorkbenchManager());
			auto isRoot = wb->getRootWorkbench() == newWorkbench;
			
			if (isRoot)
			{
				rootWb = newWorkbench.get();
				rootWb->addListener(this);
			}
		}
	}

	virtual ~DebuggableSnexProcessor()
	{
		if (rootWb != nullptr)
			rootWb->removeListener(this);

		if (auto wb = static_cast<WorkbenchManager*>(mc_->getWorkbenchManager()))
			wb->removeListener(this);
	}

	WorkbenchData::WeakPtr rootWb;

	bool enableCppPreview = false;

private:

	MainController* mc_;
};

struct WorkbenchSynthesiser : public JavascriptSynthesiser,
							  public DebuggableSnexProcessor
{
	WorkbenchSynthesiser(MainController* mc) :
		JavascriptSynthesiser(mc, "internal", NUM_POLYPHONIC_VOICES),
		DebuggableSnexProcessor(mc)
	{
		
	};

	void debugModeChanged(bool isEnabled) override
	{
		setSoftBypass(isEnabled, false);

		if (!isEnabled)
			prepareToPlay(getSampleRate(), getLargestBlockSize());
	}

	Colour getColour() const override
	{
		return MultiOutputDragSource::getFadeColour(0, 2);
	};

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(WorkbenchSynthesiser);
};


struct DspNetworkProcessor : public ProcessorWithScriptingContent,
							 public MasterEffectProcessor,
							 public scriptnode::DspNetwork::Holder,
							 public snex::ExternalDataHolderWithForcedUpdate,
							 public DebuggableSnexProcessor
{
	SET_PROCESSOR_NAME("DspNetworkProcessor", "DspNetworkProcessor", "Internally used by the SNEX workbench");

	DspNetworkProcessor(MainController* mc, const String& id) :
		ProcessorWithScriptingContent(mc),
		MasterEffectProcessor(mc, id),
		DebuggableSnexProcessor(mc)
	{
		finaliseModChains();
	};

	~DspNetworkProcessor()
	{
		
	}

	void debugModeChanged(bool isEnabled) override
	{
		setBypassed(isEnabled, sendNotificationSync);

		if (!isEnabled)
			prepareToPlay(getSampleRate(), getLargestBlockSize());
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		if (activeNetwork == nullptr)
			return;

		SimpleReadWriteLock::ScopedReadLock sl(lock);
        activeNetwork->prepareToPlay(sampleRate, samplesPerBlock);
        activeNetwork->setNumChannels(2);
	}

	void workbenchChanged(WorkbenchData::Ptr newWorkbench) override
	{
		DebuggableSnexProcessor::workbenchChanged(newWorkbench);

		if(rootWb != nullptr)
			testData = &rootWb->getTestData();
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
		activeNetwork->process(b, eventBuffer);
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

		if (auto p = activeNetwork->getRootNode()->getParameterFromIndex(parameterIndex))
			p->setValueAsync(newValue);
	}

	float getAttribute(int parameterIndex) const override
	{
		if (activeNetwork == nullptr)
			return 0.0f;

		if (auto p = activeNetwork->getRootNode()->getParameterFromIndex(parameterIndex))
			return p->getValue();

		return 0.0f;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		
	}

	ValueTree exportAsValueTree() const override
	{
		return MasterEffectProcessor::exportAsValueTree();
	}

	hise::SimpleReadWriteLock lock;

	int getActiveNetworkIndex() const
	{
		return networks.indexOf(activeNetwork);
	}

	scriptnode::DspNetwork* getActiveNetwork() const
	{
		return  activeNetwork;
	}

	int getNumDataObjects(ExternalData::DataType t) const override
	{
		return testData != nullptr ? testData->getNumDataObjects(t) : 0;
	}

	Table* getTable(int index) override { return testData != nullptr ? testData->getTable(index) : nullptr; };
	SliderPackData* getSliderPack(int index) override { return testData != nullptr ? testData->getSliderPack(index) : nullptr; };
	MultiChannelAudioBuffer* getAudioFile(int index) override { return testData != nullptr ? testData->getAudioFile(index) : nullptr; };
	FilterDataObject* getFilterData(int index)  override { return testData != nullptr ? testData->getFilterData(index) : nullptr; };
	SimpleRingBuffer* getDisplayBuffer(int index)  override { return testData != nullptr ? testData->getDisplayBuffer(index) : nullptr; };

	bool removeDataObject(ExternalData::DataType t, int index) override
	{
		if(testData != nullptr)
			return testData->removeDataObject(t, index);
		return false;
	}

	WeakReference<ExternalDataHolder> testData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetworkProcessor);

	
};

using namespace snex::ui;


struct DspNetworkSubBase: public ControlledObject,
						  public valuetree::AnyListener
{
	virtual ~DspNetworkSubBase() {}

	DspNetworkSubBase(WorkbenchData* d, MainController* mc, DspNetwork::Holder* np_):
		ControlledObject(mc),
		AnyListener(valuetree::AsyncMode::Synchronously),
		np(np_)
	{
		
	}

	WeakReference<DspNetwork::Holder> np;
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

			return {};
		}

		String getId() const override { return {}; }

		Path createPath(const String& id) const override
		{
			Path p;

			auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

			LOAD_EPATH_IF_URL("dll", HnodeIcons::dllIcon);
			LOAD_EPATH_IF_URL("code", HiBinaryData::SpecialSymbols::scriptProcessor);
			LOAD_EPATH_IF_URL("jit", HnodeIcons::jit);
			LOAD_EPATH_IF_URL("scriptnode", ScriptnodeIcons::pinIcon);
			LOAD_EPATH_IF_URL("main_logo", ScriptnodeIcons::mainLogo);
			LOAD_EPATH_IF_URL("parameters", HiBinaryData::SpecialSymbols::macros);

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

	DspNetworkCodeProvider(WorkbenchData* d, MainController* mc, DspNetwork::Holder* np_, const File& fileToWriteTo);

	~DspNetworkCodeProvider() {};

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

	String createCppForNetwork() const;

	File getXmlFile() const
	{
		return connectedFile;
	}

	

	void anythingChanged(valuetree::AnyListener::CallbackType d);

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


struct WorkbenchInfoComponent : public Component,
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
			auto pc = findParentComponentOfClass<WorkbenchInfoComponent>();

			auto dnp = pc->getWorkbench()->getCodeProvider();
			
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

	WorkbenchInfoComponent(WorkbenchData* d);

	void anythingChanged(valuetree::AnyListener::CallbackType d) override
	{
		scriptnodeButton.setChanged(true);
	}

	~WorkbenchInfoComponent()
	{
	}

	

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("SnexWorkbenchInfo"); }

	void paint(Graphics& g) override;

	void paintOverChildren(Graphics& g) override
	{
		
	}

#if 0
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
#endif

	void setWorkbench(WorkbenchData::Ptr)
	{

	}

	WorkbenchData::Ptr getWorkbench()
	{
		if (auto pc = findParentComponentOfClass<ComponentWithBackendConnection>())
		{
			auto bp = pc->getBackendRootWindow()->getBackendProcessor();
			auto r = static_cast<WorkbenchManager*>(bp->getWorkbenchManager());
			return r->getRootWorkbench();
		}
		
		return nullptr;
	}

	void buttonClicked(Button* b) override
	{
		if (b == &parameterButton)
		{
			auto np = new ParameterList(getWorkbench().get());
			np->setSize(jmax(1, np->sliders.size()) * 160, 48);
			np->setName("Parameters");

			findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(np, &parameterButton, parameterButton.getLocalBounds().getCentre());
		}

		if (b == &signalButton)
		{
			auto rootTile = findParentComponentOfClass<FloatingTile>()->getRootFloatingTile();
			using PanelType = snex::ui::SnexWorkbenchPanel<snex::ui::TestGraph>;
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

	void resized() override;

	Path scriptnodePath;
	Path codePath;

	
	CompileSourceButton scriptnodeButton;
	CompileSourceButton jitNodeButton;
	CompileSourceButton codeButton;
	CompileSourceButton dllButton;

	HiseShapeButton consoleButton;
	HiseShapeButton parameterButton;
	HiseShapeButton signalButton;

	ScopedPointer<Drawable> mainLogoColoured;
	

	TooltipBar tooltips;
	Path mainLogo;

	snex::cppgen::CustomNodeProperties data;
};

struct WorkbenchBottomComponent : public Component
{
	struct RoutingSelector;

	WorkbenchBottomComponent(MainController* mc);

	void resized() override
	{
		auto b = getLocalBounds();

		auto kBounds = b.withSizeKeepingCentre(800, getHeight());
		keyboard.setBounds(kBounds);

		routingSelector->setBounds(getLocalBounds().removeFromLeft(getHeight() * 3));
	}

	void paint(Graphics& g) override;
	
	CustomKeyboard keyboard;

	ScopedPointer<Component> routingSelector;
};

struct DspNetworkCompileHandler : public WorkbenchData::CompileHandler,
							      public DspNetworkSubBase
{
	DspNetworkCompileHandler(WorkbenchData* d, MainController* mc, DspNetwork::Holder* np_):
		CompileHandler(d),
		DspNetworkSubBase(d, mc, np_)
	{
	}

	void anythingChanged(valuetree::AnyListener::CallbackType ) override
	{
		
	}

	WorkbenchData::CompileResult compile(const String& codeToCompile) override;

	void initExternalData(ExternalDataHolder* h) override;

	void processTestParameterEvent(int parameterIndex, double value) override;

	Result prepareTest(PrepareSpecs ps, const Array<WorkbenchData::TestData::ParameterEvent>& ) override;

	void processTest(ProcessDataDyn& data) override;

	void postCompile(WorkbenchData::CompileResult& lastResult) override;;

	WeakReference<scriptnode::DspNetwork> interpreter;

	scriptnode::OpaqueNode dllNode;

	snex::ui::WorkbenchData::CompileResult lastResult;

	JitCompiledNode::Ptr jitNode;
};
#endif

}
