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

#if USE_BACKEND
struct Selector : public Component,
				  public ControlledObject,
				  public PathFactory,
				  public ComboBox::Listener,
                  public Timer
{
	Selector(DspNetwork::Holder* holder_, MainController* mc):
		ControlledObject(mc),
		holder(holder_),
		newButton("new", nullptr, *this),
		importButton("import", nullptr, *this),
		embeddedButton("embedded", nullptr, *this)
	{
		addAndMakeVisible(selector);
		addAndMakeVisible(newButton);
		addAndMakeVisible(importButton);
		addAndMakeVisible(embeddedButton);

		selector.addListener(this);

		auto files = BackendDllManager::getNetworkFiles(getMainController(), true);

		int index = 1;

		selector.setLookAndFeel(&slaf);

		for (auto f : files)
		{
			selector.addItem(f.getFileNameWithoutExtension(), index++);
		}

		newButton.onClick = [this]()
		{
			auto n = PresetHandler::getCustomName("DspNetwork");

			n = snex::cppgen::StringHelpers::makeValidCppName(n);

			auto f = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::Networks).getChildFile(n).withFileExtension(".xml");

			if (!f.existsAsFile())
				f.replaceWithText("<empty/>");

			this->setNetwork(n);
		};

		static const String svgData = "3173.nT6K8C1CdzsX.nAvzpwKP71.L..nH...EAjqLc2hFV1fTsRlSJm.psDlhe5LyLyTxIyqflPygkRfpLkC1926AjX.6EfoRqeMsB8cjhHWq8ZIrbdEhnu5UFuqTC45HpdnMd0eb45yJls0TGljo0PaXQScqHinmZxxRAHKcnJ4QIkUOKWU6TDs7VQqKCIzMTqyNsLgkins7Qxt0YtPRRIg9vx9yGkHwr0Fcb51RRd9iVNdyfldRMqT0bd1E8pkU0TQm5p2RkTpQ2ZTKa8cbpT3AGFAFLj.H3+G58i49CbAVnhIr.U.Y+37GFcehrJcTF5pJIcbjmmFuxZZhNdQNwBh2flpSVWKK5hnpoqRqTlEZMljBm86TzSmFBRGtubadkujj28qyHR1jpCcdkJoCcNBhzgc0VQtYLNoIpniYqZ8j8NQ5Ndu9hgoWRNEMOe8L8iCMeIY7EpVcKBo0uBVt9gQnySgG..J..XTA.vBxPB3.CIflfDSTQDWHwFfgNfhJl.FjfhLHBLHwDQPDZ.EUbgEYPhIfPBpPEn.CQngHlPDSzAUDASLgDXvCTvDevEVvPBa.EPXEvAFXfDQngJhvhHpHCOb.NvHBHrfLXwDjvt6a.GXnAXPhI7fIXAMXgDlYHIXgJnv0F7F1vFjfDlPFt3hK7.C3.CKvvDafGv39O7fCLv.ERXQEQnAFJQ14jgLOf.NvvCnXB1PDRngKtfEjHhKvvAl4OL.Nv3BWTAI1fA3.CKnAKRotbrNj5tyiuqmxRkwirVehMmopozFaEjLeD5N9TFI3M2YwTScRnu4YkPEY7XGukxtr71.qw4gd0hPhq4hgMnuGH.GX7ALvS90CEfCL7fGbfwEPDQEGHQVEst3X7W00JlFIkrHeGzo7VMJIhdxHzSVpSE4mjpNcH4nlxRYaRGpEZEQ24hFe4+8nFZMsDw8WorQwx9dUREcxjn1b1PzWV4Yl2Tmz+3kJojEIbd444qbojxz82VaZImSol4uOkoZLe+rRPc8VvM8GzEt9KYMOaAy7JklxFqZwVaVRJszgF+uo7QKYL0BcW80qRpOUN9aYu4jp7LeaiogkGGSKEdP.XfCEoi9ZjChX5qHmNNpFKaWmU4FZhL7bVzKkYjkDY8J2zUOsEdGhEcIcGWEpZhXAKemrZY6ErwqzyIZVagdqk1bLq350JhTZp5QzmQrTMhlnQSIqym5zH1dVMxUjqhoERVOpY7Z3XzJkYlnZMxLz2gR2iUpgLdT38VUfPaQo0ebF4zJGIr7ORlSpXPkVo1ngJpgZdo56H1g6ni9WDdzUOSEKEha136YKc5QFOhlp46Jls9G4lyXodczy0.sFZdzqJWuFAuWkyTUOqNayQpaqTE55vlw+B.RcqSx7hScGY8D7kOD2yyUmq51GjVeGnldnRdTiIpAM248ju5X8k7lUBpp2Btn+btR0kU9d4tiokkceux+ND7nxrdBYk2W3XdnPnQsenB2p1q5cMwbQzL7lyhc1MNIG9ZQiDYqgUdTGeEszQQ8pqH05Ii1xVb0yVo9oqMBUWKpP0W4JiRTIZdNK5OcWuppzJ4rzmnjvidI00OXZtnYGxyI0cL8V1e0JwzbqzXwx6ijH84rQR8TURhbQRTOLQ8Ry1LIx4X69b7KTlqt1MGqZJOkSYdOMNamF1mxt++wLR+2sNjtibXRTZpTSs+VxPz0cak7PaDkLhpB8Uv499tps+XIQxFQ6UVgYciEZLxxmoU9HKt6o1RxFwHTWzEAuUzcUn72clHJspt1guf0bRQ+3dpA1t9UvHeZBBFFMtfP.fnBIf3zzLxGS54T2dOG9RKMt6zxHUsAox6AoaYUsRyFclgKoJWUQVoPEbBuLoSu7K2mdPhvW9KNEMWSFZVgtphLpr4XjdrHFdVYa8QeQqH5dh1WJu55uTuRMSYjS7HcUxUipqrId1MhYR9QCkEZWsetVtlWNJWxPMIulhen6H+PWExT+Z1c8sjRMz3Vnkr04R1h7ta5Y2yzcjYSO8qu4kCyemURE4Z5sr2osQMylVmSgGB.PnQd9J5Nu4pe5mqPHpXkoRTlz+ZyhuN8kMUuTCyLeMrE2Bu6oHqZ4Uj2QeYkHemZuBkn2C8FmDgNhtSG+RdSMvbxiLhqMh7+QcQEivzDGz3R8icLsSGiX4CR3Qmh2Wl2opRUgDzJyvIwUOuG2KOkRSmWiRuuFXkHRevDqqrqjEcKt0X73GITlnREr9ZBI6NmKceQU0FRds1PBcGdnjUlyiaklVKlZdZ7MWDMdpUjXJcechkZMhOz3uZPmklpyK8GUNZ1sM3TnJZdjxRqCQ1HA..AjHij.JtRJDCsAHZFgUXNSD.gDPfdofj9qrrfkPI32tIA+UfpgRQ2Cm4iCGGPOYVHJecRWmQZLlkpCgmXCb+VoiOWLts0FLIZmetRAkTIcM8RrRvBze.gtJYb1wrT4YOmcXQYYTsobdZZJhUdJBHn3rqAUoICJmpmi0fcAA3pIJHLIphEZMTDIQzJTvczq3BNYJ1+BpUjOJrFFBlJ8Em5CQjJ6HMfXVP4vpk9ECnogbAARguysLR751jqgwmVyppgSYD+d2hBSvxBH3mQhmG3YqyRNWl93sSmVGyu+KiqlsIJOTn.F9UOoUrrCl3tTiaPwkH5c+vGmgGU.LnsNizYcslyF1czcdBIYXEvxwxWBJSg.nDTXEoTFjp4fdIsOEy0cja3RzZf1xjll+3OJ7lx98nz618pBxPMhPFHbSvAvUn.fkyQ8NWzk.+fsU56i6xxZMf10zH4lS4sLGrWnJSnRKaZIrbXzYH.LySqkUrPBo7NAMSDINDHOvHrac4wmuxp1Lt3buIssEt0yfVYu6+J2igYhJEzoEcEpDtjqQV9o2BF3gXXsuIiw3aBv5JehtMZSdzQaoHmDmNnVQOR9HL3DAt+FKHEeccULbuoTIEQHTwVYh2VApgAIUdkrhMRRv7O3pOybmcXur6ue.tdAFcJjo7uvUWQWvvBWb5Xz1CN.PEXZ7eTFcwBvVh348X3BUQrn80I4vsFm3bcTa9FMav+fiYHHlFcuDlkphOpGusht5kl8tjiv0tErXRSEnODQmfs8YWsef2gaGP4VCKiSX3gSa3WXYbjO5W0sTMnRS7t8JM3gWrs9g+hCm+mp9PNt7TKzDTVFQ4th5fZ+MldKCVgmKzBashHli0KzQ6x3s2QHUSDJyZRnlHYodedLpD8ul3jQC7.0n5El6rkkm26TralPzpH0sfMW8EUwnAtTC5QIOWu5LFxXPunP+t.ifQtQxixl0Er+R1rMA9IfkijqIOHw1ApaIMkAI0uaRwRvr09oUxfdyNuhq38eQRu+MhaOwAjf8my4V6SgFztoJj9E.yQ.TVpPZcexEDE+OiXBpdffyfE912ltRGMtaTSbGaOYWPRn6ayzHEXbuApfmrGarTqAjIviDr.R8S5TaHh.N.NXFwqJj+PTFgyADJTSu29UgET80Kqz4gUwCQ2OW5u5GL7wKseVDeUkiXzyRUOE3WswpEJWCUo2AApW71gv1VG4Rw1wi2ihJR6LmORZspzdpLmZbSELPBernURMEzuq8ZL5j1E3SU43V2IXGs22DGz.TdbxpBdTAqm08cZ5uc6TLxEQaqv8wQivvD.R6y7vNIB7ZwLFXQpovwWB+KJCZ+mCQRZcErGVWHrDPSITtqPew30zfEnL2H82pf2Aj+EGjUBAXdHmzqBP1tFx.1sc+wEMiIMw2mwA4iviFG3O16Qf3CalFJ22.sImqJDWLmqjAg7NIxmMHAWR026I1s6ApiF3Cz9vFDjnm4VQrgMtQUqSM3xBwUo3nILdL01nc3IIogOJvIBfGCaHkqFytppM.ESynrfDyQwidaiVnfTJ6.7XIkaGAqONT5Pb7LaEEH3EyEk+WWvrpFPZ6smeqHEALlrVdK7BijyBEoqX3uDk499GFeBygstbalPdwFR3aii+O.jN5faE1d1MQwaPnFnzk3OJPnpkSnuV0OJ4up25EzkjCaI3GMHRqCmHscJUCM+tOUHhNo1zSAyV2OgoDlKxK0PYTaKoikq3fbSmvvnG.HV61bYg5wPpmWN8.1YUahQam3CSOe0Kfl8pruMwxpagVSavNKtOnCPSJKLiRzBJNfW5v0U0p5FPDR24L8g8w4UQef.moiFz0KTXMF8eKTHt99VJTdIzZemDB0EpFzD15obiq5RJamdptbcL2dP7mquVw18j3GYfYK26PNYeWg1p7A";

		MemoryBlock mb;
		mb.fromBase64Encoding(svgData);
		zstd::ZDefaultCompressor comp;
		ValueTree v;
		comp.expand(mb, v);

		auto xml = v.createXml();

		selector.setName("selector");

		mainLogoColoured = juce::Drawable::createFromSVG(*xml).release();
		colour = dynamic_cast<Processor*>(holder.get())->getColour();
		addAndMakeVisible(tooltipper);
		selector.addMouseListener(&tooltipper, false);
		newButton.addMouseListener(&tooltipper, true);
		importButton.addMouseListener(&tooltipper, true);
		embeddedButton.addMouseListener(&tooltipper, true);

        startTimer(30);

		embeddedButton.onClick = [this]() { setNetwork(getEmbeddedId()); };
		importButton.onClick = [this]()
		{
			// implement me
			jassertfalse;
		};
	}

    void timerCallback() override
    {
        alpha += 0.02f;
        if(alpha >= 1.0f)
        {
            alpha = 1.0f;
            stopTimer();
        }
        
        repaint();
    }
    
	String getEmbeddedId()
	{
		return MarkdownLink::Helpers::getSanitizedFilename(dynamic_cast<Processor*>(holder.get())->getId());
	}

	void setNetwork(String n)
	{
		auto rootWindow = GET_BACKEND_ROOT_WINDOW(this);
		auto jsp = dynamic_cast<JavascriptProcessor*>(holder.get());

		n = snex::cppgen::StringHelpers::makeValidCppName(n);

		holder->getOrCreate(n);
        
        auto p = dynamic_cast<Processor*>(holder.get());
        
        p->prepareToPlay(p->getSampleRate(), p->getLargestBlockSize());

		if (rootWindow != nullptr)
		{
			auto gw = [rootWindow, jsp]()
			{
				BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(rootWindow, jsp);
				BackendPanelHelpers::showWorkspace(rootWindow, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotification);
			};

			MessageManager::callAsync(gw);
		}
		else
		{
			if (auto pc = findParentComponentOfClass<PanelWithProcessorConnection>())
			{
				auto f = [pc, p]()
				{
					pc->setContentWithUndo(p, 0);
				};

				MessageManager::callAsync(f);
			}
		}
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		setNetwork(selector.getText());
	}

	struct Tooltipper : public Component
	{
		String getTooltip(Component* c)
		{
			auto n = c->getName();

			if (n == "new")
				return "Create a new DSP Network file";
			if (n == "embedded")
				return "Create an embedded DSP network";
			if (n == "import")
				return "Import a scriptnode snippet";
			if (n == "selector")
				return "Load an existing DSP network";

			return "";
		}

		void mouseEnter(const MouseEvent& e)
		{
			currentText = getTooltip(e.eventComponent);
			repaint();
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.4f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(currentText, getLocalBounds().toFloat(), Justification::centred);
		}

		String currentText;
	};

	void paint(Graphics& g) override
	{
        mainLogoColoured->drawWithin(g, iconBounds, RectanglePlacement::centred, alpha);
		
		g.setColour(colour);
		g.fillRoundedRectangle(colourBounds.reduced(0, 2), 4.0f);
	}

    float alpha = 0.2f;
    
	Path createPath(const String& url) const override
	{
		Path p;
		LOAD_EPATH_IF_URL("new", SampleMapIcons::newSampleMap);
		LOAD_EPATH_IF_URL("embedded", HnodeIcons::mapIcon);
		LOAD_EPATH_IF_URL("import", SampleMapIcons::pasteSamples);
		return p;
	}

	void resized() override
	{
		auto b = getLocalBounds().withSizeKeepingCentre(320, 172);

		iconBounds = b.removeFromTop(70).toFloat();
		b.removeFromTop(20);
		auto textBounds = b.removeFromBottom(30);
		
		colourBounds = b.removeFromBottom(10).toFloat();
		b.removeFromBottom(10).toFloat();

		embeddedButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(4));
		importButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(4));
		b.removeFromLeft(10);
		newButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(4));
		selector.setBounds(b);
		tooltipper.setBounds(textBounds);
	}

    scriptnode::ScriptnodeComboBoxLookAndFeel slaf;
    
	Colour colour;
	Rectangle<float> iconBounds;
	Rectangle<float> textBounds;
	Rectangle<float> colourBounds;
	Tooltipper tooltipper;

	WeakReference<DspNetwork::Holder> holder;
	HiseShapeButton newButton;
	HiseShapeButton embeddedButton;
	HiseShapeButton importButton;
	ComboBox selector;
	
	ScopedPointer<Drawable> mainLogoColoured;
};
#endif

