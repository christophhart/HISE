
namespace hise { using namespace juce;

struct EnvelopePopup : public Component
{
	using EnvelopeType = Modulation::Mode;

	struct Row : public Component,
				 public PathFactory,
				 public ButtonListener
	{
	public:

		Row(EnvelopeType m_):
			m(m_),
			powerButton("power", this, *this),
			applyButton("apply", this, *this),
			showButton("view", this, *this)
		{
			auto c = SamplerDisplayWithTimeline::getColourForEnvelope(m);

			addAndMakeVisible(leftSlider);
			leftSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
			leftSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
			leftSlider.setLookAndFeel(&slaf);
			leftSlider.setValue(-1.0f, dontSendNotification);
			leftSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
			leftSlider.setColour(Slider::trackColourId, c.withSaturation(0.7f).withAlpha(0.5f));
			leftSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
			leftSlider.setPopupDisplayEnabled(true, true, nullptr);
			

			addAndMakeVisible(rightSlider);
			rightSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
			rightSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
			rightSlider.setLookAndFeel(&slaf);
			rightSlider.setValue(-1.0f, dontSendNotification);
			rightSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
			rightSlider.setPopupDisplayEnabled(true, true, nullptr);
			rightSlider.setColour(BubbleComponent::ColourIds::outlineColourId, c);
			rightSlider.setColour(BubbleComponent::ColourIds::backgroundColourId, c.withAlpha(0.3f));
			rightSlider.setColour(Slider::trackColourId, c.withSaturation(0.7f).withAlpha(0.5f));
			rightSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));

			addAndMakeVisible(gammaSlider);
			gammaSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
			gammaSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
			gammaSlider.setLookAndFeel(&slaf);
			gammaSlider.setValue(-1.0f, dontSendNotification);
			gammaSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
			gammaSlider.setPopupDisplayEnabled(true, true, nullptr);
			gammaSlider.setColour(BubbleComponent::ColourIds::outlineColourId, c);
			gammaSlider.setColour(BubbleComponent::ColourIds::backgroundColourId, c.withAlpha(0.3f));
			gammaSlider.setColour(Slider::trackColourId, c.withSaturation(0.7f).withAlpha(0.5f));
			gammaSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
			gammaSlider.setRange(0.0, 100.0, 1.0);
			gammaSlider.setTextValueSuffix("%");
			gammaSlider.setValue(50.0);

			showButton.setToggleModeWithColourChange(true);
			powerButton.setToggleModeWithColourChange(true);

			setupSlider(leftSlider, true);
			setupSlider(rightSlider, false);

			addAndMakeVisible(showButton);
			addAndMakeVisible(powerButton);
			addAndMakeVisible(applyButton);

			for (int i = 0; i < getNumChildComponents(); i++)
				getChildComponent(i)->setWantsKeyboardFocus(false);
		}

		void setupSlider(Slider& s, bool isLeft)
		{
			switch (m)
			{
			case EnvelopeType::GainMode:
				s.setRange(0.0, 100.0, 0.1);
				s.setSkewFactorFromMidPoint(30.0);
				s.setTextValueSuffix("ms");
				s.setTooltip(isLeft ? "The fade in time at the sample start" :
									  "The fade out time at the sample end");
				s.setValue(10.0);
				break;
			case EnvelopeType::PitchMode:
				s.setRange(0.0, 100.0, 1.0);
				s.setTextValueSuffix("%");
				s.setTooltip(isLeft ? "The length of each pitch correction slice" :
									  "The intensity of the pitch correction");
				s.setValue(isLeft ? 50.0 : 100.0);
				break;
			case EnvelopeType::PanMode:
				if (isLeft)
				{
					s.setRange(0.0, 2000.0, 1.0);
					s.setTextValueSuffix("ms");
					s.setValue(1000.0);
					s.setTooltip("The time before the frequency decay starts");
					break;
				}
				else
				{
					s.setRange(20.0, 20000.0, 1.0);
					s.setSkewFactorFromMidPoint(2600.0);
					s.setTextValueSuffix("Hz");
					s.setValue(2600.0);
					s.setTooltip("The frequency decay end point");
					break;
				}
            default:
                break;
			}
		}

		void paint(Graphics& g) override
		{
			auto c = SamplerDisplayWithTimeline::getColourForEnvelope(m);

			auto b = getLocalBounds().toFloat().reduced(1.0f);

			g.setColour(c.withAlpha(0.15f));
			g.fillRoundedRectangle(b, 3.0f);
			g.setColour(c);
			g.drawRoundedRectangle(b, 3.0f, 1.0f);

			b.removeFromBottom(5.0f);
			auto text = b.removeFromBottom(15.0f);

			auto larea = text.withLeft(leftSlider.getX()).withRight(leftSlider.getRight());
			auto rarea = text.withLeft(rightSlider.getX()).withRight(rightSlider.getRight());
			auto garea = text.withLeft(gammaSlider.getX()).withRight(gammaSlider.getRight());

			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(getText(true), larea, Justification::centred);
			g.drawText(getText(false), rarea, Justification::centred);
			g.drawText("Gamma", garea, Justification::centred);
		}

		String getText(bool getLeft)
		{
			switch (m)
			{
			case EnvelopeType::GainMode: return getLeft ? "Fade in" : "Fade out";
			case EnvelopeType::PitchMode: return getLeft ? "Resolution" : "Intensity";
			case EnvelopeType::PanMode: return getLeft ? "Hold" : "Target Frequency";
			default: return {};
			}
		}

		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_EPATH_IF_URL("power", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
			LOAD_EPATH_IF_URL("view", SampleToolbarIcons::envelope);
			LOAD_EPATH_IF_URL("apply", LoopIcons::apply);
			return p;
		}

