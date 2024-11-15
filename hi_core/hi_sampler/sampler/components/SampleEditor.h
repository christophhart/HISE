
#ifndef __JUCE_HEADER_A3E3B1B2DF55A952__
#define __JUCE_HEADER_A3E3B1B2DF55A952__

namespace hise {
using namespace juce;

struct ExternalFileChangeWatcher;
class SamplerBody;
class SampleEditHandler;

struct DraggableThumbnail : public HiseAudioThumbnail
{
	struct Laf : public HiseAudioThumbnail::LookAndFeelMethods,
				 public LookAndFeel_V3
	{
		void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path) override;

		void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area) override;
	} laf;

	DraggableThumbnail();

    ~DraggableThumbnail()
    {
        setLookAndFeel(nullptr);
    }
    
	void setPosition(const MouseEvent& e);

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseDoubleClick(const MouseEvent& e) override;

	void paint(Graphics& g) override;

	static void mainSoundSelected(DraggableThumbnail& d, ModulatorSamplerSound::Ptr, int);

	ModulatorSamplerSound::Ptr currentSound;
	float downZoomFactor = 1.0f;
	int downX = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DraggableThumbnail);
};

struct ExternalFileChangeWatcher : public Timer,
                                   public SampleMap::Listener
{
    ExternalFileChangeWatcher(ModulatorSampler* s, Array<File> fileList_):
        fileList(fileList_),
        sampler(s)
    {
        startTimer(1000);

        sampler->getSampleMap()->addListener(this);

        for (const File& f : fileList)
            modificationTimes.add(f.getLastModificationTime());
    }

    virtual void sampleMapWasChanged(PoolReference newSampleMap)
    {
        stopTimer();
        sampler->getSampleMap()->removeListener(this);
    }

    virtual void sampleMapCleared()
    {
        stopTimer();
        sampler->getSampleMap()->removeListener(this);
    };

    void timerCallback() override
    {
        for (int i = 0; i < fileList.size(); i++)
        {
            auto t = fileList[i].getLastModificationTime();

            if (t != modificationTimes[i])
            {
                stopTimer();

                if (PresetHandler::showYesNoWindow("Detected File change", "Press OK to reload the samplemap"))
                {
                    sampler->getSampleMap()->saveAndReloadMap();
                }
                
                modificationTimes.clear();

                for (const auto& l : fileList)
                    modificationTimes.add(l.getLastModificationTime());
                
                startTimer(1000);
            }
        }
    }

    WeakReference<ModulatorSampler> sampler;
    const Array<File> fileList;
    Array<Time> modificationTimes;
};