DspNetworkGraphPanel::DspNetworkGraphPanel(FloatingTile* parent) :
	NetworkPanel(parent)
{
	
}


void DspNetworkGraphPanel::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF1D1D1D));
}

Component* DspNetworkGraphPanel::createComponentForNetwork(DspNetwork* p)
{
	auto n = new DspNetworkGraph(p);

	return new DspNetworkGraph::WrapperWithMenuBar(n);
}

Component* DspNetworkGraphPanel::createEmptyComponent()
{
#if USE_BACKEND
	if (auto h = dynamic_cast<DspNetwork::Holder*>(getProcessor()))
		return new Selector(h, getMainController());
#endif

	return nullptr;
}

NodePropertyPanel::NodePropertyPanel(FloatingTile* parent):
	NetworkPanel(parent)
{

}

struct NodePropertyContent : public Component,
							 public DspNetwork::SelectionListener
{
	NodePropertyContent(DspNetwork* n):
		network(n)
	{
		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&content, false);
		n->addSelectionListener(this);
	}

	~NodePropertyContent()
	{
		if(network != nullptr)
			network->removeSelectionListener(this);
	}

	void resized() override
	{
		viewport.setBounds(getLocalBounds());
		content.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), content.getHeight());
	}

	void selectionChanged(const NodeBase::List& selection)
	{
		editors.clear();

		auto y = 0;

		for (auto n : selection)
		{
            PropertyEditor* pe = new PropertyEditor(n, false, n->getValueTree(), {PropertyIds::ID, PropertyIds::FactoryPath, PropertyIds::Bypassed});
			editors.add(pe);
			pe->setTopLeftPosition(0, y);
			pe->setSize(content.getWidth(), pe->getHeight());
			content.addAndMakeVisible(pe);
			y = pe->getBottom();
		}

		content.setSize(content.getWidth(), y);
	}

	WeakReference<DspNetwork> network;
	Component content;
	Viewport viewport;
	OwnedArray<PropertyEditor> editors;
};