		void buttonClicked(Button* b) override
		{
			if (b == &powerButton)
			{
				findParentComponentOfClass<EnvelopePopup>()->setEnvelopeActive(m, b->getToggleState());
			}
			if (b == &showButton)
			{
				findParentComponentOfClass<EnvelopePopup>()->setDisplayedEnvelope(m, b->getToggleState());
			}
			if (b == &applyButton)
			{
				powerButton.setToggleState(true, sendNotificationSync);
				findParentComponentOfClass<EnvelopePopup>()->applyToSelection(m, leftSlider.getValue(), rightSlider.getValue(), gammaSlider.getValue() / 100.0);
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromBottom(15);
			b.removeFromLeft(5);
			powerButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(5));
			showButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(5));
			b.removeFromLeft(10);
			applyButton.setBounds(b.removeFromRight(b.getHeight()).reduced(5));
			leftSlider.setBounds(b.removeFromLeft(b.getWidth() / 3));
			rightSlider.setBounds(b.removeFromLeft(b.getWidth() / 2));
			gammaSlider.setBounds(b);
		}

		const Modulation::Mode m;
		LookAndFeel_V4 slaf;
		HiseShapeButton showButton;
		HiseShapeButton applyButton;
		HiseShapeButton powerButton;
		Slider leftSlider, rightSlider, gammaSlider;
	};

	void setDisplayedEnvelope(Modulation::Mode m, bool shouldShow)
	{
        auto& b = sampler->getSampleEditHandler()->toolBroadcaster;
        
        if(!shouldShow)
            b.setMode(SamplerTools::Mode::Nothing);
        else
        {
            switch(m)
            {
                case Modulation::Mode::GainMode:
                    b.setMode(SamplerTools::Mode::GainEnvelope);
                    break;
                case Modulation::Mode::PitchMode:
                    b.setMode(SamplerTools::Mode::PitchEnvelope);
                    break;
                case Modulation::Mode::PanMode:
                    b.setMode(SamplerTools::Mode::FilterEnvelope);
                    break;
				default:
					break;
            }
        }
	}

	void setEnvelopeActive(EnvelopeType m, bool shouldBeActive)
	{
		for(auto s: *sampler->getSampleEditHandler())
		{
			String v;

			if (shouldBeActive)
				v << "24..........7C...vO...f+....7C...vO";

			auto id = SampleIds::Helpers::getEnvelopeId(m);

			s->setSampleProperty(id, v);
		}

		if (auto mainSelection = sampler->getSampleEditHandler()->getMainSelection())
		{
			if (m == EnvelopeType::GainMode || m == EnvelopeType::PanMode)
			{
				mainSelection->addEnvelopeProcessor(*waveform->getThumbnail());
			}			
		}

        setDisplayedEnvelope(m, shouldBeActive);
	}

	struct LambdaTableEditWithUndo : public UndoableAction
	{
		using TableFunction = std::function<bool(Table&)>;

		LambdaTableEditWithUndo(Table* t, const TableFunction& f_):
			UndoableAction(),
			table(t),
			b64(t->exportData()),
			f(f_)
		{
			
		}

		bool perform() override
		{
			if (table != nullptr)
			{
				Table::ScopedUpdateDelayer sds(*table);
				return f(*table);
			}
				

			return false;
		}

		bool undo() override
		{
			if (table != nullptr)
			{
				Table::ScopedUpdateDelayer sds(*table);
				table->restoreData(b64);
				return true;
			}
				
			return false;
		}

		TableFunction f;
		WeakReference<Table> table;
		String b64;
	};

	void applyToSelection(Modulation::Mode m, double leftValue, double rightValue, double gamma)
	{
		for (auto s : *sampler->getSampleEditHandler())
		{
			Range<double> normRange;
			auto length = (double)s->getReferenceToSound(0)->getLengthInSamples();
			auto sr = s->getSampleRate();

			auto msToNorm = [length, sr](double ms)
			{
				conversion_logic::ms2samples s2s;

				PrepareSpecs ps;
				ps.sampleRate = sr;
				s2s.prepare(ps);
				auto smp = s2s.getValue(ms);
				return smp / length;
			};

			normRange.setStart((double)s->getSampleProperty(SampleIds::SampleStart) / length);
			normRange.setEnd((double)s->getSampleProperty(SampleIds::SampleEnd) / length);

			switch (m)
			{
			case EnvelopeType::GainMode:
			{
				if (auto ge = s->getEnvelope(m))
				{
					auto l = msToNorm(leftValue);
					auto r = msToNorm(rightValue);

					auto f = [normRange, l, r, gamma](Table& t)
					{
						t.reset();
						t.setTablePoint(1, 1.0f, 0.0f, gamma);
						t.addTablePoint(normRange.getStart(), 0.0f);
						t.addTablePoint(normRange.getStart() + l, 0.5f, 1.0f - gamma);
						t.addTablePoint(normRange.getEnd() - r, 0.5f, gamma);
						t.addTablePoint(normRange.getEnd(), 0.0f, gamma);

						return true;
					};
					
					sampler->getUndoManager()->perform(new LambdaTableEditWithUndo(&ge->table, f));
				}
				break;
			}
			case EnvelopeType::PitchMode:
			{
				if (auto pe = s->getEnvelope(m))
				{
					PitchDetection detector;

					auto noteNumber = (int)s->getSampleProperty(SampleIds::Root);

					auto freq = MidiMessage::getMidiNoteInHertz(noteNumber);

					auto numSamples = detector.getNumSamplesNeeded(sr, freq);

					auto sensitivityNormed = jlimit(0.0f, 1.0f, (float)leftValue / 100.0f);
					auto numSamplesToUse = jmap<float>(sensitivityNormed, (float)numSamples, (float)numSamples * 6.0f);

					auto intensity = rightValue / 100.0;

					AudioSampleBuffer b(2, numSamplesToUse);

					ScopedPointer<AudioFormatReader> afr = s->createAudioReader(0);

					jassert(length == afr->lengthInSamples);
					int numToDo = (int)afr->lengthInSamples;

					conversion_logic::st2pitch s2p;
					conversion_logic::pitch2st p2s;

					Range<double> nonErrorRange(s2p.getValue(-1.0), s2p.getValue(1.0));
					Range<double> validRange(s2p.getValue(-1.0), s2p.getValue(1.0));

					int offset = 0;

					Array<Point<double>> list;
					list.ensureStorageAllocated(length / numSamplesToUse);

					auto lastValue = 1.0;

					while (numToDo > 0)
					{
						afr->read(&b, 0, numSamplesToUse, offset, true, true);

						auto pitch = detector.detectPitch(b, 0, numSamplesToUse, sr);

						auto pf = pitch / freq;

						if (!nonErrorRange.contains(pf))
						{
							pf = 0.4f + 0.6f * lastValue;
						}
						else
						{
							pf = validRange.clipValue(pf);
							pf = gamma * lastValue + (1.0 - gamma) * pf;
						}

						auto lastWasNotOne = std::abs(lastValue - 1.0) > 0.0001;

						lastValue = pf;
						
						if (lastWasNotOne)
						{
							auto x = (double)offset / (double)length;
							auto y = p2s.getValue(1.0 / pf);
							y *= 0.5;
							y += 0.5;

							y = 0.5 * (1.0 - intensity) + intensity * y;
							
							list.add({ x, y });
						}
						
						numToDo -= numSamplesToUse;
						offset += numSamplesToUse;
					}
					
					if (!list.isEmpty())
					{
						auto f = [list](Table& t)
						{
							t.reset();
							t.setTablePoint(0, 0.0f, 0.5f, 0.5f);
							t.setTablePoint(1, 0.0f, 0.5f, 0.5f);

							for (auto p : list)
							{
								if (p.y != 0.0 && p.y != 1.0)
									t.addTablePoint(p.x, p.y);
							}
							
							t.setTablePoint(0, 0.0f, t.getGraphPoint(1).y, 0.5f);

							return true;
						};

						sampler->getUndoManager()->perform(new LambdaTableEditWithUndo(&pe->table, f));
					}
				}
				break;
			}
			case EnvelopeType::PanMode:
			{
				if (auto fe = s->getEnvelope(m))
				{
					auto l = msToNorm(leftValue);
					auto r = ModulatorSamplerSound::EnvelopeTable::getFreqValueInverse(rightValue);
					
					auto f = [normRange, l, r, gamma](Table& t)
					{
						t.reset();
						t.setTablePoint(0, 0.0f, 1.0f, 0.5f);
						t.setTablePoint(1, 1.0, r, gamma);
						t.addTablePoint(normRange.getStart(), 1.0f);
						t.addTablePoint(normRange.getStart() + l, 1.0f, gamma);
						t.addTablePoint(normRange.getEnd(), r, gamma);
						return true;
					};

					sampler->getUndoManager()->perform(new LambdaTableEditWithUndo(&fe->table, f));
				}
				break;
			}
			default:
				break;
			}
		}
	}

	EnvelopePopup(ModulatorSampler* sampler_, SamplerDisplayWithTimeline* display_, SamplerSoundWaveform* waveform_):
		sampler(sampler_),
		waveform(waveform_),
		display(display_),
		gain(Modulation::Mode::GainMode),
		pitch(Modulation::Mode::PitchMode),
		filter(Modulation::Mode::PanMode)
	{
		addAndMakeVisible(gain);
		addAndMakeVisible(pitch);
		addAndMakeVisible(filter);

		setSize(500, 45 * 3 + 10);
		setName("Sample Envelope Editor");

		sampler->getSampleEditHandler()->selectionBroadcaster.addListener(*this, mainSelectionChanged);
		
		
        sampler->getSampleEditHandler()->toolBroadcaster.broadcaster.addListener(*this, toolChanged);

		grabKeyboardFocusAsync();
		setWantsKeyboardFocus(false);
		setFocusContainerType(FocusContainerType::keyboardFocusContainer);
	}

	std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override
	{
		return std::make_unique<SampleEditHandler::SubEditorTraverser>(display);
	}
    
    static void toolChanged(EnvelopePopup& p, SamplerTools::Mode m)
    {
        p.gain.showButton.setToggleStateAndUpdateIcon(m == SamplerTools::Mode::GainEnvelope);
        p.pitch.showButton.setToggleStateAndUpdateIcon(m == SamplerTools::Mode::PitchEnvelope);
        p.filter.showButton.setToggleStateAndUpdateIcon(m == SamplerTools::Mode::FilterEnvelope);
    }

	static void mainSelectionChanged(EnvelopePopup& p, ModulatorSamplerSound::Ptr s, int index)
	{
		auto ok = s != nullptr;

		p.gain.powerButton.setToggleStateAndUpdateIcon(ok && s->getEnvelope(EnvelopeType::GainMode));
		p.pitch.powerButton.setToggleStateAndUpdateIcon(ok && s->getEnvelope(EnvelopeType::PitchMode));
		p.filter.powerButton.setToggleStateAndUpdateIcon(ok && s->getEnvelope(EnvelopeType::PanMode));
	}

	void resized() override
	{
		auto b = getLocalBounds();
		auto h = (b.getHeight()-10) / 3;
		gain.setBounds(b.removeFromTop(h));
		b.removeFromTop(5);
		pitch.setBounds(b.removeFromTop(h));
		b.removeFromTop(5);
		filter.setBounds(b.removeFromTop(h));
	}

	WeakReference<SamplerDisplayWithTimeline> display;
	WeakReference<SamplerSoundWaveform> waveform;
	WeakReference<ModulatorSampler> sampler;

	Row gain, pitch, filter;

	JUCE_DECLARE_WEAK_REFERENCEABLE(EnvelopePopup);
};

struct VerticalZoomer : public Component,
						public Slider::Listener,
					    public SettableTooltipClient,
						public SampleMap::Listener
						
{
	VerticalZoomer(SamplerSoundWaveform* waveform, ModulatorSampler* s):
		display(waveform),
		sampler(s)
	{
		sampler->getSampleMap()->addListener(this);
		sampler->getSampleEditHandler()->allSelectionBroadcaster.addListener(*this, soundSelectionChanged);

		addAndMakeVisible(zoomSlider);
		zoomSlider.setRange(1.0, 16.0);
		zoomSlider.setSliderStyle(Slider::LinearBarVertical);
		zoomSlider.addListener(this);
		display->addMouseListener(this, true);

		setTooltip("Use the mousewheel to change display gain");
	}

	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d) override
	{
		if(e.eventComponent == this)
			zoomSlider.mouseWheelMove(e, d);
	}

	static void soundSelectionChanged(VerticalZoomer& z, int numSelected)
	{
		z.repaint();
	}

	virtual void sampleMapWasChanged(PoolReference) {};

	virtual void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue)
	{
		if (s == display->getCurrentSound())
		{
			if (id == SampleIds::Normalized || id == SampleIds::Volume)
			{
				display->refresh(sendNotificationSync);
				repaint();
			}
		}
	};

	~VerticalZoomer()
	{
		sampler->getSampleMap()->removeListener(this);
	}

	void sliderValueChanged(Slider* s) override
	{
		display->setVerticalZoom(s->getValue());
		repaint();
	};

	void drawLevels(Graphics& g, Rectangle<float> area, float gain)
	{
		auto invGain = 1.0f / gain;
		auto invGainHalf = invGain / 2.0f;

		auto db = String(roundToInt(Decibels::gainToDecibels(invGain)));
		auto dbHalf = String(roundToInt(Decibels::gainToDecibels(invGainHalf)));
		auto silence = "-dB";
		
		g.setFont(GLOBAL_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText(db, area, Justification::topRight);
		g.drawText(db, area, Justification::bottomRight);

		area = area.withSizeKeepingCentre(area.getWidth(), area.getHeight() / 2.0f + 10.0f);

		g.drawText(dbHalf, area, Justification::topRight);
		g.drawText(dbHalf, area, Justification::bottomRight);

		g.drawText(silence, area, Justification::centredRight);
	}

	void paint(Graphics& g) override
	{
		if (auto s = display->getCurrentSound())
		{
			auto isStereo = s->getReferenceToSound(0)->isStereo();

			auto gain = display->getCurrentSampleGain();

			auto b = getLocalBounds().toFloat();

			if (isStereo)
			{
				drawLevels(g, b.removeFromTop(b.getHeight() / 2.0f), gain);
				drawLevels(g, b, gain);
			}
			else
			{
				drawLevels(g, b, gain);
			}
		}
	}

	void resized() override
	{
		zoomSlider.setBounds(getLocalBounds().removeFromLeft(8));
        zoomSlider.setVisible(false);
	}

	WeakReference<ModulatorSampler> sampler;
	SamplerSoundWaveform* display;
	Slider zoomSlider;

	JUCE_DECLARE_WEAK_REFERENCEABLE(VerticalZoomer);
};



