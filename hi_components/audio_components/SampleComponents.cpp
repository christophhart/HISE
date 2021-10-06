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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise
{
using namespace juce;



ScriptingObjects::ScriptAudioFile* getScriptAudioFile(ReferenceCountedObject* p)
{
	return dynamic_cast<ScriptingObjects::ScriptAudioFile*>(p);
}

WaveformComponent::WaveformComponent(Processor* p, int index_) :
	processor(p),
	tableLength(0),
	tableValues(nullptr),
	index(index_)
{
	setColour(bgColour, Colours::transparentBlack);
	setColour(lineColour, Colours::white);
	setColour(fillColour, Colours::white.withAlpha(0.5f));

	if (p != nullptr)
	{
		p->addChangeListener(this);

		if (auto b = dynamic_cast<Broadcaster*>(p))
		{
			b->addWaveformListener(this);
			b->getWaveformTableValues(index, &tableValues, tableLength, normalizeValue);
		}
		else
			jassertfalse; // You have to subclass the processor...


	}

	setBufferedToImage(true);
}

WaveformComponent::~WaveformComponent()
{
	if (processor.get() != nullptr)
	{
		dynamic_cast<Broadcaster*>(processor.get())->removeWaveformListener(this);
		processor->removeChangeListener(this);
	}

}

void WaveformComponent::paint(Graphics &g)
{
	if (useFlatDesign)
	{
		g.setColour(findColour(bgColour));
		g.fillAll();

		g.setColour(findColour(fillColour));
		g.fillPath(path);

		g.setColour(findColour(lineColour));
		g.strokePath(path, PathStrokeType(2.0f));
	}
	else
	{
		auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

		laf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());
		laf->drawOscilloscopePath(g, *this, path);
	}
}

void WaveformComponent::refresh()
{
	if (rb != nullptr)
	{
		const auto& s = rb->getReadBuffer();
		setTableValues(s.getReadPointer(0), s.getNumSamples(), 1.0f);
	}

	rebuildPath();
}

juce::Path WaveformComponent::WaveformFactory::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("sine", WaveformIcons::sine);
	LOAD_PATH_IF_URL("triangle", WaveformIcons::triangle);
	LOAD_PATH_IF_URL("saw", WaveformIcons::saw);
	LOAD_PATH_IF_URL("square", WaveformIcons::square);
	LOAD_PATH_IF_URL("noise", WaveformIcons::noise);

	return p;
}



juce::Path WaveformComponent::getPathForBasicWaveform(WaveformType t)
{
	WaveformFactory f;

	switch (t)
	{
	case Sine:		return f.createPath("sine");
	case Triangle:	return f.createPath("triangle");
	case Saw:		return f.createPath("saw");
	case Square:	return f.createPath("square");
	case Noise:		return f.createPath("noise");
	default: break;
	}

	return {};
}

void WaveformComponent::setTableValues(const float* values, int numValues, float normalizeValue_)
{
	tableValues = values;
	tableLength = numValues;
	normalizeValue = normalizeValue_;
}

void WaveformComponent::rebuildPath()
{
	if (bypassed)
	{
		path.clear();
		repaint();
		return;
	}

	path.clear();

	if (broadcaster == nullptr)
		return;

	if (tableLength == 0)
	{
		repaint();
		return;
	}


	float w = (float)getWidth();
	float h = (float)getHeight();

	path.startNewSubPath(0.0, h / 2.0f);

	const float cycle = tableLength / w;

	if (tableValues != nullptr && tableLength > 0)
	{

		for (int i = 0; i < getWidth(); i++)
		{
			const float tableIndex = ((float)i * cycle);

			float value;

			if (broadcaster->interpolationMode == LinearInterpolation)
			{
				const int x1 = (int)tableIndex;
				const int x2 = (x1 + 1) % tableLength;
				const float alpha = tableIndex - (float)x1;

				value = Interpolator::interpolateLinear(tableValues[x1], tableValues[x2], alpha);
			}
			else
			{
				value = tableValues[(int)tableIndex];
			}

			value = broadcaster->scaleFunction(value);

			value *= normalizeValue;

			jassert(tableIndex < tableLength);

			path.lineTo((float)i, value * -(h - 2) / 2 + h / 2);
		}
	}

	path.lineTo(w, h / 2.0f);

	//path.closeSubPath();

	repaint();
}