juce::Component* NodePropertyPanel::createComponentForNetwork(DspNetwork* p)
{
	return new NodePropertyContent(p);
}

#if USE_BACKEND
struct FaustEditorWrapper: public Component,
                           public DspNetwork::FaustManager::FaustListener
{
    FaustEditorWrapper(DspNetwork* network_):
      network(network_)
    {
        network->faustManager.addFaustListener(this);
    }
    
    ~FaustEditorWrapper()
    {
        network->faustManager.removeFaustListener(this);
    }
    
    
    bool keyPressed(const KeyPress& k) override
    {
        if(k == KeyPress::F5Key)
        {
			if (bottomBar != nullptr)
				bottomBar->recompile();

            return true;
        }
        
        return false;
    }
    
    
    void recompile()
    {
        if(currentDocument != nullptr)
        {
            currentDocument->writeFile();
            network->faustManager.sendCompileMessage(currentDocument->file, sendNotificationSync);
        }
    }
    
    void faustFileSelected(const File& f) override
    {
        currentDocument = nullptr;
        
        for(auto d: documents)
        {
            if(d->file == f)
            {
                currentDocument = d;
                break;
            }
        }
        
        if(currentDocument == nullptr)
        {
			auto ef = network->getMainController()->getExternalScriptFile(f, false);

			if(ef != nullptr)
				currentDocument = documents.add(new FaustDocument(ef));
			else if(f.existsAsFile())
				currentDocument = documents.add(new FaustDocument(f));
			else
				return;
        }
        
		bottomBar = new EditorBottomBar(dynamic_cast<JavascriptProcessor*>(network->getScriptProcessor()));
        editor = new mcl::FullEditor(currentDocument->doc);
       

        editor->editor.setLanguageManager(new mcl::FaustLanguageManager());
        
        addAndMakeVisible(editor);
		addAndMakeVisible(bottomBar);
		bottomBar->setCompileFunction(BIND_MEMBER_FUNCTION_0(FaustEditorWrapper::recompile));
		
        resized();
    }
    
    Result compileFaustCode(const File& f) override
    {
        return Result::ok();
    }
    
    void faustCodeCompiled(const File& f, const Result& compileResult) override
    {
        if(editor != nullptr)
        {
            if(currentDocument != nullptr &&
               currentDocument->file == f)
            {
                editor->editor.clearWarningsAndErrors();
                
				if (!compileResult.wasOk())
				{
					auto e = compileResult.getErrorMessage();

					auto sa = StringArray::fromTokens(e, ":", "");

					String errorMessage;

					errorMessage << "Line " << sa[1] << "(0): " << sa[3];

					if (sa.size() > 4)
						errorMessage << ": " << sa[4];

					editor->editor.setError(errorMessage);

					bottomBar->setError(errorMessage);
				}
				else
					bottomBar->setError("");
            }
        }
    }
    
    
    
    void resized() override
    {
        if(editor != nullptr)
        {
			auto b = getLocalBounds();

			bottomBar->setBounds(b.removeFromBottom(EditorBottomBar::BOTTOM_HEIGHT));
            editor->setBounds(b);
        }
    }

    struct FaustDocument
    {
        FaustDocument(const File& f):
          file(f),
          docToUse(&d),
		  doc(*docToUse),
		  resourceType(ExternalScriptFile::ResourceType::FileBased)
        {
			// should use the other constructor...
			jassert(f.existsAsFile());
            d.replaceAllContent(f.loadFileAsString());
        };

		FaustDocument(ExternalScriptFile::Ptr p):
		  file(p->getFile()),
		  docToUse(&p->getFileDocument()),
		  doc(*docToUse),
		  resourceType(ExternalScriptFile::ResourceType::EmbeddedInSnippet)
		{}

		bool writeFile()
		{
			if(resourceType == ExternalScriptFile::ResourceType::FileBased)
			{
				return file.replaceWithText(docToUse->getAllContent());
			}

			return false;
		}

		File file;

	private:
		
		CodeDocument d;

    public:

		const ExternalScriptFile::ResourceType resourceType;

		CodeDocument* docToUse = nullptr;
        mcl::TextDocument doc;
    };
    
    OwnedArray<FaustDocument> documents;
    FaustDocument* currentDocument = nullptr;
    
    ScopedPointer<mcl::FullEditor> editor;
	ScopedPointer<EditorBottomBar> bottomBar;
    
    WeakReference<DspNetwork> network;
    JUCE_DECLARE_WEAK_REFERENCEABLE(FaustEditorWrapper);
};
#endif