//==============================================================================
SampleEditor::SampleEditor (ModulatorSampler *s, SamplerBody *b):
	SamplerSubEditor(s->getSampleEditHandler()),
	sampler(s)
{
    addAndMakeVisible (viewport = new Viewport ("new viewport"));
    viewport->setScrollBarsShown (false, true);

    addAndMakeVisible (volumeSetter = new ValueSettingComponent(s));
    addAndMakeVisible (pitchSetter = new ValueSettingComponent(s));
    addAndMakeVisible (sampleStartSetter = new ValueSettingComponent(s));
    addAndMakeVisible (sampleEndSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopStartSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopEndSetter = new ValueSettingComponent(s));
    addAndMakeVisible (loopCrossfadeSetter = new ValueSettingComponent(s));
    addAndMakeVisible (startModulationSetter = new ValueSettingComponent(s));
    
	addAndMakeVisible(overview);

    addAndMakeVisible(spectrumSlider);
    spectrumSlider.setRange(-1.0, 1.0, 0.0);
    spectrumSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    spectrumSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
	spectrumSlider.setLookAndFeel(&slaf);
	spectrumSlider.setValue(-1.0f, dontSendNotification);

	spectrumSlider.onValueChange = [this]()
    {
        auto nAlpha = spectrumSlider.getValue() * 0.5f + 0.5f;
		auto sAlpha = nAlpha;
        auto wAlpha = sAlpha;
        
        sAlpha = scriptnode::faders::overlap().getFadeValue<0>(2, sAlpha);
        wAlpha = scriptnode::faders::overlap().getFadeValue<1>(2, wAlpha);
        
		spectrumSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(wAlpha).withAlpha(0.5f));

        currentWaveForm->getThumbnail()->setSpectrumAndWaveformAlpha(sAlpha, wAlpha);
    };
	spectrumSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
	spectrumSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.0f).withAlpha(0.5f));

	spectrumSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
	spectrumSlider.setTooltip("Blend spectrogram over waveform");



    addAndMakeVisible (panSetter = new ValueSettingComponent(s));

    viewContent = new SamplerDisplayWithTimeline(sampler);

	viewContent->addAndMakeVisible(currentWaveForm = new SamplerSoundWaveform(sampler));
    currentWaveForm->setIsSamplerWorkspacePreview();
    
	viewport->setViewedComponent(viewContent, false);
	viewport->setScrollBarThickness(13);

	fader.addScrollBarToAnimate(viewport->getHorizontalScrollBar());
	viewport->getHorizontalScrollBar().setLookAndFeel(&laf);
	viewport->getHorizontalScrollBar().addListener(this);

	addAndMakeVisible(verticalZoomer = new VerticalZoomer(currentWaveForm, sampler));

	currentWaveForm->setVisible(true);

	zoomFactor = 1.0f;

	body = b;

	addButton(SampleMapCommands::ZoomIn, false);
	addButton(SampleMapCommands::ZoomOut, false);
    addButton(SampleMapCommands::SelectWithMidi, true);
	addButton(SampleMapCommands::ApplyToMainOnly, true);
	addButton(SampleMapCommands::PreviewCurrentSound, true);

    analyseButton = addButton(SampleMapCommands::Analyser, true);
    
	addButton(SampleMapCommands::EnablePlayArea, true);
	addButton(SampleMapCommands::EnableSampleStartArea, true);
	addButton(SampleMapCommands::EnableLoopArea, true);
	envelopeButton = dynamic_cast<HiseShapeButton*>(addButton(SampleMapCommands::ShowEnvelopePopup, false));
	scriptButton = addButton(SampleMapCommands::ShowScriptPopup, false);
	addButton(SampleMapCommands::ToggleFirstScriptButton, true);

    addButton(SampleMapCommands::ZeroCrossing, false);
    improveButton = addButton(SampleMapCommands::ImproveLoopPoints, false);
    
    externalButton = addButton(SampleMapCommands::ExternalEditor, false);
    
    addButton(SampleMapCommands::NormalizeVolume, true);
    addButton(SampleMapCommands::LoopEnabled, true);
	
    
    
    addAndMakeVisible(sampleSelector = new ComboBox());
    addAndMakeVisible(multimicSelector = new ComboBox());
    
    sampleSelector->setLookAndFeel(&claf);
    multimicSelector->setLookAndFeel(&claf);
    
    sampleSelector->addListener(this);
    multimicSelector->addListener(this);
    
    claf.setDefaultColours(*sampleSelector);
    claf.setDefaultColours(*multimicSelector);
    
	panSetter->setPropertyType(SampleIds::Pan);
	volumeSetter->setPropertyType(SampleIds::Volume);
	pitchSetter->setPropertyType(SampleIds::Pitch);
	sampleStartSetter->setPropertyType(SampleIds::SampleStart);
	sampleEndSetter->setPropertyType(SampleIds::SampleEnd);
	startModulationSetter->setPropertyType(SampleIds::SampleStartMod);
	loopStartSetter->setPropertyType(SampleIds::LoopStart);
	loopEndSetter->setPropertyType(SampleIds::LoopEnd);
	loopCrossfadeSetter->setPropertyType(SampleIds::LoopXFade);

	auto lc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::LoopArea);
	auto xc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::LoopCrossfadeArea);
	auto sc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::SampleStartArea);

	loopStartSetter->setLabelColour(lc, Colours::white);
	loopEndSetter->setLabelColour(lc, Colours::white);
	loopCrossfadeSetter->setLabelColour(xc, Colours::white);
	startModulationSetter->setLabelColour(sc, Colours::white);

	sampler->getSampleMap()->addListener(this);

    //[/UserPreSize]

    setSize (800, 250);

    //[Constructor] You can add your own custom stuff here..

	sampleStartSetter->addChangeListener(this);
    sampleEndSetter->addChangeListener(this);
    loopStartSetter->addChangeListener(this);
    loopEndSetter->addChangeListener(this);
    loopCrossfadeSetter->addChangeListener(this);
    startModulationSetter->addChangeListener(this);

	currentWaveForm->addAreaListener(this);
    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xff1d1d1d));
    currentWaveForm->setColour(AudioDisplayComponent::ColourIds::outlineColour, Colour(0xff1d1d1d));
    
	handler->selectionBroadcaster.addListener(*this, mainSelectionChanged);
	handler->selectionBroadcaster.addListener(overview, DraggableThumbnail::mainSoundSelected);
	handler->selectionBroadcaster.addListener(*dynamic_cast<SamplerDisplayWithTimeline*>(viewContent.get()),
	[this](SamplerDisplayWithTimeline& tl, ModulatorSamplerSound::Ptr newSound, int m)
	{
		if(newSound != nullptr)
			newSound->addEnvelopeProcessor(*currentWaveForm->getThumbnail());

		tl.setEnvelope(tl.envelope, newSound.get(), tl.envelope != Modulation::Mode::numModes);
	});

    handler->toolBroadcaster.broadcaster.addListener(*currentWaveForm, [](SamplerSoundWaveform& wf, SamplerTools::Mode m)
    {
        switch(m)
        {
            case SamplerTools::Mode::PlayArea: wf.setClickArea(SamplerSoundWaveform::PlayArea); break;
            case SamplerTools::Mode::LoopArea: wf.setClickArea(SamplerSoundWaveform::LoopArea); break;
            case SamplerTools::Mode::LoopCrossfadeArea: wf.setClickArea(SamplerSoundWaveform::LoopCrossfadeArea); break;
            case SamplerTools::Mode::SampleStartArea: wf.setClickArea(SamplerSoundWaveform::SampleStartArea); break;
            default:    wf.setClickArea(SamplerSoundWaveform::AreaTypes::numAreas); break;
        }
    });
                                         
    handler->toolBroadcaster.broadcaster.addListener(*dynamic_cast<SamplerDisplayWithTimeline*>(viewContent.get()), [&](SamplerDisplayWithTimeline& d, SamplerTools::Mode m)
    {
        switch(m)
        {
            case SamplerTools::Mode::GainEnvelope:
                d.setEnvelope(Modulation::Mode::GainMode, handler->getMainSelection().get(), true);
                break;
            case SamplerTools::Mode::PitchEnvelope:
                d.setEnvelope(Modulation::Mode::PitchMode, handler->getMainSelection().get(), true);
                break;
            case SamplerTools::Mode::FilterEnvelope:
                d.setEnvelope(Modulation::Mode::PanMode, handler->getMainSelection().get(), true);
                break;
            default:
                d.setEnvelope(Modulation::Mode::numModes, nullptr, true);
                break;
        }
    });
    
	setFocusContainerType(FocusContainerType::keyboardFocusContainer);
	setWantsKeyboardFocus(true);
	addKeyListener(handler);

    startTimer(60);

	loadEditorSettings();

    //[/Constructor]
}

String SampleEditor::getNameForCommand(SampleMapCommands c, bool on)
{
    switch(c)
    {
        case SampleMapCommands::ZoomIn: return on ? "zoom-in" : "";
        case SampleMapCommands::ZoomOut: return on ? "zoom-out" : "";
        case SampleMapCommands::Analyser: return on ? "analyse" : "";
        case SampleMapCommands::SelectWithMidi: return on ?"select-midi" : "select-mouse";
        case SampleMapCommands::EnablePlayArea: return on ? "play-area" : "";
        case SampleMapCommands::EnableSampleStartArea: return on ? "samplestart-area" : "";
        case SampleMapCommands::EnableLoopArea: return on ? "loop-area" : "";
        case SampleMapCommands::NormalizeVolume: return on ? "normalise-on" : "normalise-off";
        case SampleMapCommands::LoopEnabled: return on ? "loop-on" : "loop-on";
        case SampleMapCommands::ExternalEditor: return on ? "external" : "";
        case SampleMapCommands::ZeroCrossing: return on ? "zero" : "";
		case SampleMapCommands::ImproveLoopPoints: return on ? "improve-loop" : "";
		case SampleMapCommands::ApplyToMainOnly: return on ? "main-only" : "";
		case SampleMapCommands::PreviewCurrentSound: return on ? "preview" : "";
		case SampleMapCommands::ShowEnvelopePopup: return on ? "envelope" : "";
		case SampleMapCommands::ShowScriptPopup: return on ? "script-popup" : "";
		case SampleMapCommands::ToggleFirstScriptButton: return on ? "toggle-first" : "";
        default: return "";
    }
}

SampleEditor::SampleMapCommands SampleEditor::getCommandIdForName(const String& n)
{
    for(int i = 0; i < (int)SampleMapCommands::numCommands; i++)
    {
        if(getNameForCommand((SampleMapCommands)i) == n)
            return (SampleMapCommands)i;
    }
    
    return {};
}