juce::Identifier WaveformComponent::Panel::getProcessorTypeId() const
{
	return WavetableSynth::getClassType();
}

Component* WaveformComponent::Panel::createContentComponent(int index)
{
	if (index == -1)
		index = 0;

	auto c = new WaveformComponent(getProcessor(), index);

	c->setUseFlatDesign(true);
	c->setColour(bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	c->setColour(fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
	c->setColour(lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));

	if (c->findColour(bgColour).isOpaque())
		c->setOpaque(true);

	return c;
}

void WaveformComponent::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<WavetableSynth>(moduleList);
}

void WaveformComponent::Broadcaster::connectWaveformUpdaterToComplexUI(ComplexDataUIBase* d, bool enableUpdate)
{
	if (d == nullptr)
		return;

	if (enableUpdate)
	{
		d->getUpdater().addEventListener(&updater);

		if (auto rb = dynamic_cast<SimpleRingBuffer*>(d))
			rb->setPropertyObject(new BroadcasterPropertyObject(this));
	}
	else
		d->getUpdater().removeEventListener(&updater);
}

void WaveformComponent::Broadcaster::updateData()
{
	for (int i = 0; i < getNumWaveformDisplays(); i++)
	{
		float const* values = nullptr;
		int numValues = 0;
		float normalizeFactor = 1.0f;

		getWaveformTableValues(i, &values, numValues, normalizeFactor);

		for (auto l : listeners)
		{
			if (l.getComponent() != nullptr && l->index == i)
			{
				l->setTableValues(values, numValues, normalizeFactor);
				l->rebuildPath();
			}
		}
	}

}

SamplerSoundWaveform::SamplerSoundWaveform(const ModulatorSampler *ownerSampler) :
	AudioDisplayComponent(),
	sampler(ownerSampler),
	sampleStartPosition(-1.0),
	currentSound(nullptr)
{
	areas.add(new SampleArea(PlayArea, this));
	areas.add(new SampleArea(SampleStartArea, this));
	areas.add(new SampleArea(LoopArea, this));
	areas.add(new SampleArea(LoopCrossfadeArea, this));

	setColour(AudioDisplayComponent::ColourIds::bgColour, Colour(0xFF383838));

	addAndMakeVisible(areas[PlayArea]);
	areas[PlayArea]->addAndMakeVisible(areas[SampleStartArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopCrossfadeArea]);
	areas[PlayArea]->setAreaEnabled(false);
	
	areas[SampleStartArea]->leftEdge->setVisible(false);
	areas[LoopCrossfadeArea]->rightEdge->setVisible(false);

	setOpaque(true);

	setMouseCursor(MouseCursor::CrosshairCursor);

#ifdef JUCE_DEBUG

	startTimer(30);

#else

	startTimer(30);

#endif

    getThumbnail()->setBufferedToImage(false);
	getThumbnail()->setDrawHorizontalLines(true);
	getThumbnail()->setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
	getThumbnail()->setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
	getThumbnail()->setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
	getThumbnail()->setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::white.withAlpha(0.7f));
};

SamplerSoundWaveform::~SamplerSoundWaveform()
{

}



void SamplerSoundWaveform::timerCallback()
{
	auto previewActive = (previewStart != -1);

	if (sampler->getLastStartedVoice() != nullptr || previewActive)
	{
		if (previewActive || dynamic_cast<ModulatorSamplerVoice*>(sampler->getLastStartedVoice())->getCurrentlyPlayingSamplerSound() == currentSound.get())
		{
			sampleStartPosition = sampler->getSamplerDisplayValues().currentSampleStartPos;
			setPlaybackPosition(sampler->getSamplerDisplayValues().currentSamplePos);
		}
		else
		{
			setPlaybackPosition(0);
		}
	}

};


void SamplerSoundWaveform::updateRanges(SampleArea *areaToSkip)
{
	if (currentSound != nullptr)
	{
		updateRange(PlayArea, false);
		updateRange(SampleStartArea, false);
		updateRange(LoopArea, false);
		updateRange(LoopCrossfadeArea, true);
	}
	else
	{
		refreshSampleAreaBounds(areaToSkip);
	}


}