FaustEditorPanel::FaustEditorPanel(FloatingTile* parent):
    NetworkPanel(parent)
{

}

juce::Component* FaustEditorPanel::createComponentForNetwork(DspNetwork* p)
{
#if USE_BACKEND
    return new FaustEditorWrapper(p);
#else
    return nullptr;
#endif
}

#if 0
SnexPopupEditor::SnexPopupEditor(const String& name, SnexSource* src, bool isPopup) :
	d(doc),
	Component(name),
	source(src),
	editor(d),
	popupMode(isPopup),
	corner(this, nullptr),
	compileButton("compile", this, f),
	resetButton("reset", this, f),
	asmButton("asm", this, f),
	optimiseButton("optimize", this, f),
	asmView(asmDoc, &tokeniser)
{
	

	optimizations = source->s.getOptimizationPassList();

	codeValue.referTo(source->expression.asJuceValue());
	codeValue.addListener(this);

	d.getCodeDocument().replaceAllContent(codeValue.getValue().toString());
	d.getCodeDocument().clearUndoHistory();

	for (auto& o : snex::OptimizationIds::Helpers::getAllIds())
		s.addOptimization(o);

	s.addDebugHandler(this);

	d.getCodeDocument().addListener(this);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0x33888888));
	d.getCodeDocument().clearUndoHistory();
	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
	editor.setShowNavigation(false);

	asmView.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF333333));
	asmView.setFont(GLOBAL_MONOSPACE_FONT());
	asmView.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF));
	asmView.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	asmView.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	asmView.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
	asmView.setReadOnly(true);
	asmView.setOpaque(false);
	asmDoc.replaceAllContent("; no assembly generated");


	auto numLinesToShow = jmin(20, d.getCodeDocument().getNumLines());
	auto numColToShow = jlimit(60, 90, d.getCodeDocument().getMaximumLineLength());

	auto w = (float)numColToShow * editor.getFont().getStringWidth("M");
	auto h = editor.getFont().getHeight() * (float)numLinesToShow * 1.5f;

	setSize(roundToInt(w), roundToInt(h));

	addAndMakeVisible(editor);

	addAndMakeVisible(asmView);
	asmView.setVisible(false);

	addAndMakeVisible(corner);

	addAndMakeVisible(asmButton);
	addAndMakeVisible(compileButton);
	addAndMakeVisible(resetButton);
	addAndMakeVisible(optimiseButton);

	recompile();
}