String SampleEditor::getTooltipForCommand(SampleMapCommands c)
{
    switch(c)
    {
        case SampleMapCommands::ZoomIn:                 return "Zoom in the waveform";
        case SampleMapCommands::ZoomOut:                return "Zoom out the waveform";
        case SampleMapCommands::Analyser:               return "Edit spectrogram properties";
        case SampleMapCommands::SelectWithMidi:         return "Enable MIDI selection";
        case SampleMapCommands::EnablePlayArea:         return "Enable Sample range editing";
        case SampleMapCommands::EnableSampleStartArea:  return "Enable SampleStartMod editing";
        case SampleMapCommands::EnableLoopArea:         return "Enable loop range editing";
        case SampleMapCommands::NormalizeVolume:        return "Normalise selected samples";
        case SampleMapCommands::LoopEnabled:            return "Enable looping for selection";
		case SampleMapCommands::PreviewCurrentSound:    return "Preview the current sound";
        case SampleMapCommands::ExternalEditor:         return "Open current sample selection in external audio editor";
        case SampleMapCommands::ZeroCrossing:           return "Enable zero crossing";
		case SampleMapCommands::ApplyToMainOnly:		return "Enable single selection cycling with tab key";
		case SampleMapCommands::ImproveLoopPoints:		return "Open the Loop Finder Popup";
		case SampleMapCommands::ShowEnvelopePopup:		return "Show the gain / pitch / filter envelope";
		case SampleMapCommands::ShowScriptPopup:		return "Show the interface of the first script processor";
		case SampleMapCommands::ToggleFirstScriptButton: return "Toggle the first button of the first script processor (F9)";
            
        default: return "";
    }
}

bool SampleEditor::getState(SampleMapCommands c) const
{
    bool isSelected = !selection.isEmpty();
    
    switch(c)
    {
        case SampleMapCommands::ZoomIn:         return false;
        case SampleMapCommands::ZoomOut:        return false;
        case SampleMapCommands::Analyser:       return false;
        case SampleMapCommands::SelectWithMidi: return sampler->getEditorState(ModulatorSampler::MidiSelectActive);
        case SampleMapCommands::EnablePlayArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::PlayArea;
        case SampleMapCommands::EnableSampleStartArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::SampleStartArea;
        case SampleMapCommands::EnableLoopArea: return currentWaveForm->currentClickArea == SamplerSoundWaveform::AreaTypes::LoopArea;
        case SampleMapCommands::NormalizeVolume: return isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::Normalized);
        case SampleMapCommands::LoopEnabled:    return isSelected && (int)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled);
		case SampleMapCommands::ApplyToMainOnly: return handler->applyToMainSelection;
		case SampleMapCommands::PreviewCurrentSound:
			return handler->getPreviewer().isPlaying();
        case SampleMapCommands::ExternalEditor: return false;
        case SampleMapCommands::ZeroCrossing:   return currentWaveForm->zeroCrossing;
		case SampleMapCommands::ImproveLoopPoints: return false;
		case SampleMapCommands::ShowEnvelopePopup: return false;
		case SampleMapCommands::ShowScriptPopup:   return false;
		case SampleMapCommands::ToggleFirstScriptButton:
		{
			if (auto jsp = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(sampler->getChildProcessor(ModulatorSynth::MidiProcessor)))
			{
				return jsp->getAttribute(0) > 0.5f;
			}

			return false;
		}
        default: return false;
    }
}