void SamplerSoundWaveform::updateRange(AreaTypes a, bool refreshBounds)
{
	auto area = areas[a];

	switch (a)
	{
	case hise::AudioDisplayComponent::PlayArea:
		area->setSampleRange(Range<int>(currentSound->getSampleProperty(SampleIds::SampleStart),
			currentSound->getSampleProperty(SampleIds::SampleEnd)));

		area->setAllowedPixelRanges(currentSound->getPropertyRange(SampleIds::SampleStart),
			currentSound->getPropertyRange(SampleIds::SampleEnd));
		break;
	case hise::AudioDisplayComponent::SampleStartArea:
	{
		area->setSampleRange(Range<int>(currentSound->getSampleProperty(SampleIds::SampleStart), (int)currentSound->getSampleProperty(SampleIds::SampleStart) + (int)currentSound->getSampleProperty(SampleIds::SampleStartMod)));
		area->setAllowedPixelRanges(currentSound->getPropertyRange(SampleIds::SampleStart),
			currentSound->getPropertyRange(SampleIds::SampleStartMod));
		break;
	}
	case hise::AudioDisplayComponent::LoopArea:
	{
		area->setVisible(currentSound->getSampleProperty(SampleIds::LoopEnabled));
		area->setSampleRange(Range<int>(currentSound->getSampleProperty(SampleIds::LoopStart),
			currentSound->getSampleProperty(SampleIds::LoopEnd)));

		area->setAllowedPixelRanges(currentSound->getPropertyRange(SampleIds::LoopStart),
			currentSound->getPropertyRange(SampleIds::LoopEnd));
		break;
	}
	case hise::AudioDisplayComponent::LoopCrossfadeArea:
	{
		const int64 start = (int64)currentSound->getSampleProperty(SampleIds::LoopStart) - (int64)currentSound->getSampleProperty(SampleIds::LoopXFade);

		area->setSampleRange(Range<int>((int)start, currentSound->getSampleProperty(SampleIds::LoopStart)));
		break;
	}
	case hise::AudioDisplayComponent::numAreas:
		break;
	default:
		break;
	}

	if (refreshBounds)
		refreshSampleAreaBounds(nullptr);
}

void SamplerSoundWaveform::toggleRangeEnabled(AreaTypes type)
{
	areas[type]->toggleEnabled();
}

double SamplerSoundWaveform::getSampleRate() const
{
	return currentSound != nullptr ? currentSound->getSampleRate() : -1.0;
}

void SamplerSoundWaveform::drawSampleStartBar(Graphics &g)
{
	if (sampleStartPosition != -1.0)
	{
		g.setColour(Colours::darkblue.withAlpha(0.5f));

		const int x = areas[PlayArea]->getX() + (int)(sampleStartPosition * areas[SampleStartArea]->getWidth());

		g.drawLine((float)x, 0.0f, (float)x, (float)getBottom() - 2.0f, 1.0f);

		g.setColour(Colours::blue.withAlpha(0.1f));

		g.fillRect(jmax<int>(0, x - 5), 0, 10, getHeight());
	}
}

void SamplerSoundWaveform::paint(Graphics &g)
{
	auto bgColour = findColour(AudioDisplayComponent::ColourIds::bgColour);
	g.fillAll(bgColour);

	if (getTotalSampleAmount() == 0) return;

	if (areas[SampleStartArea]->getSampleRange().getLength() != 0)
	{
		drawSampleStartBar(g);
	};

	if (previewStart != -1)
	{
		auto pos = roundToInt((double)previewStart / (double)getTotalSampleAmount() * (double)getWidth());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawVerticalLine(pos, 0.0f, (float)getHeight());
	}

	if (!onInterface && currentSound.get() != nullptr)
	{
		if (currentSound->getReferenceToSound()->isMonolithic())
		{
			g.setColour(Colour(0x22000000));
			g.fillRect(0, 0, 80, 20);
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(findColour(SamplerSoundWaveform::ColourIds::textColour));
			g.drawText("Monolith", 0, 0, 80, 20, Justification::centred);
		}
	}
}

void SamplerSoundWaveform::resized()
{
	AudioDisplayComponent::resized();

	if (onInterface)
	{
		for (auto a : areas)
			a->setVisible(false);
	}
}