void SnexPopupEditor::buttonClicked(Button* b)
{
	if (b == &resetButton && PresetHandler::showYesNoWindow("Do you want to reset the code", "Press OK to load the default code"))
	{

	}
	if (b == &asmButton)
	{
		bool showBoth = getWidth() > 800;

		auto visible = asmView.isVisible();

		asmView.setVisible(!visible);

		editor.setVisible(showBoth || visible);
		resized();
	}
	if (b == &compileButton)
	{
		recompile();
	}
	if (b == &optimiseButton)
	{
		auto allIds = snex::OptimizationIds::Helpers::getAllIds();

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(9001, "Enable All optimisations", true, allIds.size() == optimizations.size());
		m.addSeparator();

		int index = 0;
		for (auto& o : allIds)
		{
			m.addItem(index + 1, o, true, optimizations.contains(allIds[index]));
			index++;
		}

		int result = m.show() - 1;

		if (result == -1)
			return;

		source->s.clearOptimizations();

		if (result == 9000)
		{
			if (allIds.size() != optimizations.size())
				optimizations = allIds;
			else
				optimizations = {};
		}
		else
		{
			bool isActive = optimizations.contains(allIds[result]);

			if (!isActive)
				optimizations.add(allIds[result]);
			else
				optimizations.removeString(allIds[result]);
		}

		source->s.clearOptimizations();
		s.clearOptimizations();

		for (auto& o : optimizations)
		{
			source->s.addOptimization(o);
			s.addOptimization(o);
		}

		source->recompile();
		recompile();
	}
}