struct LoopImproveWindow: public Component,
                          public ControlledObject,
                          public ComboBox::Listener,
                          public ButtonListener,
                          public SampleMap::Listener,
                          public PathFactory
{
    int getDisplaySize() const { return amountSlider.getValue(); }
    
    LoopImproveWindow(ModulatorSamplerSound* s, ModulatorSampler* sampler_):
      ControlledObject(sampler_->getMainController()),
      sound(s),
      sampler(sampler_),
      applyButton("apply", this, *this),
      previewButton("preview", this, *this),
      findButton("find", this, *this),
      dragger(this, nullptr),
	  findThread(*this),
	  crossfadeUpdater(*this)
    {
        addAndMakeVisible(dragger);
        
        addAndMakeVisible(loopStart);
        addAndMakeVisible(loopEnd);
        addAndMakeVisible(&amountSlider);
        
        addAndMakeVisible(applyButton);
        addAndMakeVisible(findButton);
        addAndMakeVisible(previewButton);
        
        
        previewButton.setToggleModeWithColourChange(true);
        
        sampler->getSampleMap()->addListener(this);
        
        addAndMakeVisible(sizeSelector);
        sizeSelector.setLookAndFeel(&claf);
        GlobalHiseLookAndFeel::setDefaultColours(sizeSelector);
        sizeSelector.addItemList({"256", "512", "1024", "2048", "4096", "8192", "16384" }, 1);
        sizeSelector.setText("1024", dontSendNotification);
        sizeSelector.addListener(this);
        


        amountSlider.setRange(256, 8192, 1.0);
        amountSlider.setSkewFactor(0.3f);
        amountSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        amountSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
        amountSlider.setLookAndFeel(&laf);
        amountSlider.setValue(1024.0, dontSendNotification);
        amountSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
        amountSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.0f).withAlpha(0.5f));
        amountSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
        amountSlider.setTooltip("Change waveform zoom");
        
		amountSlider.onValueChange = [this]()
		{
			this->refreshThumbnails();
		};

		

		crossfadeGammaSlider.setRange(0.125, 8, 0.01);
		crossfadeGammaSlider.setSkewFactorFromMidPoint(1.0);
		crossfadeGammaSlider.setDoubleClickReturnValue(true, 1.0);
		crossfadeGammaSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
		crossfadeGammaSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 80, 20);
		crossfadeGammaSlider.setLookAndFeel(&laf);
		crossfadeGammaSlider.setValue(sampler->getSampleMap()->getValueTree()["CrossfadeGamma"], dontSendNotification);
		crossfadeGammaSlider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
		crossfadeGammaSlider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.0f).withAlpha(0.5f));
		crossfadeGammaSlider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));
		crossfadeGammaSlider.setTooltip("Change the X-Crossfade gamma value for this sample map");

		crossfadeGammaSlider.onValueChange = [this]()
		{
			crossfadeUpdater.startTimer(800);
		};

		addAndMakeVisible(crossfadeGammaSlider);

        previewButton.setTooltip("Preview the current loop");
        applyButton.setTooltip("Write the current loop to the sample properties");
        sizeSelector.setTooltip("Select a zoom size");
        findButton.setTooltip("Find the best loop point");
        
        loopStart.setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
        loopEnd.setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
        
        loopStart.setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
        loopStart.setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
        loopStart.setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::red.withSaturation(0.43f).withAlpha(0.6f));
        loopEnd.setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
        loopEnd.setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
        loopEnd.setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::blue.withSaturation(0.43f).withAlpha(0.6f));
        
        setName("Loop Finder");
        setSize(600, 300);
        
        selectionChanged(*this, sound, 0);
    }
    
    ~LoopImproveWindow()
    {
        sampler->getSampleMap()->removeListener(this);
    }
    
    void mouseDown(const MouseEvent& e)
    {
        mouseDownLoop = currentLoop;
        dragStart = e.getPosition().getX() > getWidth() / 2;
    }
    
    void mouseDrag(const MouseEvent& e)
    {
        auto ds = (float)getDisplaySize();
        
        auto deltaNorm = (float)e.getDistanceFromDragStartX() / (float)getWidth();
        
        if(e.mods.isShiftDown())
            deltaNorm *= 0.25f;
        
        auto delta = roundToInt(deltaNorm * ds);
        
        if(dragStart)
            currentLoop.setStart(mouseDownLoop.getStart() - delta);
        else
            currentLoop.setEnd(mouseDownLoop.getEnd() - delta);
        
		unappliedChanges = true;

        refreshThumbnails();
    }
    
    Path createPath(const String& url) const override
    {
        Path p;
        LOAD_EPATH_IF_URL("preview", LoopIcons::preview);
        LOAD_EPATH_IF_URL("apply", LoopIcons::apply);
        LOAD_EPATH_IF_URL("find", LoopIcons::find);
        
        return p;
    }
    
    void comboBoxChanged(ComboBox* cb) override
    {
        amountSlider.setValue(cb->getText().getIntValue(), sendNotificationSync);
    }
    
	Range<int> getLoopEdgeRange(bool getEnd)
	{
		auto DisplaySize = getDisplaySize();
		auto os = jmax(0, currentLoop.getStart() - DisplaySize / 2);
		auto nums = jmin(DisplaySize, fullBuffer.getNumSamples() - os);

		auto oe = jmax(0, currentLoop.getEnd() - DisplaySize / 2);
		auto nume = jmin(DisplaySize, fullBuffer.getNumSamples() - oe);

		if(getEnd)
			return { oe, oe + nume };
		else
			return { os, os + nums };
	}

	var getLoopEdgesAsChannelBuffers(Range<int> startRange = {}, Range<int> endRange = {})
	{
		if (sound == nullptr || !sound->getSampleProperty(SampleIds::LoopEnabled))
		{
			ok = false;
			return {};
		}

		ok = true;

		auto DisplaySize = getDisplaySize();

		VariantBuffer::Ptr ls = new VariantBuffer(DisplaySize);
		VariantBuffer::Ptr rs = new VariantBuffer(DisplaySize);

		VariantBuffer::Ptr le = new VariantBuffer(DisplaySize);
		VariantBuffer::Ptr re = new VariantBuffer(DisplaySize);

		if(startRange.isEmpty())
			startRange = getLoopEdgeRange(false);

		if(endRange.isEmpty())
			endRange = getLoopEdgeRange(true);

		auto copyTo = [&](VariantBuffer::Ptr dst, int channel, Range<int> r)
		{
			FloatVectorOperations::copy(dst->buffer.getWritePointer(0), fullBuffer.getReadPointer(channel, r.getStart()), r.getLength());
		};

		copyTo(ls, 0, startRange);
		copyTo(rs, 1, startRange);
		copyTo(le, 0, endRange);
		copyTo(re, 1, endRange);

		Array<var> channels;
		channels.add(var(ls.get()));
		channels.add(var(rs.get()));
		channels.add(var(le.get()));
		channels.add(var(re.get()));

		return channels;
	}

	struct FindThread : public Thread
	{
		FindThread(LoopImproveWindow& w) :
			Thread("Find best loop"),
			parent(w)
		{};

		void run() override
		{
			auto DisplaySize = parent.getDisplaySize();

			auto endRange = parent.getLoopEdgeRange(true);
			auto startRange = parent.getLoopEdgeRange(false);

			Array<ErrorStats> allStats;

			allStats.ensureStorageAllocated(DisplaySize);

			{
				ScopedLock sl(parent.fullBufferLock);

				for (int i = 0; i < DisplaySize; i++)
				{
					if (threadShouldExit())
						return;

					auto delta = -1 * DisplaySize / 2 + i;
					ErrorStats thisStats;
					thisStats.loopRange = parent.currentLoop;
					thisStats.loopRange.setStart(thisStats.loopRange.getStart() + delta);
					auto thisStart = startRange + delta;
					thisStats.calculate(parent.fullBuffer, thisStart, endRange, parent.getDisplaySize());
					allStats.add(thisStats);
				}
			}
			

			ErrorStats::Comparator comparator;
			allStats.sort(comparator);

			auto best = allStats.getFirst();

			auto r = best.loopRange;

			if (!threadShouldExit() && !r.isEmpty())
			{
				WeakReference<LoopImproveWindow> p = &parent;

				MessageManager::callAsync([r, p]()
				{
					p->currentLoop = r;
					p->unappliedChanges = true;
					p->refreshThumbnails();
					p->repaint();
				});
			}
		}

		LoopImproveWindow& parent;
	} findThread;

	struct ErrorStats
	{
		String toString() const
		{
			String s;
			s << "Average Diff: " << String(maxdiff, 1) << "dB, ";
			s << "Error: " << String(error, 1) << "dB";
			return s;
		}

		struct Comparator
		{
			static int compareElements(const ErrorStats& first, const ErrorStats& second)
			{
				auto firstScore = first.getScore();
				auto secondScore = second.getScore();

				if (firstScore == secondScore)
					return 0;
				else if (firstScore < secondScore)
					return 1;
				else
					return -1;
			}
		};

		float getScore() const
		{
			return -1.0f * error + -1.0f * maxdiff * 2.0f;
		}

		void calculate(AudioSampleBuffer& fullBuffer, Range<int> startRange, Range<int> endRange, int DisplaySize)
		{
			float diff[2] = { 0.0f };

			auto getSample = [&](bool getEnd, bool getRight, int sampleIndex)
			{
				auto index = (getEnd ? endRange.getStart() : startRange.getStart()) + sampleIndex;

                if(isPositiveAndBelow(index, fullBuffer.getNumSamples()))
                    return fullBuffer.getSample((int)getRight, index);
                
                return 0.0f;
			};

			for (int i = DisplaySize / 4; i < DisplaySize * 3 / 4; i++)
			{
				float window = (float)(i - DisplaySize / 4) / (float)(DisplaySize / 4);

				if (i > DisplaySize / 2)
					window = 2.0f - window;

				diff[0] += window * std::abs(getSample(false, false, i) - getSample(true, false, i));
				diff[1] += window * std::abs(getSample(false, true, i) - getSample(true, true, i));
			}

			auto p2le = getSample(true, false, DisplaySize / 2 - 2);
			auto p1le = getSample(true, false, DisplaySize / 2 - 1);
			auto p2re = getSample(true, true, DisplaySize / 2 - 2);
			auto p1re = getSample(true, true, DisplaySize / 2 - 1);

            auto p2ls = getSample(false, false, DisplaySize / 2 - 2);
            //auto p1ls = getSample(false, false, DisplaySize / 2 - 1);
            auto p2rs = getSample(false, true, DisplaySize / 2 - 2);
            //auto p1rs = getSample(false, true, DisplaySize / 2 - 1);
            
            auto n2le = getSample(true, false, DisplaySize / 2 + 2);
            auto n1le = getSample(true, false, DisplaySize / 2 + 1);
            auto n2re = getSample(true, true, DisplaySize / 2 + 2);
            auto n1re = getSample(true, true, DisplaySize / 2 + 1);

            auto n2ls = getSample(false, false, DisplaySize / 2 + 2);
            auto n1ls = getSample(false, false, DisplaySize / 2 + 1);
            auto n2rs = getSample(false, true, DisplaySize / 2 + 2);
            auto n1rs = getSample(false, true, DisplaySize / 2 + 1);
            
            auto errL = std::abs(p2le - p2ls) + std::abs(p1le - p2ls) +
                        std::abs(n1le - n1ls) + std::abs(n2le - n2ls);
            
            auto errR = std::abs(p2re - p2rs) + std::abs(p1re - p2rs) +
                        std::abs(n1re - n1rs) + std::abs(n2re - n2rs);
            
            
			auto expectedl = p1le + (p1le - p2le);
			auto expectedr = p1re + (p1re - p2re);

			auto actuall = getSample(false, false, DisplaySize / 2);
			auto actualr = getSample(false, true, DisplaySize / 2);

			error = jmax(std::abs(expectedl - actuall), std::abs(expectedr - actualr));
            
            error = jmax(errL, errR);
			error = Decibels::gainToDecibels(error);

			diff[0] /= (float)(DisplaySize / 4);
			diff[1] /= (float)(DisplaySize / 4);

			maxdiff = jmax(diff[0], diff[1]);
			maxdiff = Decibels::gainToDecibels(maxdiff);
		}

		Range<int> loopRange;
		float maxdiff = 0.0f;
		float error = 0.0f;
	};

	void refreshThumbnails()
	{
		ScopedLock sl(fullBufferLock);
		auto channels = getLoopEdgesAsChannelBuffers();

		if (channels.size() != 4)
		{
			loopStart.clear();
			loopEnd.clear();
			statistics = {};

			repaint();
			return;
		}

		auto startRange = getLoopEdgeRange(false);
		auto endRange = getLoopEdgeRange(true);

		auto DisplaySize = getDisplaySize();

		auto maxS = 1.0f / jmax(0.001f, fullBuffer.getMagnitude(startRange.getStart(), startRange.getLength()));
		auto maxE = 1.0f / jmax(0.001f, fullBuffer.getMagnitude(endRange.getStart(), endRange.getLength()));

		statistics.loopRange = currentLoop;
		statistics.calculate(fullBuffer, startRange, endRange, DisplaySize);

		auto gain = jmin(maxS, maxE);

        loopStart.setDisplayGain(gain, dontSendNotification);
        loopEnd.setDisplayGain(gain, dontSendNotification);
		loopStart.setBuffer(channels[0], channels[1]);
		loopEnd.setBuffer(channels[2], channels[3]);
        
        repaint();
    }
    
    void sampleMapWasChanged(PoolReference newSampleMap) {}

    void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) override
    {
        if(s == sound.get())
        {
            if(id == SampleIds::LoopStart || id == SampleIds::LoopEnd || id == SampleIds::LoopEnabled)
            {
                refreshThumbnailAndRanges();
            }
        }
    };
    
    static void selectionChanged(LoopImproveWindow& w, ModulatorSamplerSound::Ptr newSound, int micIndex)
    {
		
        w.sound = newSound;
        
        if(newSound == nullptr)
        {
            w.fullBuffer = {};
            w.refreshThumbnailAndRanges();
            return;
        }
        
        StreamingSamplerSound::Ptr ss = newSound->getReferenceToSound(micIndex);

        if (ss == nullptr)
        {
            jassertfalse;
            return;
        }

        ScopedPointer<AudioFormatReader> afr;

        if (ss->isMonolithic())
            afr = ss->createReaderForPreview();
        else
            afr = PresetHandler::getReaderForFile(ss->getFileName(true));

        if (afr != nullptr)
        {
			ScopedLock sl(w.fullBufferLock);
            w.fullBuffer.setSize(2, (int)afr->lengthInSamples);
            afr->read(&w.fullBuffer, 0, (int)afr->lengthInSamples, 0, true, true);
        }
        
        w.refreshThumbnailAndRanges();
    }
    
    void refreshThumbnailAndRanges()
    {
        if(sound != nullptr)
        {
            originalLoop = { (int)sound->getSampleProperty(SampleIds::LoopStart), (int)sound->getSampleProperty(SampleIds::LoopEnd)};
            currentLoop = originalLoop;
        }
        else
        {
            originalLoop = {};
            currentLoop = {};
        }
        
        refreshThumbnails();
    }
    
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d) override
    {
        amountSlider.mouseWheelMove(e, d);
    }
    
    void buttonClicked(Button* b) override
    {
        if(b == &previewButton && fullBuffer.getNumSamples() > 0)
        {
			int PreviewLength = 44100 * 2;

			AudioSampleBuffer b(2, PreviewLength);

			auto useMultipleLoops = currentLoop.getLength() < (PreviewLength / 2);

			if (useMultipleLoops)
			{
				for (int i = 0; i < PreviewLength; i += currentLoop.getLength())
				{
					auto numToCopy = jmin(currentLoop.getLength(), PreviewLength - i);
					b.copyFrom(0, i, fullBuffer, 0, currentLoop.getStart(), numToCopy);
					b.copyFrom(1, i, fullBuffer, 1, currentLoop.getStart(), numToCopy);
				}
			}
			else
			{
				auto o1 = currentLoop.getEnd() - (PreviewLength / 2);
				
				b.copyFrom(0, 0, fullBuffer, 0, o1, PreviewLength / 2);
				b.copyFrom(1, 0, fullBuffer, 1, o1, PreviewLength / 2);

				b.copyFrom(0, PreviewLength / 2, fullBuffer, 0, currentLoop.getStart(), PreviewLength / 2);
				b.copyFrom(1, PreviewLength / 2, fullBuffer, 1, currentLoop.getStart(), PreviewLength / 2);
			}

			b.applyGainRamp(0, 1024, 0.0f, 1.0f);
			b.applyGainRamp(PreviewLength - 1024, 1024, 1.0f, 0.0f);

            getMainController()->setBufferToPlay(b, sound->getSampleRate());
        }
        if(b == &findButton)
        {
			findThread.stopThread(1000);
			findThread.startThread();
			repaint();
        }
        if(b == &applyButton && sound != nullptr)
        {
            sound->setSampleProperty(SampleIds::LoopStart, currentLoop.getStart(), true);
            sound->setSampleProperty(SampleIds::LoopEnd, currentLoop.getEnd(), true);
			unappliedChanges = false;
			repaint();
        }
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        auto top = b.removeFromTop(24);
        applyButton.setBounds(top.removeFromRight(24).reduced(2));
        
        previewButton.setBounds(top.removeFromRight(24).reduced(2));
        
        findButton.setBounds(top.removeFromRight(24).reduced(2));
        
        auto bottom = b.removeFromBottom(24);
        
        dragger.setBounds(bottom.removeFromRight(15).removeFromBottom(15));
        
        sizeSelector.setBounds(bottom.removeFromLeft(150));
        
        amountSlider.setBounds(bottom);
        
        b.removeFromBottom(10);
        
        loopStart.setBounds(b);//.removeFromTop(b.getHeight()/2));
        loopEnd.setBounds(b);
        
        loopStart.setRange(getWidth()/2, getWidth());
        loopEnd.setRange(0, getWidth() / 2);
		crossfadeGammaSlider.setBounds(top.removeFromLeft(190));
    }
    
    void paint(Graphics& g) override
    {
		if (findThread.isThreadRunning())
		{
			g.setColour(Colours::white.withAlpha(0.6f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Searching best loop point in area", getLocalBounds().toFloat(), Justification::centred);
			return;
		}

        auto b = getLocalBounds().toFloat();
        
        b.removeFromTop(24);
        
        
        
        g.setColour(Colours::white.withAlpha(0.2f));
        g.drawVerticalLine(getWidth() / 2, 28, getHeight() - 32.0f);
        
        auto lb = loopStart.getBounds();
        
        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(lb.toFloat(), 3.0f);
        
        g.drawHorizontalLine(lb.getY() + lb.getHeight() / 4, 0.0f, (float)getWidth());
        g.drawHorizontalLine(lb.getY() + 3 * lb.getHeight() / 4, 0.0f, (float)getWidth());
        
        g.setFont(GLOBAL_BOLD_FONT());
        g.setColour(Colours::white.withAlpha(0.9f));
        
        if(!ok)
        {
			if(sound == nullptr)
				g.drawText("No sample selected", getLocalBounds().toFloat(), Justification::centred);
			else
				g.drawText("Loop is disabled for current sample", getLocalBounds().toFloat(), Justification::centred);
            return;
        }
        
		String s;
		s << statistics.toString();
		s << (unappliedChanges ? "*" : "");

		auto t = getLocalBounds().toFloat().removeFromTop(24.0f);
		t.setRight(findButton.getX() - 10);

		t.removeFromLeft(crossfadeGammaSlider.getWidth() + 10);

        g.drawText(s, t.reduced(10.0f, 0.0f), Justification::centredRight);
    }
    
	struct CrossfadeUpdater : public Timer
	{
		CrossfadeUpdater(LoopImproveWindow& parent_) :
			parent(parent_)
		{};

		void timerCallback() override
		{
			parent.sampler->getSampleMap()->getValueTree().setProperty(Identifier("CrossfadeGamma"), 
																	   parent.crossfadeGammaSlider.getValue(), 
																	   parent.sampler->getUndoManager());

			stopTimer();
		}
		
		LoopImproveWindow& parent;
	} crossfadeUpdater;

    bool ok;
    
	ErrorStats statistics;

	CriticalSection fullBufferLock;
    
    AudioSampleBuffer fullBuffer;
    
    HiseAudioThumbnail loopStart;
    HiseAudioThumbnail loopEnd;
    
    HiseShapeButton applyButton;
    HiseShapeButton findButton;
    HiseShapeButton previewButton;
    
    Range<int> originalLoop;
    Range<int> currentLoop;
    Range<int> mouseDownLoop;
    bool dragStart = false;
	bool unappliedChanges = false;
    
    Slider amountSlider;
    ModulatorSamplerSound::Ptr sound;
    
	Slider crossfadeGammaSlider;

    ComboBox sizeSelector;
    GlobalHiseLookAndFeel claf;
    
    WeakReference<ModulatorSampler> sampler;
    
    LookAndFeel_V4 laf;
    
    juce::ResizableCornerComponent dragger;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(LoopImproveWindow);
};

void SampleEditor::perform(SampleMapCommands c)
{
	auto tl = dynamic_cast<SamplerDisplayWithTimeline*>(viewContent.get());

    switch(c)
    {
    case SampleMapCommands::NormalizeVolume:  SampleEditHandler::SampleEditingActions::normalizeSamples(handler, this); return;
    case SampleMapCommands::LoopEnabled:
    {
        for(int i = 0; i < selection.size(); i++)
        {
           selection[i]->toggleBoolProperty(SampleIds::LoopEnabled);
        };

        const bool isOn = (selection.size() != 0) ? (bool)selection.getLast()->getSampleProperty(SampleIds::LoopEnabled) : false;

        currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopArea)->setVisible(isOn);
        currentWaveForm->getSampleArea(SamplerSoundWaveform::LoopCrossfadeArea)->setVisible(isOn);
        return;
    }
    case SampleMapCommands::SelectWithMidi:   sampler->setEditorState(ModulatorSampler::MidiSelectActive, !sampler->getEditorState(ModulatorSampler::MidiSelectActive));
        return;
    case SampleMapCommands::EnableSampleStartArea:
        handler->toolBroadcaster.toggleMode(SamplerTools::Mode::SampleStartArea);
        return;
    case SampleMapCommands::EnableLoopArea:
        handler->toolBroadcaster.toggleMode(SamplerTools::Mode::LoopArea);
        return;
    case SampleMapCommands::EnablePlayArea:
        handler->toolBroadcaster.toggleMode(SamplerTools::Mode::PlayArea);
        return;
    case SampleMapCommands::ZeroCrossing:
        currentWaveForm->zeroCrossing = !currentWaveForm->zeroCrossing;
        return;
    case SampleMapCommands::ZoomIn:
        zoom(false);
        return;
    case SampleMapCommands::ZoomOut:
        zoom(true);
        return;
	case SampleMapCommands::ApplyToMainOnly: 
		handler->applyToMainSelection = !handler->applyToMainSelection;
		handler->selectionBroadcaster.resendLastMessage(sendNotificationAsync);
		return;
	case SampleMapCommands::PreviewCurrentSound:
	{
		handler->togglePreview();
		return;
	}
    case SampleMapCommands::Analyser:
    {
        auto n = new Spectrum2D::Parameters::Editor(currentWaveForm->getThumbnail()->getParameters());
        findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentAsDetachedPopup(n, analyseButton, {8, 16});
        return;
    }
	case SampleMapCommands::ImproveLoopPoints:
	{
        auto n = new LoopImproveWindow(const_cast<ModulatorSamplerSound*>(currentWaveForm->getCurrentSound()), sampler);
        
        handler->selectionBroadcaster.addListener(*n, LoopImproveWindow::selectionChanged);
        
        findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentAsDetachedPopup(n, improveButton, {8, 16});
        
        return;
	}
	case SampleMapCommands::ShowEnvelopePopup:
	{
		auto n = new EnvelopePopup(sampler, tl, currentWaveForm);
		findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentInRootPopup(n, envelopeButton, { 8, 16 });
		return;
	}
	case SampleMapCommands::ShowScriptPopup:
	{
		if (auto jsp = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(sampler))
		{
			auto n = new ScriptContentComponent(jsp);
			auto c = jsp->getContent();
			n->setSize(c->getContentWidth(), c->getContentHeight());
			n->setName(jsp->getId());
			findParentComponentOfClass<FloatingTile>()->getRootFloatingTile()->showComponentInRootPopup(n, scriptButton, { 8, 16 });
		}
		else
		{
			PresetHandler::showMessageWindow("No script processor", "You haven't added a script processor to this sampler", PresetHandler::IconType::Error);
		}

		return;
	}
	case SampleMapCommands::ToggleFirstScriptButton:
	{
		SampleEditHandler::SampleEditingActions::toggleFirstScriptButton(handler);
		return;
	}
    case SampleMapCommands::ExternalEditor:
    {
        if (sampler->getSampleMap()->isMonolith())
        {
            PresetHandler::showMessageWindow("Monolith file", "You can't edit a monolith file", PresetHandler::IconType::Error);
            return;
        }

        if (sampler->getSampleMap()->isUsingUnsavedValueTree())
        {
            PresetHandler::showMessageWindow("Unsaved samplemap", "You need to save your samplemap to a file before starting the external editor.  \n> This is required so that you can reload the map after editing");
            return;
        }

        auto fp = GET_HISE_SETTING(sampler, HiseSettings::Other::ExternalEditorPath).toString();

        if (fp.isEmpty())
        {
            PresetHandler::showMessageWindow("No external editor specified", "You need to set an audio editor you want to use for this operation.  \n> Settings -> Other -> ExternalEditorPath", PresetHandler::IconType::Error);

            return;
        }

        File editor(fp);

        String args;

        Array<File> editedFiles;

        for (auto s : selection)
        {
            for (int i = 0; i < s->getNumMultiMicSamples(); i++)
            {
                editedFiles.add(File(s->getReferenceToSound(i)->getFileName(true)));
                s->getReferenceToSound(i)->closeFileHandle();
            }
        }

#if JUCE_WINDOWS
        for(auto& f: editedFiles)
            args << "\"" << f.getFullPathName().replace(" ", "\\ ") << "\" ";
#else
        for(auto& f: editedFiles)
            args << f.getFullPathName().replace(" ", "\\ ") << " ";
#endif

        externalWatcher = new ExternalFileChangeWatcher(sampler, editedFiles);
        editor.startAsProcess(args);

        return;
    }
	default:
		return;
    }
    return;
}

