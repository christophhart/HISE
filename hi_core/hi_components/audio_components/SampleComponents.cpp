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
    setLookAndFeel(nullptr);
    
	if (processor.get() != nullptr)
	{
		dynamic_cast<Broadcaster*>(processor.get())->removeWaveformListener(this);
	}

}

void WaveformComponent::setBypassed(bool shouldBeBypassed)
{
	if (bypassed != shouldBeBypassed)
	{
		bypassed = shouldBeBypassed;
		rebuildPath();
	}
}

void WaveformComponent::paint(Graphics &g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();
	laf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());
	laf->drawOscilloscopePath(g, *this, path);
}

void WaveformComponent::resized()
{
	rebuildPath();
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

Colour WaveformComponent::getColourForAnalyserBase(int colourId)
{
	return findColour(colourId);
}

juce::Path WaveformComponent::WaveformFactory::createPath(const String& url) const
{
	Path p;

	LOAD_EPATH_IF_URL("sine", WaveformIcons::sine);
	LOAD_EPATH_IF_URL("triangle", WaveformIcons::triangle);
	LOAD_EPATH_IF_URL("saw", WaveformIcons::saw);
	LOAD_EPATH_IF_URL("square", WaveformIcons::square);
	LOAD_EPATH_IF_URL("noise", WaveformIcons::noise);

	return p;
}

WaveformComponent::Broadcaster::Updater::Updater(Broadcaster& p):
	parent(p)
{
	startTimer(30);
}

void WaveformComponent::Broadcaster::Updater::timerCallback()
{
	if (changeFlag)
	{
		changeFlag = false;

		parent.updateData();
	}
}

void WaveformComponent::Broadcaster::Updater::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
		parent.triggerWaveformUpdate();
}

bool WaveformComponent::Broadcaster::BroadcasterPropertyObject::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
		return SimpleRingBuffer::toFixSize<128>(v);

	if (id == RingBufferIds::NumChannels)
		return SimpleRingBuffer::toFixSize<1>(v);
                
	return true;
}

Path WaveformComponent::Broadcaster::BroadcasterPropertyObject::createPath(Range<int> sampleRange,
	Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const
{
	const float* d[1] = { nullptr };
	int numSamples = 0;
	float nv;
	br->getWaveformTableValues(0, d, numSamples, nv);

	AudioSampleBuffer b((float**)d, 1, numSamples);
				
	Path p;
	p.preallocateSpace(numSamples);
	p.startNewSubPath(0.0f, 0.0f);
	p.startNewSubPath(0.0f, -1.0f);
	p.startNewSubPath(0.0f, -1.0f * startValue);

	for (int i = 0; i < numSamples; i++)
		p.lineTo((float)i, -1.0f * b.getSample(0, i));

				

	if(!p.getBounds().isEmpty() && !targetBounds.isEmpty())
		p.scaleToFit(targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), targetBounds.getHeight(), false);

	return p;
}