void SnexPopupEditor::valueChanged(Value& v)
{
	auto c = v.getValue().toString();

	auto code = d.getCodeDocument().getAllContent();

	if (c != code)
	{
		ScopedValueSetter<bool> s(internalChange, true);
		d.getCodeDocument().replaceAllContent(c);
		d.getCodeDocument().clearUndoHistory();
	}
}

void SnexPopupEditor::recompile()
{
	editor.clearWarningsAndErrors();
	auto code = d.getCodeDocument().getAllContent();

	snex::jit::Compiler compiler(s);
	compiler.setDebugHandler(this);

	source->initCompiler(compiler);

	auto obj = compiler.compileJitObject(code);

	asmDoc.replaceAllContent(compiler.getAssemblyCode());
	asmDoc.clearUndoHistory();

	Colour c;

	if (compiler.getCompileResult().wasOk())
	{
		c = Colour(0xFA181818);
		codeValue.setValue(code);
	}
	else
		c = JUCE_LIVE_CONSTANT_OFF(Colour(0xf21d1616));

	if (!popupMode)
		c = c.withAlpha(1.0f);

	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, c);
}

void SnexPopupEditor::paint(Graphics& g)
{
	if (!popupMode)
	{
		g.fillAll(Colour(0xFF333333));
	}

	auto b = getLocalBounds();

	Rectangle<int> top;

	if (popupMode)
		top = b.removeFromTop(24);
	else
		top = b.removeFromBottom(24);

	GlobalHiseLookAndFeel::drawFake3D(g, top);
}