std::unique_ptr<ComponentTraverser> SampleEditor::createKeyboardFocusTraverser()
{
	return std::make_unique<SampleEditHandler::SubEditorTraverser>(this);
}

SampleEditor::~SampleEditor()
{
	saveEditorSettings();

	if (sampler != nullptr)
	{
		sampler->getSampleMap()->removeListener(this);
	}
    

    viewport = nullptr;
    volumeSetter = nullptr;
    pitchSetter = nullptr;
    sampleStartSetter = nullptr;
    sampleEndSetter = nullptr;
    loopStartSetter = nullptr;
    loopEndSetter = nullptr;
    loopCrossfadeSetter = nullptr;
    startModulationSetter = nullptr;
    panSetter = nullptr;
}

void SampleEditor::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	auto s = scrollBarThatHasMoved->getCurrentRange();

	auto start = s.getStart();
	auto end = s.getEnd();

	start /= (double)jmax(1, currentWaveForm->getWidth());
	end /= (double)jmax(1, currentWaveForm->getWidth());

	overview.setRange(start * overview.getWidth(), end * overview.getWidth());
}

bool SampleEditor::isInWorkspace() const
{
#if USE_BACKEND
	return findParentComponentOfClass<ProcessorEditor>() == nullptr;
#else
	return false;
#endif
}

void SampleEditor::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& /*newValue*/)
{
	if (s == currentWaveForm->getCurrentSound() && SampleIds::Helpers::isAudioProperty(id))
	{
		if (id == SampleIds::SampleStartMod)
		{
			if (!getState(SampleMapCommands::EnableSampleStartArea))
				perform(SampleMapCommands::EnableSampleStartArea);
		}
		if (id == SampleIds::SampleStart || id == SampleIds::SampleEnd)
		{
			if (!getState(SampleMapCommands::EnablePlayArea))
				perform(SampleMapCommands::EnablePlayArea);
		}
		if (id == SampleIds::LoopEnabled || id == SampleIds::LoopEnd ||
			id == SampleIds::LoopStart)
		{
			if (!getState(SampleMapCommands::EnableLoopArea))
				perform(SampleMapCommands::EnableLoopArea);
		}

		currentWaveForm->updateRanges();
	}	
}