void WaveformComponent::Broadcaster::BroadcasterPropertyObject::transformReadBuffer(AudioSampleBuffer& b)
{
	if (br != nullptr)
	{
		const float* d[1] = { nullptr };
		int numSamples = 0;
		float nv;
		br->getWaveformTableValues(0, d, numSamples, nv);

		if (numSamples == 128)
			FloatVectorOperations::copy(b.getWritePointer(0), d[0], numSamples);
	}
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

WaveformComponent::Panel::Panel(FloatingTile* parent):
	PanelWithProcessorConnection(parent)
{
	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
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
	{
		d->getUpdater().removeEventListener(&updater);
	}
		
}

void WaveformComponent::Broadcaster::suspendStateChanged(bool shouldBeSuspended)
{
	updater.suspendTimer(shouldBeSuspended);
}

void WaveformComponent::Broadcaster::addWaveformListener(WaveformComponent* listener)
{
	listener->broadcaster = this;
	listeners.addIfNotAlreadyThere(listener);
}

void WaveformComponent::Broadcaster::removeWaveformListener(WaveformComponent* listener)
{
	listener->broadcaster = nullptr;
	listeners.removeAllInstancesOf(listener);
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

Colour SamplerTools::getToolColour(Mode m)
{
	switch(m)
	{
	case Mode::GainEnvelope:
	case Mode::PitchEnvelope:
	case Mode::FilterEnvelope:  return SamplerDisplayWithTimeline::getColourForEnvelope((Modulation::Mode)((int)m - (int)Mode::GainEnvelope));
	case Mode::PlayArea:
	case Mode::LoopArea:
	case Mode::LoopCrossfadeArea:
	case Mode::SampleStartArea: return AudioDisplayComponent::SampleArea::getAreaColour((AudioDisplayComponent::AreaTypes)((int)m - (int)Mode::PlayArea));
	default: return Colours::white;
	}
}

void SamplerTools::toggleMode(Mode newMode)
{
	if(currentMode == newMode)
		currentMode = Mode::Nothing;
	else
		currentMode = newMode;
        
	broadcaster.sendMessage(sendNotificationSync, currentMode);
}

void SamplerTools::setMode(Mode newMode)
{
	if(currentMode != newMode)
	{
		currentMode = newMode;
		broadcaster.sendMessage(sendNotificationSync, currentMode);
	}
}

SamplerSoundWaveform::SamplerSoundWaveform(ModulatorSampler *ownerSampler) :
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

    sampler->addDeleteListener(this);
    
	addAndMakeVisible(areas[PlayArea]);
	areas[PlayArea]->addAndMakeVisible(areas[SampleStartArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopCrossfadeArea]);
	areas[PlayArea]->setAreaEnabled(false);
	
	

	startTimer(30);
};

SamplerSoundWaveform::~SamplerSoundWaveform()
{
    if(sampler != nullptr)
        sampler->removeDeleteListener(this);
    
    getThumbnail()->setLookAndFeel(nullptr);
    slaf = nullptr;
}

struct SamplerLaf : public HiseAudioThumbnail::LookAndFeelMethods,
	public LookAndFeel_V3,
	public PathFactory
{
	Path createPath(const String& url) const override
	{
		Path p;

		LOAD_EPATH_IF_URL("loop", SampleToolbarIcons::loopOn);
		LOAD_EPATH_IF_URL("samplestart", ProcessorIcons::sampleStartIcon);
		LOAD_EPATH_IF_URL("xfade", ProcessorIcons::groupFadeIcon);
		return p;
	}

	void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path) override
	{
		float wAlpha = th.waveformAlpha * th.waveformAlpha;

		g.setColour(Colour(0xFFAAAAAA).withAlpha(wAlpha).withMultipliedBrightness(areaIsEnabled ? 1.0f : 0.6f));
		g.strokePath(path, PathStrokeType(1.0f));
	}

	void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area) override
	{
		g.setColour(Colours::white.withAlpha(areaIsEnabled ? 0.4f : 0.1f));
		g.drawHorizontalLine(area.getCentreY(), area.getX(), area.getRight());
	}

	void drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const HiseAudioThumbnail::RectangleListType& rectList) override
	{
		float wAlpha = th.waveformAlpha * th.waveformAlpha;
		g.setColour(Colour(0xFFAAAAAA).withAlpha(wAlpha).withMultipliedBrightness(areaIsEnabled ? 1.0f : 0.6f));
		g.fillRectList(rectList);
	}

	void drawThumbnailRange(Graphics& g, HiseAudioThumbnail& te, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled) override
	{
		if (areaIndex == AudioDisplayComponent::AreaTypes::PlayArea)
		{
			UnblurryGraphics ug(g, te, true);

			g.setColour(c.withAlpha(areaEnabled ? 0.4f : 0.2f));

			ug.draw1PxRect(area);
		}
		else
		{
			g.setColour(c.withAlpha(areaEnabled ? 1.0f : 0.8f));

			switch (areaIndex)
			{
			case AudioDisplayComponent::AreaTypes::SampleStartArea:
			{
				auto right = area.removeFromRight(1.0f);
				auto top = area.removeFromTop(3.0f);

				g.fillRect(right);

				auto w = (int)top.getWidth();

				for (int i = 0; i < w; i += 6)
				{
					g.fillRect(top.removeFromLeft(6));
					top.removeFromLeft(1);
				}

				g.setColour(c.withAlpha(areaEnabled ? 0.1f : 0.04f));
				g.fillRect(area);

				break;
			}
			
			case AudioDisplayComponent::AreaTypes::LoopArea:
			{
				g.setColour(c.withAlpha(areaEnabled ? 0.1f : 0.04f));
				g.fillRect(area);

				g.setColour(c.withAlpha(areaEnabled ? 1.0f : 0.8f));

				auto left = area.removeFromLeft(1.0f);
				auto right = area.removeFromRight(1.0f);
				auto top = area.removeFromTop(8.0f);

				auto topLeft = top.removeFromLeft(50.0f);
				auto topRight = top.removeFromRight(50.0f);

				g.fillRect(left);
				g.fillRect(right);
				g.fillRect(topLeft);
				g.fillRect(topRight);
				break;
			}
			}
			
			const static StringArray names = { "play",  "samplestart", "loop", "xfade"};

			if (area.getWidth() > 30)
			{
				auto p = createPath(names[areaIndex]);
				scalePath(p, area.removeFromRight(24.0f).removeFromTop(24.0f).reduced(4.0f));
				g.setColour(c);
				g.fillPath(p);
			}
		}
	}
};