void SnexPopupEditor::resized()
{
	auto b = getLocalBounds();

	Rectangle<int> top;

	if (popupMode)
		top = b.removeFromTop(24);
	else
		top = b.removeFromBottom(24);

	compileButton.setBounds(top.removeFromRight(24));

	bool showBoth = getWidth() > 800;

	
	asmButton.setBounds(top.removeFromRight(24));
	resetButton.setBounds(top.removeFromRight(24));
	optimiseButton.setBounds(top.removeFromRight(24));

	if (asmView.isVisible())
	{
		asmView.setBounds(showBoth ? b.removeFromRight(550) : b);
	}
	
	editor.setBounds(b);


	if (!popupMode)
		corner.setVisible(false);

	corner.setBounds(getLocalBounds().removeFromBottom(15).removeFromRight(15));
}

juce::Path SnexPopupEditor::Icons::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
	LOAD_PATH_IF_URL("optimize", SnexIcons::optimizeIcon);

	return p;
}
#endif

#if USE_BACKEND
WorkbenchTestPlayer::WorkbenchTestPlayer(FloatingTile* parent) :
	FloatingTileContent(parent),
	SimpleTimer(parent->getMainController()->getGlobalUIUpdater()),
	playButton("start", nullptr, factory),
	stopButton("stop", nullptr, factory),
	midiButton("midi", nullptr, factory)
{
	addAndMakeVisible(playButton);
	addAndMakeVisible(stopButton);
	addAndMakeVisible(midiButton);

	playButton.setToggleModeWithColourChange(true);
	midiButton.setToggleModeWithColourChange(true);

	playButton.onClick = BIND_MEMBER_FUNCTION_0(WorkbenchTestPlayer::play);
	stopButton.onClick = BIND_MEMBER_FUNCTION_0(WorkbenchTestPlayer::stop);

	addAndMakeVisible(inputPreview);
	addAndMakeVisible(outputPreview);

	workbenchChanged(dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getCurrentWorkbench());
}