void SamplerSoundWaveform::setSoundToDisplay(const ModulatorSamplerSound *s, int multiMicIndex/*=0*/)
{
	timeProperties.sampleLength = 0;
	timeProperties.sampleRate = 0.0;

	if (s != nullptr && !s->isMissing() && !s->isPurged())
	{
		currentSound = const_cast<ModulatorSamplerSound*>(s);

		StreamingSamplerSound::Ptr sound = s->getReferenceToSound(multiMicIndex);

		if (sound == nullptr)
		{
			jassertfalse;
			return;
		}

		ScopedPointer<AudioFormatReader> afr;

		if (sound->isMonolithic())
		{
			afr = sound->createReaderForPreview();
		}
		else
		{
			afr = PresetHandler::getReaderForFile(sound->getFileName(true));
		}

		if (afr != nullptr)
		{
			numSamplesInCurrentSample = (int)afr->lengthInSamples;

			if (onInterface && currentSound != nullptr)
			{
				numSamplesInCurrentSample = currentSound->getReferenceToSound()->getSampleLength();
			}

			refresh(dontSendNotification);
			preview->setReader(afr.release(), numSamplesInCurrentSample);

			timeProperties.sampleLength = (double)currentSound->getReferenceToSound(0)->getLengthInSamples();
			timeProperties.sampleRate = (double)currentSound->getReferenceToSound(0)->getSampleRate();

			updateRanges();
		}
		else jassertfalse;

	}
	else
	{
		currentSound = nullptr;

		for (int i = 0; i < areas.size(); i++)
		{
			areas[i]->setBounds(0, 0, 0, 0);
		}

		preview->clear();
	}
};



void SamplerSoundWaveform::mouseDown(const MouseEvent& e)
{
	if (e.mods.isAltDown())
	{
		auto mc = currentSound->getMainController();

		auto numSamples = getTotalSampleAmount();

		auto posNorm = (double)e.getPosition().getX() / (double)getWidth();

		auto start = roundToInt((double)numSamples * posNorm);
		start = getThumbnail()->getNextZero(start);

		AudioSampleBuffer b = getThumbnail()->getBufferCopy({ start, numSamples });

		b.applyGainRamp(0, 1024, 0.0f, 1.0f);

		auto s = const_cast<ModulatorSampler*>(sampler);

		previewStart = start;

		start -= (int)currentSound->getSampleProperty(SampleIds::SampleStart);

		mc->setBufferToPlay(b, [start, s, numSamples](int pos)
		{
			s->getSamplerDisplayValues().currentSampleStartPos = start;
			s->getSamplerDisplayValues().currentSamplePos = (double)(start + pos) / (double)numSamples;
		});

		return;
	}

    auto a =getAreaForModifiers(e);
    auto propId = getSampleIdToChange(a, e);
    
    if(propId.isValid())
    {
        auto n = (double)e.getPosition().getX() / (double)getWidth();
        
        auto value = roundToInt(timeProperties.sampleLength * n);
        
        if(propId == SampleIds::SampleStartMod)
            value -= (int)currentSound->getSampleProperty(SampleIds::SampleStart);
        
        if(zeroCrossing)
        {
            value = getThumbnail()->getNextZero(value);
        }
        
        auto r = currentSound->getPropertyRange(propId);
        
        value = jlimit(r.getStart(), r.getEnd(), value);
        
        currentSound->setSampleProperty(propId, value, true);
        return;
    }
    
	
}

void SamplerSoundWaveform::mouseUp(const MouseEvent& e)
{
	previewStart = -1;

	if (currentSound != nullptr)
		currentSound->getMainController()->stopBufferToPlay();
}

void SamplerSoundWaveform::mouseMove(const MouseEvent& e)
{
	if (currentSound != nullptr)
	{
		auto n = (double)e.getPosition().getX() / (double)getWidth();

		auto timeString = SamplerDisplayWithTimeline::getText(timeProperties, n);

		if(e.mods.isAltDown())
			setTooltip("Click to preview from " + timeString);

		auto a = getAreaForModifiers(e);
		auto propId = getSampleIdToChange(a, e);

		if (propId.isValid())
		{
			String tt;

			tt << "Set ";
			tt << propId;
			tt << " to " << timeString;

			setTooltip(tt);
		}
		else
			setTooltip(timeString);
	}
}

float SamplerSoundWaveform::getNormalizedPeak()
{
	const ModulatorSamplerSound *s = getCurrentSound();

	if (s != nullptr)
	{
		return s->getNormalizedPeak();
	}
	else return 1.0f;
}