void SampleEditor::refreshDisplayFromComboBox()
{
	handler->cycleMainSelection(sampleSelector->getSelectedItemIndex(), multimicSelector->getSelectedItemIndex());

	

	auto idx = sampleSelector->getSelectedItemIndex();


	if (auto s = selection[idx])
	{
		auto micIndex = jlimit(0, sampler->getNumMicPositions()-1, multimicSelector->getSelectedItemIndex());
		handler->selectionBroadcaster.sendMessage(sendNotification, s, micIndex);

		
	}
}

//==============================================================================
void SampleEditor::paint (Graphics& g)
{
	auto b = getLocalBounds().removeFromTop(24);

    claf.drawFake3D(g, b);
}

void SampleEditor::resized()
{
	auto b = getLocalBounds();

	auto topBar = b.removeFromTop(24);
	
	for (auto b : menuButtons)
		b->setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(2));

    b = b.reduced(8);
    
    
	externalButton->setBounds(topBar.removeFromRight(topBar.getHeight()).reduced(3));

    multimicSelector->setBounds(topBar.removeFromRight(100));
    sampleSelector->setBounds(topBar.removeFromRight(300));
    
	topBar.removeFromRight(10);
	analyseButton->setBounds(topBar.removeFromRight(topBar.getHeight()).reduced(3));
    spectrumSlider.setBounds(topBar.removeFromRight(100).reduced(2));
	

	//toolbar->setBounds(topBar);

	b.removeFromTop(20);
	int setterHeight = 32;
	int width = 150;
    
    if(isInWorkspace())
        width = jmin(b.getWidth() / 8, 150);
    
	int halfWidth = width / 2;
	static constexpr int Margin = 5;

	if (isInWorkspace())
	{
		b.removeFromBottom(12);
		auto bottom = b.removeFromBottom(setterHeight);
		bottom = bottom.withSizeKeepingCentre(width * 8, bottom.getHeight());

		volumeSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		panSetter->setBounds(bottom.removeFromLeft(halfWidth - Margin));
		bottom.removeFromLeft(Margin);

		pitchSetter->setBounds(bottom.removeFromLeft(halfWidth - Margin));
		bottom.removeFromLeft(Margin);

		sampleStartSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		sampleEndSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		startModulationSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopStartSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopEndSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);

		loopCrossfadeSetter->setBounds(bottom.removeFromLeft(width - Margin));
		bottom.removeFromLeft(Margin);
	}
	else
	{
		auto bottom = b.removeFromBottom(2 * setterHeight + 10);

		bottom = bottom.withSizeKeepingCentre(width * 4, bottom.getHeight());

		auto r1 = bottom.removeFromTop(setterHeight);
		auto r2 = bottom.removeFromBottom(setterHeight);

		volumeSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		panSetter->setBounds(r1.removeFromLeft(halfWidth - Margin));
		r1.removeFromLeft(Margin);

		pitchSetter->setBounds(r1.removeFromLeft(halfWidth - Margin));
		r1.removeFromLeft(Margin);

		sampleEndSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		sampleStartSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);

		loopEndSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		loopStartSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);

		startModulationSetter->setBounds(r2.removeFromLeft(width - Margin));
		r2.removeFromLeft(Margin);

		loopCrossfadeSetter->setBounds(r1.removeFromLeft(width - Margin));
		r1.removeFromLeft(Margin);
	}

	b.removeFromBottom(12);

	if (isInWorkspace())
	{
		auto zb = b.removeFromLeft(18);
		b.removeFromRight(24);
		zb.removeFromBottom(viewport->getScrollBarThickness());
		
		zb.removeFromTop(32);
		zb.removeFromTop(SamplerDisplayWithTimeline::TimelineHeight);
		verticalZoomer->setBounds(zb);
		b.removeFromLeft(6);

		overview.setBounds(b.removeFromTop(32));

		b.removeFromTop(5);

	}
	else
	{
		verticalZoomer->setVisible(false);

		overview.setVisible(false);

	}

	viewport->setBounds(b);

	viewContent->setSize((int)(viewport->getWidth() * zoomFactor), viewport->getHeight() - (viewport->isHorizontalScrollBarShown() ? viewport->getScrollBarThickness() : 0));
}



Component* SampleEditor::addButton(SampleMapCommands commandId, bool hasState)
{
	SampleEditorToolbarFactory::Factory f;
    auto newButton = new HiseShapeButton(getNameForCommand(commandId, true), nullptr, f, getNameForCommand(commandId, false));

	if (hasState)
		newButton->setToggleModeWithColourChange(true);

    newButton->setTooltip(getTooltipForCommand(commandId));
	newButton->setClickingTogglesState(false);
	newButton->setWantsKeyboardFocus(false);

    newButton->onClick = [commandId, this]()
    {
        perform(commandId);
    };
    
	if (commandId == SampleMapCommands::EnableLoopArea)
	{
		auto lc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::LoopArea);

		newButton->onColour = lc.withMultipliedBrightness(1.5f);
		newButton->offColour = lc;
		newButton->refreshButtonColours();
	}

	if (commandId == SampleMapCommands::EnablePlayArea)
	{
		auto lc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::PlayArea);

		newButton->onColour = lc.withMultipliedBrightness(1.5f);
		newButton->offColour = lc.withBrightness(0.7f);
		newButton->refreshButtonColours();
	}

	if (commandId == SampleMapCommands::EnableSampleStartArea)
	{
		auto lc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::SampleStartArea);

		newButton->onColour = lc.withMultipliedBrightness(1.5f);
		newButton->offColour = lc;
		newButton->refreshButtonColours();
	}

	menuButtons.add(newButton);
	addAndMakeVisible(newButton);
	return newButton;
}


bool SampleEditor::keyPressed(const KeyPress& key)
{
	if (key == KeyPress('l', {}, 'l'))
	{
		perform(SampleMapCommands::EnableLoopArea);
		return true;
	}
	if (key == KeyPress('p', {}, 'p'))
	{
		perform(SampleMapCommands::EnablePlayArea);
		return true;
	}
	if (key == KeyPress('s', {}, 's'))
	{
		perform(SampleMapCommands::EnableSampleStartArea);
		return true;
	}
	if (key == KeyPress::spaceKey)
	{
		perform(SampleMapCommands::PreviewCurrentSound);
		return true;
	}

	return false;
}

void SampleEditor::saveEditorSettings()
{
	var d(new DynamicObject());

	auto obj = d.getDynamicObject();

	obj->setProperty("SpectrumSlider", spectrumSlider.getValue());
	obj->setProperty("ZeroCrossing", currentWaveForm->zeroCrossing);
	obj->setProperty("ClickArea", (int)currentWaveForm->currentClickArea);
	obj->setProperty("Envelope", (int)dynamic_cast<SamplerDisplayWithTimeline*>(viewContent.get())->envelope);

	auto sp = currentWaveForm->getThumbnail()->getParameters();
	sp->saveToJSON(d);

	getPropertyFile().replaceWithText(JSON::toString(d));

}

void SampleEditor::loadEditorSettings()
{
	auto v = JSON::parse(getPropertyFile());

	if (auto obj = v.getDynamicObject())
	{
		auto sp = currentWaveForm->getThumbnail()->getParameters();
		sp->loadFromJSON(v);
		spectrumSlider.setValue(v.getProperty("SpectrumSlider", 0.0));

		currentWaveForm->zeroCrossing = v.getProperty("ZeroCrossing", true);
		currentWaveForm->setClickArea((AudioDisplayComponent::AreaTypes)(int)v.getProperty("ClickArea", (int)AudioDisplayComponent::AreaTypes::numAreas), false);

		dynamic_cast<SamplerDisplayWithTimeline*>(viewContent.get())->setEnvelope((Modulation::Mode)(int)v.getProperty("Envelope", (int)Modulation::Mode::numModes), handler->getMainSelection().get(), true);
	}
}

void SampleEditor::mainSelectionChanged(SampleEditor& editor, ModulatorSamplerSound::Ptr sound, int micIndex)
{
	auto sampleIndex = editor.handler->getSelectionReference().getItemArray().indexOf(sound);

	editor.sampleSelector->clear(dontSendNotification);
	editor.multimicSelector->clear(dontSendNotification);

	auto sIndex = 1;

	for (auto s : *editor.handler)
	{
		editor.sampleSelector->addItem(s->getSampleProperty(SampleIds::FileName).toString().replace("{PROJECT_FOLDER}", ""), sIndex++);
	}

	auto micPositions = StringArray::fromTokens(editor.sampler->getStringForMicPositions(), ";", "");
	micPositions.removeEmptyStrings();

	auto mIndex = 1;

	for (auto t : micPositions)
	{
		editor.multimicSelector->addItem(t, mIndex++);
	}

	editor.multimicSelector->setTextWhenNothingSelected("No multimics");
	editor.multimicSelector->setTextWhenNoChoicesAvailable("No multimics");


	editor.sampleSelector->setSelectedItemIndex(sampleIndex, dontSendNotification);
	editor.multimicSelector->setSelectedItemIndex(micIndex, dontSendNotification);
	
	editor.currentWaveForm->setSoundToDisplay(sound.get(), micIndex);

	ScopedPointer<AudioFormatReader> afr;

	if (sound != nullptr)
	{
		auto ss = sound->getReferenceToSound(micIndex);

		if (ss->isMonolithic())
			afr = ss->createReaderForPreview();
		else
			afr = PresetHandler::getReaderForFile(ss->getFileName(true));
	}

	editor.overview.setReader(afr.release());
}

void SampleEditor::soundsSelected(int numSelected)
{
    selection.clear();

	for (auto sound: *handler)
		selection.add(sound);

	auto selectionToUse = handler->getSelectionOrMainOnlyInTabMode();

	panSetter->setCurrentSelection(selectionToUse);
	volumeSetter->setCurrentSelection(selectionToUse);
	pitchSetter->setCurrentSelection(selectionToUse);
	sampleStartSetter->setCurrentSelection(selectionToUse);
	sampleEndSetter->setCurrentSelection(selectionToUse);
	startModulationSetter->setCurrentSelection(selectionToUse);
	loopStartSetter->setCurrentSelection(selectionToUse);
	loopEndSetter->setCurrentSelection(selectionToUse);
	loopCrossfadeSetter->setCurrentSelection(selectionToUse);

	
	updateWaveform();
}