void WorkbenchTestPlayer::postPostCompile(WorkbenchData::Ptr wb)
{
    if(wb == nullptr)
        return;
    
	auto& td = wb->getTestData();

    auto& b1 = td.testSourceData;
    auto& b2 = td.testOutputData;
    
    auto size = b1.getNumSamples();
    int numChannels = b1.getNumChannels();
    
    if(b1.getNumSamples() * b1.getNumChannels() == 0 ||
       b2.getNumSamples() * b2.getNumChannels() == 0)
        return;
    
	auto il = new VariantBuffer(b1.getWritePointer(0), size);
	auto ir = new VariantBuffer(b1.getWritePointer(jmin(1, numChannels-1)), size);
	auto ol = new VariantBuffer(b2.getWritePointer(0), size);
	auto or_ = new VariantBuffer(b2.getWritePointer(jmin(1, numChannels-1)), size);

	inputPreview.setBuffer(var(il), var(ir));
	outputPreview.setBuffer(var(ol), var(or_));
}

void WorkbenchTestPlayer::play()
{
	playButton.setToggleStateAndUpdateIcon(true);
	getMainController()->setBufferToPlay(wb->getTestData().testOutputData, 44100.0);
}

void WorkbenchTestPlayer::stop()
{
	playButton.setToggleStateAndUpdateIcon(false);
	getMainController()->setBufferToPlay({}, 44100.0);
}

void WorkbenchTestPlayer::timerCallback()
{
	//auto index = getMainController()->getPreviewBufferPosition();
	//inputPreview.setPlaybackPosition((double)index / wb->getTestData().testSourceData.getNumSamples());
}

void WorkbenchTestPlayer::resized()
{
	auto b = getParentShell()->getContentBounds();

	auto buttonHeight = 24;

	auto topBar = b.removeFromTop(buttonHeight);
	playButton.setBounds(topBar.removeFromLeft(buttonHeight).reduced(1));
	stopButton.setBounds(topBar.removeFromLeft(buttonHeight).reduced(1));
	midiButton.setBounds(topBar.removeFromLeft(buttonHeight).reduced(1));
	
	inputPreview.setBounds(b.removeFromTop(b.getHeight() / 2));
	outputPreview.setBounds(b);
}

void WorkbenchTestPlayer::paint(Graphics& g)
{
	PopupLookAndFeel::drawFake3D(g, getLocalBounds().removeFromTop(24));
}

juce::Path WorkbenchTestPlayer::Factory::createPath(const String& url) const
{
	MidiPlayerBaseType::TransportPaths tf;

	auto p = tf.createPath(url);

	if (!p.isEmpty())
		return p;

	LOAD_EPATH_IF_URL("midi", HiBinaryData::SpecialSymbols::midiData);

	return p;
}
#endif

}