class SampleEditor : public Component,
	public SamplerSubEditor,
	public AudioDisplayComponent::Listener,
    public ComboBox::Listener,
	public SafeChangeListener,
	public SampleMap::Listener,
	public ScrollBar::Listener,
    public Timer
{
public:
    
    /** All application commands are collected here. */
    enum class SampleMapCommands
    {
        // Undo / Redo
        ZoomIn = 0x3000,
        ZoomOut,
        EnableSampleStartArea,
        EnableLoopArea,
        EnablePlayArea,
		EnableReleaseArea,
        SelectWithMidi,
		ApplyToMainOnly,
        NormalizeVolume,
        LoopEnabled,
		PreviewCurrentSound,
        Analyser,
        ExternalEditor,
        ZeroCrossing,
		ShowEnvelopePopup,
		ImproveLoopPoints,
		ShowReleaseStartOptions,
		ShowScriptPopup,
		ToggleFirstScriptButton,
        numCommands
    };
    
	//==============================================================================
	SampleEditor(ModulatorSampler *s, SamplerBody *b);
	~SampleEditor();

    void timerCallback() override
    {
		if (!isShowing())
			return;

        for(auto b: menuButtons)
        {
            auto state = getCommandIdForName(b->getName());
            b->setToggleStateAndUpdateIcon(getState(state));
        }
    }
    
    static String getNameForCommand(SampleMapCommands c, bool on=true);
    static SampleMapCommands getCommandIdForName(const String& n);
    static String getTooltipForCommand(SampleMapCommands c);
    bool getState(SampleMapCommands c) const;
    void perform(SampleMapCommands c);
    
	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		updateWaveform();
	}

	std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override;

	void updateInterface() override
	{
		updateWaveform();
	}

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

	bool isInWorkspace() const;

	void sampleMapWasChanged(PoolReference r) override
	{
		currentWaveForm->setSoundToDisplay(nullptr);
		updateWaveform();
	}

	void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) override;

	void sampleAmountChanged() override
	{
		if(currentWaveForm->getCurrentSound() == nullptr)
		{
			currentWaveForm->setSoundToDisplay(nullptr);
		}
	}

	void rangeChanged(AudioDisplayComponent *c, int areaThatWasChanged) override
	{
		SamplerSoundWaveform *waveForm = dynamic_cast<SamplerSoundWaveform*>(c);

		if(waveForm == nullptr) return;

		const ModulatorSamplerSound *currentSound = waveForm->getCurrentSound();

		if(currentSound != nullptr && selection.getLast() == currentSound)
		{
			auto soundToChange = selection.getLast();

			AudioDisplayComponent::SampleArea *area = c->getSampleArea(areaThatWasChanged);

			const int64 startSample = (int64)jmax<int>(0, (int)area->getSampleRange().getStart());
			const int64 endSample = (int64)(area->getSampleRange().getEnd());

			switch (areaThatWasChanged)
			{
			case SamplerSoundWaveform::SampleStartArea:
			{
				soundToChange->setSampleProperty(SampleIds::SampleStartMod, (int)(endSample - startSample));
				soundToChange->closeFileHandle();
				break;
			}
			case SamplerSoundWaveform::LoopArea:
			{
				if (!area->leftEdgeClicked)
				{
					soundToChange->setSampleProperty(SampleIds::LoopEnd, (int)endSample);
				}
				else
				{
					soundToChange->setSampleProperty(SampleIds::LoopStart, (int)startSample);
				}
				break;
			}
			case SamplerSoundWaveform::PlayArea:
			{
				if (!area->leftEdgeClicked)
				{
					soundToChange->setSampleProperty(SampleIds::SampleEnd, (int)endSample);
				}
				else
				{
					soundToChange->setSampleProperty(SampleIds::SampleStart, (int)startSample);
				}
				break;
			}
			case SamplerSoundWaveform::LoopCrossfadeArea:
			{
				jassert(area->leftEdgeClicked);
				soundToChange->setSampleProperty(SampleIds::LoopXFade, (int)(endSample - startSample));
				break;
			}
			}

			//currentWaveForm->updateRanges();
		}
	};

	static File getPropertyFile()
	{
		return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("SampleEditorSettings").withFileExtension("js");
	}

	void saveEditorSettings();

	void loadEditorSettings();
	
	static void mainSelectionChanged(SampleEditor& editor, ModulatorSamplerSound::Ptr sound, int micIndex);

	void soundsSelected(int numSelected) override;

	void paintOverChildren(Graphics &g);

	void updateWaveform();

	void zoom(bool zoomOut, int mousePos=0);

	void setZoomFactor(float factor, int mousePos = 0);

    void mouseMagnify(const MouseEvent& e, float scaleFactor) override
    {
        setZoomFactor(scaleFactor * zoomFactor, e.getPosition().getX());
    }
    
	void mouseWheelMove	(const MouseEvent & e, const MouseWheelDetails & 	wheel )	override
	{
		if(e.mods.isCtrlDown())
			zoom(wheel.deltaY < 0, e.getEventRelativeTo(viewport).getPosition().getX());
		else
			getParentComponent()->mouseWheelMove(e, wheel);
	};

    void comboBoxChanged(ComboBox* cb) override
    {
        refreshDisplayFromComboBox();
    }
    
    void refreshDisplayFromComboBox();
    
    
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();

	Component* addButton(SampleMapCommands commandId, bool hasState);

	bool keyPressed(const KeyPress& key) override;

	float zoomFactor;
    
	Viewport& getViewport() { return *viewport; }

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	friend class SampleEditorToolbarFactory;

	LookAndFeel_V4 slaf;
    ScrollbarFader::Laf laf;
    GlobalHiseLookAndFeel claf;
	
	ModulatorSampler *sampler;
	SamplerBody	*body;

    Slider spectrumSlider;
    
	HiseShapeButton* envelopeButton;

	ScrollbarFader fader;
	

	ScopedPointer<Component> viewContent;

	ScopedPointer<SamplerSoundWaveform> currentWaveForm;

	SampleSelection selection;

	OwnedArray<HiseShapeButton> menuButtons;

	Component* analyseButton;
	Component* externalButton;
	Component* improveButton;
	Component* showReleaseButton;
	Component* scriptButton;

	

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Viewport> viewport;
    ScopedPointer<ValueSettingComponent> volumeSetter;
    ScopedPointer<ValueSettingComponent> pitchSetter;
    ScopedPointer<ValueSettingComponent> sampleStartSetter;
    ScopedPointer<ValueSettingComponent> sampleEndSetter;
    ScopedPointer<ValueSettingComponent> loopStartSetter;
    ScopedPointer<ValueSettingComponent> loopEndSetter;
    ScopedPointer<ValueSettingComponent> loopCrossfadeSetter;
	ScopedPointer<ValueSettingComponent> releaseStartSetter;
    ScopedPointer<ValueSettingComponent> startModulationSetter;
    ScopedPointer<ValueSettingComponent> panSetter;
	ScopedPointer<ExternalFileChangeWatcher> externalWatcher;

	ScopedPointer<Component> verticalZoomer;
    ScopedPointer<ComboBox> sampleSelector;
    ScopedPointer<ComboBox> multimicSelector;
    
	DraggableThumbnail overview;

    

	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleEditor);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_A3E3B1B2DF55A952__