void SampleEditor::paintOverChildren(Graphics &g)
{
	if (selection.size() != 0)
	{
		g.setColour(Colours::black.withAlpha(0.4f));

		const bool useGain = selection.getLast()->getNormalizedPeak() != 1.0f;

		String fileName = selection.getLast()->getPropertyAsString(SampleIds::FileName);

		PoolReference ref(sampler->getMainController(), fileName, FileHandlerBase::Samples);

		fileName = ref.getReferenceString();

		const String autogain = useGain ? ("Autogain: " + String(Decibels::gainToDecibels(selection.getLast()->getNormalizedPeak()), 1) + " dB") : String();

		int width = jmax<int>(GLOBAL_BOLD_FONT().getStringWidth(autogain), GLOBAL_BOLD_FONT().getStringWidth(fileName)) + 8;

		Rectangle<int> area; 
		
		if (isInWorkspace())
		{
			area = getLocalBounds().reduced(12).removeFromTop(24);
		}
		else
		{
			area = { viewport->getRight() - width - 1, viewport->getY() + 1, width, useGain ? 32 : 16 };
			g.fillRect(area);
		}
		
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());


		//g.drawText(fileName, area, Justification::topRight, false);

		//if (useGain)
		//	g.drawText(autogain, area, Justification::bottomRight, false);

		if (auto f = selection.getFirst())
		{
			if (f->isMissing())
			{
				g.setColour(Colours::black.withAlpha(0.4f));


				auto b = viewport->getBounds().toFloat();

				g.fillRect(b);

				g.setColour(Colours::white);
				g.drawText(f->getReferenceToSound()->getFileName(true) + " is missing", b, Justification::centred);
			}
		}

	}
}

void SampleEditor::updateWaveform()
{
	if(viewport->isVisible())
		currentWaveForm->updateRanges();

    repaint();
}

void SampleEditor::setZoomFactor(float factor, int mousePos /*= 0*/)
{
	zoomFactor = jlimit(1.0f, 128.0f, factor);
	auto oldPos = (double)(viewport->getViewPositionX());
	auto oldWidth = (double)currentWaveForm->getWidth();
	auto normDelta = mousePos / oldWidth;
	auto normPos = (double)oldPos / (double)currentWaveForm->getWidth();
	auto oldMousePosNorm = normDelta + normPos;

	resized();

	scrollBarMoved(&viewport->getHorizontalScrollBar(), 0.0f);

	auto newWidth = (double)viewport->getViewedComponent()->getWidth();

	auto newMousePos = oldMousePosNorm * newWidth;
	auto newPos = newMousePos - mousePos;

	viewport->setViewPosition(roundToInt(newPos), 0.0);
}



void SampleEditor::zoom(bool zoomOut, int mousePos/*=0*/)
{
	
    
#if JUCE_WINDOWS || JUCE_LINUX
	auto factor = 1.25f;
#else
	auto factor = 1.125f;
#endif

	if (!zoomOut)
		zoomFactor = zoomFactor * factor;
	else
		zoomFactor = zoomFactor / factor;
    
	setZoomFactor(zoomFactor, mousePos);

	
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SampleEditor" componentName=""
                 parentClasses="public Component, public SamplerSubEditor, public ApplicationCommandTarget, public AudioDisplayComponent::Listener, public SafeChangeListener"
                 constructorParams="ModulatorSampler *s, SamplerBody *b" variableInitialisers="sampler(s)"
                 snapPixels="8" snapActive="1" snapShown="0" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="250">
  <BACKGROUND backgroundColour="6f6f6f">
    <RECT pos="8 8 175M 24" fill="solid: 13ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: fffffff"/>
    <TEXT pos="8Rr 4 232 30" fill="solid: 70ffffff" hasStroke="0" text="SAMPLE EDITOR"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <VIEWPORT name="new viewport" id="babb35bbad848ac" memberName="viewport"
            virtualName="" explicitFocusOrder="0" pos="8 41 16M 119" vscroll="0"
            hscroll="1" scrollbarThickness="18" contentType="0" jucerFile=""
            contentClass="" constructorParams=""/>
  <JUCERCOMP name="" id="d91d22eeed4f96dc" memberName="volumeSetter" virtualName=""
             explicitFocusOrder="0" pos="31.5%r 162 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="6dcb9c7b0159189f" memberName="pitchSetter" virtualName=""
             explicitFocusOrder="0" pos="61 195 10% 32" posRelativeX="d91d22eeed4f96dc"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="dfa58ae047aa2ca6" memberName="sampleStartSetter"
             virtualName="" explicitFocusOrder="0" pos="4Cr 162 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="33421ae4da2b4051" memberName="sampleEndSetter" virtualName=""
             explicitFocusOrder="0" pos="0 196 14% 32" posRelativeX="dfa58ae047aa2ca6"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="aecfa63b27e29f3e" memberName="loopStartSetter" virtualName=""
             explicitFocusOrder="0" pos="51.875% 163 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="55f3ee71f4c1fc08" memberName="loopEndSetter" virtualName=""
             explicitFocusOrder="0" pos="0 196 14% 32" posRelativeX="aecfa63b27e29f3e"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="410604ce2de30fea" memberName="loopCrossfadeSetter"
             virtualName="" explicitFocusOrder="0" pos="0% 196 14% 32" posRelativeX="28c477b05f890ea4"
             sourceFile="ValueSettingComponent.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="28c477b05f890ea4" memberName="startModulationSetter"
             virtualName="" explicitFocusOrder="0" pos="139C 160 14% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
  <GENERICCOMPONENT name="new component" id="498417b33c43bc3c" memberName="toolbar"
                    virtualName="" explicitFocusOrder="0" pos="12 10 175M 20" class="Toolbar"
                    params=""/>
  <JUCERCOMP name="" id="d3e768374c58ef45" memberName="panSetter" virtualName=""
             explicitFocusOrder="0" pos="23.875%r 195 10% 32" sourceFile="ValueSettingComponent.cpp"
             constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...

void DraggableThumbnail::Laf::drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path)
{
	g.setColour(Colours::white.withAlpha(areaIsEnabled ? 0.6f : 0.4f));
	g.fillPath(path);
}

void DraggableThumbnail::Laf::drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area)
{
	if (area.getY() != 0)
		return;

	if(areaIsEnabled)
	{
		area = g.getClipBounds();

		float alpha = 0.7f;
		if (th.isMouseOverOrDragging())
			alpha += 0.2f;

		if (th.isMouseButtonDown())
			alpha += 0.2f;

		auto farea = area.toFloat().reduced(0.5f);

		auto c = th.isMouseButtonDown() ? Colour(SIGNAL_COLOUR) : Colours::white.withBrightness(alpha);

		g.setColour(c);
		g.drawRoundedRectangle(farea, 2.0f, 1.5f);

		g.setGradientFill(ColourGradient(c.withMultipliedAlpha(0.14f), 0.0f, 0.0f, c.withMultipliedAlpha(0.06f), 0.0f, th.getHeight(), false));

		g.fillRoundedRectangle(farea, 2.0f);
	}
}

DraggableThumbnail::DraggableThumbnail()
{
	setLookAndFeel(&laf);
	setShouldScaleVertically(true);
	setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xFF333333));
	setBufferedToImage(false);
	setInterceptsMouseClicks(true, true);
	setRepaintsOnMouseActivity(true);
}

void DraggableThumbnail::setPosition(const MouseEvent& e)
{
	auto se = findParentComponentOfClass<SampleEditor>();
	auto& vp = se->getViewport();

	auto posX = (float)e.getPosition().getX() / (float)getWidth();
	posX = jlimit(0.0f, 1.0f, posX);

	vp.setViewPositionProportionately(posX, 0.0f);
}

void DraggableThumbnail::mouseDown(const MouseEvent& e)
{
	auto se = findParentComponentOfClass<SampleEditor>();
	downZoomFactor = se->zoomFactor;
	downX = e.getPosition().getX();

	setPosition(e);
}

void DraggableThumbnail::mouseDrag(const MouseEvent& e)
{
	auto se = findParentComponentOfClass<SampleEditor>();
	auto thisY = (float)e.getDistanceFromDragStartY();

	if (std::abs(thisY) > getHeight() / 2)
	{
		if(thisY > 0)
			thisY -= getHeight() / 2;
		else
			thisY += getHeight() / 2;

		auto yFactor = jmax(0.0f, 1.0f + (thisY / JUCE_LIVE_CONSTANT_OFF(80.0f)));
		se->setZoomFactor(yFactor * downZoomFactor, downX);
	}

	setPosition(e);

}

void DraggableThumbnail::mouseDoubleClick(const MouseEvent& e)
{
	auto se = findParentComponentOfClass<SampleEditor>();
	se->setZoomFactor(1.0f, 0);
}

void DraggableThumbnail::paint(Graphics& g)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xff1e1e1e)));

	if (isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No sample selected", getLocalBounds().toFloat(), Justification::centred);
		return;
	}
		

	HiseAudioThumbnail::paint(g);

	if (currentSound != nullptr)
	{
		auto pc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::PlayArea);

		auto lc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::LoopArea);

		auto sc = AudioDisplayComponent::SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::SampleStartArea);

		auto getRange = [&](const Identifier& startId, const Identifier& endId)
		{
			auto start = startId.isValid() ? (int)currentSound->getSampleProperty(startId) : 0;
			auto end = (int)currentSound->getSampleProperty(endId);

			auto totalLength = (double)currentSound->getReferenceToSound(0)->getLengthInSamples();

			auto normStart = (double)start / totalLength;
			auto normEnd = (double)end / totalLength;

			auto x = roundToInt(normStart * (double)getWidth());
			auto y = 0;
			auto w = roundToInt((normEnd - normStart) * (double)getWidth());
			auto h = getHeight();
			return Rectangle<int>(x, y, w, h);
		};

		auto playArea = getRange(SampleIds::SampleStart, SampleIds::SampleEnd);
		g.setColour(pc.withAlpha(0.03f));
		g.fillRect(playArea);

		if (currentSound->getSampleProperty(SampleIds::LoopEnabled))
		{
			auto loopArea = getRange(SampleIds::LoopStart, SampleIds::LoopEnd);
			g.setColour(lc.withAlpha(0.3f));
			g.fillRect(loopArea);
		}
		
		auto modArea = getRange({}, SampleIds::SampleStartMod);
		modArea = modArea.translated(playArea.getX(), 0);
		g.setColour(sc.withAlpha(0.3f));
		g.fillRect(modArea);

	}
}

void DraggableThumbnail::mainSoundSelected(DraggableThumbnail& d, ModulatorSamplerSound::Ptr sound, int)
{
	d.currentSound = sound;
	d.repaint();
}

} // namespace hise
//[/EndFile]