void SamplerSoundWaveform::setIsSamplerWorkspacePreview()
{
    inWorkspace = true;
	onInterface = false;
    setOpaque(true);
    setMouseCursor(MouseCursor::NormalCursor);
    getThumbnail()->setBufferedToImage(false);
    getThumbnail()->setDrawHorizontalLines(true);
    getThumbnail()->setDisplayMode(HiseAudioThumbnail::DisplayMode::DownsampledCurve);
    getThumbnail()->setColour(AudioDisplayComponent::ColourIds::bgColour, Colours::transparentBlack);
    getThumbnail()->setColour(AudioDisplayComponent::ColourIds::fillColour, Colours::transparentBlack);
    getThumbnail()->setColour(AudioDisplayComponent::ColourIds::outlineColour, Colours::white.withAlpha(0.7f));

	slaf = new SamplerLaf();

	getThumbnail()->setLookAndFeel(slaf);
}

void SamplerSoundWaveform::timerCallback()
{
	auto previewActive = sampler->getMainController()->getPreviewBufferPosition() > 0;

	if (lastActive != previewActive)
	{
		lastActive = previewActive;
		repaint();
	}

	if (sampler->getLastStartedVoice() != nullptr || previewActive)
	{
		if (currentSound != nullptr && (previewActive || dynamic_cast<ModulatorSamplerVoice*>(sampler->getLastStartedVoice())->getCurrentlyPlayingSamplerSound() == currentSound.get()))
		{
			auto dv = sampler->getSamplerDisplayValues();
			auto reversed = currentSound->getReferenceToSound(0)->isReversed();
			sampleStartPosition = reversed ? 1.0 - dv.currentSampleStartPos : dv.currentSampleStartPos;

			setPlaybackPosition(dv.currentSamplePos);
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
		

		auto isReversed = currentSound->getReferenceToSound(0)->isReversed();

		Range<int> displayArea;
		Range<int> leftDragRange, rightDragRange;
		
		auto startMod = (int)currentSound->getSampleProperty(SampleIds::SampleStartMod);

		if (isReversed)
		{
			auto offset = (int)currentSound->getSampleProperty(SampleIds::SampleEnd) - startMod;

			displayArea = { offset, offset + startMod };

			leftDragRange = { 0, offset + startMod };
			rightDragRange = currentSound->getPropertyRange(SampleIds::SampleEnd);
		}
		else
		{
			auto offset = (int)currentSound->getSampleProperty(SampleIds::SampleStart);

			displayArea = { offset, offset + startMod };
			leftDragRange = currentSound->getPropertyRange(SampleIds::SampleStart);
			rightDragRange = currentSound->getPropertyRange(SampleIds::SampleStartMod) + offset;
		}
		
		area->setSampleRange(displayArea);
		area->setAllowedPixelRanges(leftDragRange, rightDragRange);
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
		int start = 0;
		int end = 0;

		auto rev = currentSound->getReferenceToSound(0)->isReversed();
		area->setReversed(rev);

		if (rev)
		{
			start = (int)currentSound->getSampleProperty(SampleIds::LoopEnd);
			end = (int)currentSound->getSampleProperty(SampleIds::LoopEnd) + (int)currentSound->getSampleProperty(SampleIds::LoopXFade);;
		}
		else
		{
			start = (int)currentSound->getSampleProperty(SampleIds::LoopStart) - (int)currentSound->getSampleProperty(SampleIds::LoopXFade);
			end = currentSound->getSampleProperty(SampleIds::LoopStart);
		}

		area->setSampleRange(Range<int>(start, end));
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
		auto c = SampleArea::getAreaColour(AudioDisplayComponent::AreaTypes::SampleStartArea);
		g.setColour(c);

		const int x = areas[PlayArea]->getX() + areas[SampleStartArea]->getX() + (int)(sampleStartPosition * areas[SampleStartArea]->getWidth());

		g.drawVerticalLine(x, 1, (float)getBottom() - 1);

		g.setColour(c.withAlpha(0.3f));

		g.fillRect(jmax<int>(0, x - 5), 1, 10, getHeight() - 2);
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

#if USE_BACKEND
	const auto& p = sampler->getSampleEditHandler()->getPreviewer();

	auto previewStart = p.getPreviewStart();

	if (previewStart != -1)
	{
		auto pos = roundToInt((double)previewStart / (double)getTotalSampleAmount() * (double)getWidth());
		g.setColour(Colours::white.withAlpha(0.5f));

		if (p.isPlaying())
			g.setColour(Colour(SIGNAL_COLOUR));

		g.drawVerticalLine(pos, 0.0f, (float)getHeight());

		Path p;
		p.loadPathFromData(LoopIcons::preview, sizeof(LoopIcons::preview));

		Rectangle<float> pb((float)pos + 5.0f, 5.0f, 14.0f, 14.0f);
		PathFactory::scalePath(p, pb);

		g.strokePath(p, PathStrokeType(1.0f));
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
#endif
}

void SamplerSoundWaveform::paintOverChildren(Graphics &g)
{
	AudioDisplayComponent::paintOverChildren(g);

	#if HISE_SAMPLER_ALLOW_RELEASE_START
	if(currentSound != nullptr)
	{
		auto rs = (int)currentSound->getSampleProperty(SampleIds::ReleaseStart);

		if(rs != 0)
		{
			auto xOffset = roundToInt((double)getWidth() * (rs) / (double)getTotalSampleAmount());
			auto rc = SampleArea::getReleaseStartColour();

			g.setColour(rc.withAlpha(0.7f));
			g.drawVerticalLine(xOffset, 0.0f, (float)getHeight());
			g.fillRect((float)xOffset, 0.0f, 30.0f, JUCE_LIVE_CONSTANT_OFF(7.0f));

			auto options = sampler->getSampleMap()->getReleaseStartOptions();

			g.setColour(rc.withAlpha(0.2f));

			auto fadeWidth = roundToInt((double)getWidth() * (double)options->releaseFadeTime / (double)getTotalSampleAmount());
			Rectangle<float> fadeArea((float)xOffset, 0.0f, (float)fadeWidth, (float)getHeight());
			g.fillRect(fadeArea.toFloat());

			Path p;

			p.startNewSubPath(0.0f, 0.0f);
			p.quadraticTo(0.5f, std::pow(0.5f, options->fadeGamma), 1.0f, 1.0f);
			p.scaleToFit(fadeArea.getX(), fadeArea.getY(), fadeArea.getWidth(), fadeArea.getHeight(), false);
			g.setColour(rc.withAlpha(0.7f));
			g.strokePath(p, PathStrokeType(1.0f));
		}

	}
	#endif

	if (xPos != -1)
	{
		if(releaseStartIsSelected)
		{
			g.setColour(SampleArea::getReleaseStartColour());
		}
		else if (previewHover)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawVerticalLine(xPos, 0.0f, (float)getHeight());
			return;
		}
		else
			g.setColour(SampleArea::getAreaColour(currentClickArea));

		Rectangle<float> lineArea((float)xPos, 0.0f, 1.0f, (float)getHeight());
		RectangleList<float> segments;

		for (int i = 0; i < getHeight(); i += 6)
		{
			segments.addWithoutMerging(lineArea.removeFromTop(4.0f));
			lineArea.removeFromTop(2.0f);
		}

		g.fillRectList(segments);

		auto n = (double)xPos / (double)getWidth();

		auto timeString = SamplerDisplayWithTimeline::getText(timeProperties, n);

		auto f = GLOBAL_BOLD_FONT();

		Rectangle<float> ta(xPos, 0.0f, f.getStringWidthFloat(timeString) + 15.0f, 20.0f);

		g.fillRect(ta);
		g.setColour(Colours::black.withAlpha(0.8f));
		g.setFont(f);
		g.drawText(timeString, ta, Justification::centred);
	}
}

void SamplerSoundWaveform::resized()
{
	AudioDisplayComponent::resized();

	if (onInterface)
	{
		for (auto a : areas)
		{
			a->setVisible(a->isAreaEnabled());
		}
	}
}

void SamplerSoundWaveform::setSoundToDisplay(const ModulatorSamplerSound *s, int multiMicIndex/*=0*/)
{
	setPlaybackPosition(0);
	timeProperties.sampleLength = 0;
	timeProperties.sampleRate = 0.0;

	currentSound = const_cast<ModulatorSamplerSound*>(s);

	gammaListener.setCallback(sampler.get()->getSampleMap()->getValueTree(), { Identifier("CrossfadeGamma") }, valuetree::AsyncMode::Asynchronously, [this](Identifier, var newValue)
		{
			getSampleArea(AreaTypes::LoopCrossfadeArea)->setGamma((float)newValue);
		});

	if (s != nullptr && !s->isMissing() && !s->isPurged())
	{
		

		auto reversed = s->getReferenceToSound(0)->isReversed();

		areas[SampleStartArea]->leftEdge->setVisible(reversed);
		areas[LoopCrossfadeArea]->rightEdge->setVisible(reversed);
		areas[SampleStartArea]->rightEdge->setVisible(!reversed);
		areas[LoopCrossfadeArea]->leftEdge->setVisible(!reversed);

		if (auto afr = currentSound->createAudioReader(multiMicIndex))
		{
			numSamplesInCurrentSample = (int)afr->lengthInSamples;

			refresh(dontSendNotification);
			preview->setReader(afr, numSamplesInCurrentSample);

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
	if (onInterface)
		return;

	

#if USE_BACKEND

	if(releaseStartIsSelected)
	{
		auto n = (double)e.getPosition().getX() / (double)getWidth();
        
        auto value = roundToInt(timeProperties.sampleLength * n);
        
        if(zeroCrossing)
        {
            value = getThumbnail()->getNextZero(value);
        }
        
		if (currentSound == nullptr)
			return;

        auto r = currentSound->getPropertyRange(SampleIds::ReleaseStart);

		if (!r.contains(value))
			return;
		
        currentSound->setSampleProperty(SampleIds::ReleaseStart, value, true);
        return;
	}

	if (e.mods.isAnyModifierKeyDown())
	{
		auto numSamples = getTotalSampleAmount();
		auto posNorm = (double)e.getPosition().getX() / (double)getWidth();
		auto start = roundToInt((double)numSamples * posNorm);
		start = getThumbnail()->getNextZero(start);

		AudioSampleBuffer full = getThumbnail()->getBufferCopy({ 0, numSamples });

        auto s = sampler.get();

		s->getSampleEditHandler()->setPreviewStart(start);
		s->getSampleEditHandler()->togglePreview();

		return;
	}

    auto a =getAreaForModifiers(e);
    auto propId = getSampleIdToChange(a, e);
    
    if(propId.isValid())
    {
        auto n = (double)e.getPosition().getX() / (double)getWidth();
        
        auto value = roundToInt(timeProperties.sampleLength * n);
        
        if(zeroCrossing)
        {
            value = getThumbnail()->getNextZero(value);
        }
        
		if (currentSound == nullptr)
			return;

        if (propId == SampleIds::SampleStartMod)
            value -= (int)currentSound->getSampleProperty(SampleIds::SampleStart);
        
        auto r = currentSound->getPropertyRange(propId);

		if (!r.contains(value))
			return;
		
        currentSound->setSampleProperty(propId, value, true);
        return;
    }
#endif
}

void SamplerSoundWaveform::mouseUp(const MouseEvent& e)
{
	if (onInterface)
		return;

#if USE_BACKEND
	if(e.mods.isAnyModifierKeyDown())
		sampler->getSampleEditHandler()->togglePreview();
#endif
}

void SamplerSoundWaveform::mouseMove(const MouseEvent& e)
{
	if (onInterface)
		return;

	AudioDisplayComponent::mouseMove(e);

	if (currentSound != nullptr)
	{
		auto n = (double)e.getPosition().getX() / (double)getWidth();

		auto timeString = SamplerDisplayWithTimeline::getText(timeProperties, n);



		previewHover = !releaseStartIsSelected && e.mods.isAnyModifierKeyDown();

		if(releaseStartIsSelected)
		{
			setTooltip("Click to set release start offset from " + timeString);

			setMouseCursor(MouseCursor::CrosshairCursor);
			xPos = e.getPosition().getX();

			if(zeroCrossing)
			{
				auto n = (double)xPos / (double)getWidth();
				auto value = roundToInt(timeProperties.sampleLength * n);
				value = getThumbnail()->getNextZero(value);
				n = (double)value / timeProperties.sampleLength;
				xPos = roundToInt(n * (double)getWidth());
			}
			
			repaint();
			return;
		}
		if (previewHover)
		{
			setTooltip("Click to preview from " + timeString);
			
			Image icon(Image::ARGB, 30, 30, true);
			Graphics g(icon);

			
			Path p;
			p.loadPathFromData(LoopIcons::preview, SIZE_OF_PATH(LoopIcons::preview));
			PathFactory::scalePath(p, { 0.0f, 0.0f, 30.0f, 30.0f });
			g.setColour(Colours::white);
			g.fillPath(p);
			setMouseCursor(MouseCursor(icon, 15, 15));
			xPos = e.getPosition().getX();
			repaint();
			return;
		}
		
		auto a = getAreaForModifiers(e);
		auto propId = getSampleIdToChange(a, e);

		if (propId.isValid())
		{
			String tt;

			tt << "Set ";
			tt << propId;
			tt << " to " << timeString;
			xPos = e.getEventRelativeTo(this).getPosition().getX();

			auto n = (double)xPos / (double)getWidth();

			auto value = roundToInt(timeProperties.sampleLength * n);

			auto pr = currentSound->getPropertyRange(propId);

			if (propId == SampleIds::SampleStartMod)
				pr += (int)currentSound->getSampleProperty(SampleIds::SampleStart);
			if (propId == SampleIds::LoopStart)
			{
				pr = pr.getUnionWith(currentSound->getPropertyRange(SampleIds::LoopEnd));
			}
			if (propId == SampleIds::SampleStart)
			{
				pr = pr.getUnionWith(currentSound->getPropertyRange(SampleIds::SampleEnd));
			}

			value = pr.clipValue(value);

			if (zeroCrossing)
			{
				value = getThumbnail()->getNextZero(value);
			}

			n = (double)value / timeProperties.sampleLength;
			xPos = roundToInt(n * (double)getWidth());

			setTooltip(tt);
			setMouseCursor(MouseCursor::CrosshairCursor);
		}
		else
		{
			xPos = -1;
			setTooltip(timeString);
			setMouseCursor(MouseCursor::NormalCursor);
		}
			
	}

	repaint();
}

void SamplerSoundWaveform::mouseExit(const MouseEvent& e)
{
	xPos = -1;
	repaint();
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

void SamplerSoundWaveform::refresh(NotificationType n)
{
	getThumbnail()->setDisplayGain(getCurrentSampleGain(), n);
}

void SamplerSoundWaveform::setVerticalZoom(float zf)
{
	if (zf != verticalZoomGain)
	{
		verticalZoomGain = zf;
		refresh(sendNotificationSync);
	}
}

void SamplerSoundWaveform::setClickArea(AreaTypes newArea, bool resetIfSame)
{
	if (newArea == currentClickArea && resetIfSame)
		currentClickArea = AreaTypes::numAreas;
	else
		currentClickArea = newArea;

	for (int i = 0; i < areas.size(); i++)
	{
		areas[i]->setAreaEnabled(currentClickArea == i);
	}

	auto isSomething = currentClickArea != AreaTypes::numAreas;

	setMouseCursor(!isSomething ? MouseCursor::DraggingHandCursor : MouseCursor::CrosshairCursor);
		
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

SamplerDisplayWithTimeline::SamplerDisplayWithTimeline(ModulatorSampler* sampler)
{
	
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
	
	if (tableEditor != nullptr)
	{
		

		b.setWidth(b.getWidth() + 1);
		b.setHeight(b.getHeight() + 1);

		tableEditor->setBounds(b);
	}
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

juce::Colour SamplerDisplayWithTimeline::getColourForEnvelope(Modulation::Mode m)
{
	Colour colours[Modulation::Mode::numModes];

	colours[0] = JUCE_LIVE_CONSTANT_OFF(Colour(0xffbe952c));
	colours[1] = JUCE_LIVE_CONSTANT_OFF(Colour(0xff7559a4));
	colours[2] = JUCE_LIVE_CONSTANT_OFF(Colour(EFFECT_PROCESSOR_COLOUR));

	return colours[m];
}

void SamplerDisplayWithTimeline::paint(Graphics& g)
{
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


struct EnvelopeLaf : public TableEditor::LookAndFeelMethods,
			 public LookAndFeel_V3
{
	bool shouldClosePath() const override { return false; };

	void drawTableRuler(Graphics& , TableEditor& , Rectangle<float> , float , double ) override
	{

	};
};

void SamplerDisplayWithTimeline::setEnvelope(Modulation::Mode m, ModulatorSamplerSound* sound, bool setVisible)
{
	envelope = m;

	if (!setVisible || sound == nullptr || envelope == Modulation::Mode::numModes)
	{
		tableEditor = nullptr;
		resized();
		return;
	}

	if (sound != nullptr)
	{
		if (auto t = sound->getEnvelope(m))
		{
			auto table = &t->table;
			auto p = &getWaveform()->timeProperties;

			addAndMakeVisible(tableEditor = new TableEditor(nullptr, table));
			tableEditor->setAlwaysOnTop(true);
			tableEditor->setUseFlatDesign(true);

			tableEditor->setSpecialLookAndFeel(new EnvelopeLaf(), true);

			auto c = getColourForEnvelope(m);

			tableEditor->setColour(TableEditor::ColourIds::bgColour, Colours::transparentBlack);
			tableEditor->setColour(TableEditor::ColourIds::fillColour, c.withAlpha(0.1f));
			tableEditor->setColour(TableEditor::ColourIds::lineColour, c);

			table->setXTextConverter([p](float v)
			{
				return getText(*p, v);
			});

			tableEditor->addMouseListener(getWaveform(), false);

			resized();
			return;
		}
		else
		{
			tableEditor = nullptr;
			resized();
			return;
		}
	}
}

void WaterfallComponent::LookAndFeelMethods::drawWavetableBackground(Graphics& g, WaterfallComponent& wc, bool isEmpty)
{
	auto bgColour = wc.findColour(HiseColourScheme::ColourIds::ComponentBackgroundColour);
	auto bounds = wc.getLocalBounds().toFloat();

	g.fillAll(bgColour);

	auto borderColour = wc.findColour(HiseColourScheme::ColourIds::ComponentOutlineColourId);
	g.setColour(borderColour);
	g.drawRect(bounds, 1.0f);

	if (isEmpty)
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(wc.findColour(HiseColourScheme::ColourIds::ComponentTextColourId).withMultipliedAlpha(0.1f));
		g.drawText("No preview available", bounds, Justification::centred);
		return;
	}
}

void WaterfallComponent::LookAndFeelMethods::drawWavetablePath(Graphics& g, WaterfallComponent& wc, const Path& p, int tableIndex, bool isStereo, int currentTableIndex, int numTables)
{
	float thisAlpha = 1.0f - jlimit(0.0f, 1.0f, (float)hmath::abs(tableIndex - currentTableIndex) / (float)(numTables));

	auto alpha = hmath::pow(0.988f, (float)tableIndex);

	thisAlpha = jmax(0.08f, hmath::pow(thisAlpha, 8.0f)*0.5f);
	thisAlpha *= alpha;

	if (tableIndex == currentTableIndex)
	{
		thisAlpha = 1.0f;

		if (isStereo)
		{
			g.setColour(wc.findColour(HiseColourScheme::ColourIds::ComponentTextColourId));
			p.getBounds();

			g.setFont(GLOBAL_BOLD_FONT());
			auto pb = p.getBounds();

			g.drawText("L    R", pb, Justification::centredTop);
			g.drawVerticalLine(pb.getCentreX(), pb.getY(), pb.getBottom());
		}

	}

	if (tableIndex != currentTableIndex && (tableIndex % 2) != 0)
	{
		return;
	}

	auto c = wc.findColour(HiseColourScheme::ColourIds::ComponentFillTopColourId).withMultipliedAlpha(thisAlpha);

	g.setColour(c);

	g.strokePath(p, PathStrokeType(tableIndex == currentTableIndex ? 2.0f : 1.0f));
}

WaterfallComponent::WaterfallComponent(MainController* mc, ReferenceCountedObjectPtr<WavetableSound> sound_) :
	SimpleTimer(mc->getGlobalUIUpdater()),
	ControlledObject(mc),
	sound(sound_),
	displacement(0.25f, 1.0f)
{
	start();
	setOpaque(true);

	setLookAndFeel(&defaultLaf);

	setColour(HiseColourScheme::ColourIds::ComponentBackgroundColour, Colour(0xFF222222));
	setColour(HiseColourScheme::ColourIds::ComponentFillTopColourId, Colours::white);
	setColour(HiseColourScheme::ColourIds::ComponentOutlineColourId, Colours::white.withAlpha(0.05f));
	setColour(HiseColourScheme::ColourIds::ComponentTextColourId, Colours::white.withAlpha(0.5f));
}




void WaterfallComponent::rebuildPaths()
{
	Array<Path> newPaths;

	if (auto first = sound.get())
	{
		auto numTables = first->getWavetableAmount();
		const auto realNumTables = numTables;

		float tableStride = jmax(1.0f, (float)numTables / 64.0f);
		numTables = jmin(numTables, 64);

		auto size = first->getTableSize();

		stereo = first->isStereo();

		if (stereo)
			size *= 2;

		auto b = getLocalBounds().reduced(5).toFloat();

		b.removeFromTop((float)numTables * displacement.getY());
		b.removeFromRight((float)numTables * displacement.getX());

		float maxGain = 0.0f;

		for (int i = 0; i < numTables; i++)
		{
			maxGain = jmax(maxGain, first->getUnnormalizedGainValue(i));
		}

		HeapBlock<float> data;
		data.calloc(size);

		auto reversed = first->isReversed();

		if (maxGain != 0.0f)
		{
			for (float pi = 0.0f; pi < realNumTables; pi += tableStride)
			{
				Path p;

				auto thisBounds = b.translated((float)pi* displacement.getX() / (float)tableStride, -1.0f * displacement.getY() * pi / tableStride);

				auto tableIndex = reversed ? (realNumTables - (int)pi - 1) : (int)pi;

				auto l = first->getWaveTableData(0, tableIndex);
				FloatVectorOperations::copy(data.get(), l, first->getTableSize());

				if (stereo)
				{
					auto r = first->getWaveTableData(1, tableIndex);
					FloatVectorOperations::copy(data.get() + first->getTableSize(), r, first->getTableSize());
				}

				p.startNewSubPath(thisBounds.getX(), thisBounds.getY());
				p.startNewSubPath(thisBounds.getX(), thisBounds.getBottom());

				p.startNewSubPath(thisBounds.getX(), thisBounds.getCentreY());

				auto gain = first->getUnnormalizedGainValue(tableIndex);

				if (gain == 0.0f)
					continue;

				//gain /= maxGain;

				gain = 1.0f / gain;
				//gain = hmath::pow(maxGain, 0.8f);

				for (int i = 0; i < b.getWidth(); i += 2)
				{
					auto uptime = ((float)i / thisBounds.getWidth()) * (float)size;



					int pos = (int)uptime;
					pos = jlimit(0, size - 1, pos);

					int nextPos = jlimit(0, size - 1, pos + 1);

					auto alpha = uptime - (float)pos;

					auto value = Interpolator::interpolateLinear(data[pos], data[nextPos], alpha);

					jassert(hmath::abs(value) <= 1.0f);

					p.lineTo(thisBounds.getX() + (float)i,
						thisBounds.getY() + thisBounds.getHeight() * 0.5f * (1.0f - value * gain));
				}

				p.lineTo(thisBounds.getRight(), thisBounds.getCentreY());

				newPaths.add(p);
			}
		}
	}

	std::swap(paths, newPaths);
	repaint();
}

void WaterfallComponent::paint(Graphics& g)
{
	auto lafToUse = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());

	if (lafToUse == nullptr)
		lafToUse = &defaultLaf;

	lafToUse->drawWavetableBackground(g, *this, paths.isEmpty());

	int idx = 0;

	for (const auto& p : paths)
	{
		lafToUse->drawWavetablePath(g, *this, p, idx++, stereo, currentTableIndex, paths.size());
	}
}

void WaterfallComponent::timerCallback()
{
	if (!displayDataFunction)
		jassertfalse;

	auto df = displayDataFunction();

	float modValue = df.modValue;

	auto thisIndex = roundToInt(modValue * (paths.size() - 1));

	if (sound != df.sound)
	{
		sound = df.sound;
		rebuildPaths();
	}

	if (currentTableIndex != thisIndex)
	{
		currentTableIndex = thisIndex;
		repaint();
	}
}

void WaterfallComponent::resized()
{
	rebuildPaths();
}

void WaterfallComponent::setPerspectiveDisplacement(const Point<float>& newDisplacement)
{
	if (displacement != newDisplacement)
	{
		displacement = newDisplacement;
		repaint();
	}
}

WaterfallComponent::Panel::Panel(FloatingTile* parent):
	PanelWithProcessorConnection(parent)
{
	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF222222));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.05f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white.withAlpha(0.5f));
}

juce::Identifier WaterfallComponent::Panel::getProcessorTypeId() const
{
	return WavetableSynth::getClassType();
}

Component* WaterfallComponent::Panel::createContentComponent(int index)
{
	if (auto wt = dynamic_cast<WavetableSynth*>(getProcessor()))
	{
		if (auto sound = dynamic_cast<WavetableSound*>(wt->getSound(index)))
		{
			auto wc = new WaterfallComponent(getMainController(), sound);

			auto currentIndex = index;
			WeakReference<ModulatorSynth> safeThis(wt);

			auto bgColour = findPanelColour(FloatingTileContent::PanelColourId::bgColour);

			wc->setOpaque(bgColour.isOpaque());
			wc->setColour(HiseColourScheme::ComponentBackgroundColour, bgColour);
			wc->setColour(HiseColourScheme::ComponentFillTopColourId, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
			wc->setColour(HiseColourScheme::ComponentOutlineColourId, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
			wc->setColour(HiseColourScheme::ComponentTextColourId, findPanelColour(FloatingTileContent::PanelColourId::textColour));

			wc->displayDataFunction = [safeThis, currentIndex]()
			{
				WaterfallComponent::DisplayData data;

				if (safeThis != nullptr)
				{
					data.sound = dynamic_cast<WavetableSound*>(safeThis->getSound(currentIndex));
					data.modValue = static_cast<WavetableSynth*>(safeThis.get())->getDisplayTableValue();
				}

				return data;
			};

			return wc;
		}
	}

	return nullptr;
}

int WaterfallComponent::Panel::getNumDefaultableProperties() const
{
	return (int)SpecialPanelIds::numSpecialPanelIds;
}

juce::Identifier WaterfallComponent::Panel::getDefaultablePropertyId(int index) const
{
	if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
		return PanelWithProcessorConnection::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Displacement, "Displacement");

	jassertfalse;
	return {};
}

juce::var WaterfallComponent::Panel::getDefaultProperty(int index) const
{
	if (isPositiveAndBelow(index, PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds))
		return PanelWithProcessorConnection::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Displacement, Array<var>(var(0.25), var(1.0)));

	jassertfalse;

	return {};
}

void WaterfallComponent::Panel::fromDynamicObject(const var& object)
{
	PanelWithProcessorConnection::fromDynamicObject(object);

	if (auto wc = getContent<WaterfallComponent>())
	{
		auto dp = getPropertyWithDefault(object, (int)SpecialPanelIds::Displacement);

		auto displacement = ApiHelpers::getPointFromVar(dp, nullptr);

		wc->setPerspectiveDisplacement(displacement);
	}
}

juce::var WaterfallComponent::Panel::toDynamicObject() const
{
	auto obj = PanelWithProcessorConnection::toDynamicObject();

	if (auto wc = getContent<WaterfallComponent>())
	{
		storePropertyInObject(obj, (int)SpecialPanelIds::Displacement, ApiHelpers::getVarFromPoint(wc->displacement));
	}

	return obj;
}

void WaterfallComponent::Panel::fillModuleList(StringArray& moduleList)
{
	moduleList = ProcessorHelpers::getAllIdsForType<WavetableSynth>(getMainController()->getMainSynthChain());
}

void WaterfallComponent::Panel::fillIndexList(StringArray& indexList)
{
	if (auto wt = dynamic_cast<WavetableSynth*>(getProcessor()))
	{
		for (int i = 0; i < wt->getNumSounds(); i++)
		{
			if (auto sound = dynamic_cast<WavetableSound*>(wt->getSound(i)))
			{
				indexList.add(MidiMessage::getMidiNoteName(sound->getRootNote(), true, true, true));
			}
		}
	}
}


}