float SamplerSoundWaveform::getCurrentSampleGain() const
{
	float gain = 1.0f;

	if (auto s = getCurrentSound())
	{
		if (s->isNormalizedEnabled())
		{
			gain = s->getNormalizedPeak();
		}

		auto vol = (double)s->getSampleProperty(SampleIds::Volume);

		gain *= Decibels::decibelsToGain(vol);
	}

	return gain * verticalZoomGain;
}

hise::AudioDisplayComponent::AreaTypes SamplerSoundWaveform::getAreaForModifiers(const MouseEvent& e) const
{
	return currentClickArea;
}

juce::Identifier SamplerSoundWaveform::getSampleIdToChange(AreaTypes a, const MouseEvent& e) const
{
	if (auto area = areas[a])
	{
        auto ae = e.getEventRelativeTo(area);
        bool isEnd = e.mods.isRightButtonDown() || a == AudioDisplayComponent::SampleStartArea;

		switch (a)
		{
		case AudioDisplayComponent::AreaTypes::PlayArea: return (isEnd ? SampleIds::SampleEnd : SampleIds::SampleStart);
		case AudioDisplayComponent::AreaTypes::SampleStartArea: return SampleIds::SampleStartMod;
        case AudioDisplayComponent::AreaTypes::LoopArea: return (isEnd ? SampleIds::LoopEnd : SampleIds::LoopStart);
		default: return {};
		}
	}

	return {};
}

hise::SamplerSoundWaveform* SamplerDisplayWithTimeline::getWaveform()
{
	return dynamic_cast<SamplerSoundWaveform*>(getChildComponent(0));
}

const hise::SamplerSoundWaveform* SamplerDisplayWithTimeline::getWaveform() const
{
	return dynamic_cast<SamplerSoundWaveform*>(getChildComponent(0));
}

void SamplerDisplayWithTimeline::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(TimelineHeight);
	getWaveform()->setBounds(b);
}

void SamplerDisplayWithTimeline::mouseDown(const MouseEvent& e)
{
	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);

	m.addItem(1, "Samples", true, props.currentDomain == TimeDomain::Samples);
	m.addItem(2, "Milliseconds", true, props.currentDomain == TimeDomain::Milliseconds);
	m.addItem(3, "Seconds", true, props.currentDomain == TimeDomain::Seconds);

	if (auto r = m.show())
	{
		props.currentDomain = (TimeDomain)(r - 1);
		getWaveform()->timeProperties.currentDomain = props.currentDomain;
		repaint();
	}
}

String SamplerDisplayWithTimeline::getText(const Properties& p, float normalisedX)
{
	if (p.sampleRate > 0.0)
	{
		auto sampleValue = roundToInt(normalisedX * p.sampleLength);

		if (p.currentDomain == TimeDomain::Samples)
			return String(roundToInt(sampleValue));

		auto msValue = sampleValue / jmax(1.0, p.sampleRate) * 1000.0;

		if (p.currentDomain == TimeDomain::Milliseconds)
			return String(roundToInt(msValue)) + " ms";

		String sec;
		sec << Time((int64)msValue).formatted("%M:%S:");

		auto ms = String(roundToInt(msValue) % 1000);

		while (ms.length() < 3)
			ms = "0" + ms;

		sec << ms;
		return sec;
	}

	return {};
}

void SamplerDisplayWithTimeline::paint(Graphics& g)
{
	auto visibleArea = findParentComponentOfClass<Viewport>()->getViewArea();

	auto b = getLocalBounds().removeFromTop(TimelineHeight);

	g.setFont(GLOBAL_FONT());

	int delta = 200;

	if (auto s = getWaveform()->getCurrentSound())
	{
		props.sampleLength = s->getReferenceToSound(0)->getLengthInSamples();
		props.sampleRate = s->getReferenceToSound(0)->getSampleRate();
	}

	for (int i = 0; i < getWidth(); i += delta)
	{
		auto textArea = b.removeFromLeft(delta).toFloat();

		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawVerticalLine(i, 3.0f, (float)TimelineHeight);

		g.setColour(Colours::white.withAlpha(0.4f));

		auto normalisedX = (float)i / (float)getWidth();

		g.drawText(getText(props, normalisedX), textArea.reduced(5.0f, 0.0f), Justification::centredLeft);
	}
}

}
