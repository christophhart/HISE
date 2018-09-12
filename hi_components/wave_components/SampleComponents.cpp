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

HiseAudioSampleBufferComponent::HiseAudioSampleBufferComponent(SafeChangeBroadcaster* p) :
	AudioSampleBufferComponentBase(p)
{
	updateProcessorConnection();

	if (connectedProcessor != nullptr)
	{
		removeAllChangeListeners();
		connectedProcessor->addChangeListener(this);
		changeListenerCallback(connectedProcessor);
	}
}




bool HiseAudioSampleBufferComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	PoolReference ref(dragSourceDetails.description);
	return ref && ref.getFileType() == FileHandlerBase::SubDirectories::AudioFiles;
}

void HiseAudioSampleBufferComponent::updatePlaybackPosition()
{
	if (connectedProcessor)
		setPlaybackPosition(dynamic_cast<Processor*>(connectedProcessor.get())->getInputValue());
}

juce::File HiseAudioSampleBufferComponent::getDefaultDirectory() const
{
#if USE_BACKEND
	File searchDirectory;

	if (ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>())
		searchDirectory = GET_PROJECT_HANDLER(editor->getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);
	else
		searchDirectory = File();
#else
	File searchDirectory = FrontendHandler::getAdditionalAudioFilesDirectory();
#endif
	return searchDirectory;
}

void HiseAudioSampleBufferComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
	PoolReference ref(dragSourceDetails.description);
	poolItemWasDropped(ref);
}

void HiseAudioSampleBufferComponent::loadFile(const File& f)
{
	if (auto asp = dynamic_cast<AudioSampleProcessor*>(connectedProcessor.get()))
	{
#if USE_BACKEND
		String fileName = f.getFullPathName();
#else
		auto fileName = FrontendHandler::getRelativePathForAdditionalAudioFile(f);
#endif

		buffer = nullptr;
		asp->setLoadedFile(fileName, true);
	}
}

void HiseAudioSampleBufferComponent::newBufferLoaded()
{
	AudioSampleProcessor *asp = dynamic_cast<AudioSampleProcessor*>(connectedProcessor.get());

	if (asp != nullptr)
	{
		setAudioSampleBuffer(asp->getBuffer(), asp->getFileName(), dontSendNotification);
		setRange(asp->getActualRange());

		auto loopRange = asp->getLoopRange().isEmpty() ? asp->getRange() : asp->getLoopRange();

		int x1 = getSampleArea(0)->getXForSample(jmax<int>(asp->getRange().getStart(), loopRange.getStart()));
		int x2 = getSampleArea(0)->getXForSample(jmin<int>(asp->getRange().getEnd(), loopRange.getEnd()));

		xPositionOfLoop = { x1, x2 };
		setShowLoop(asp->isUsingLoop());


	}
}

void HiseAudioSampleBufferComponent::updateProcessorConnection()
{
	if (auto asp = dynamic_cast<AudioSampleProcessor*>(connectedProcessor.get()))
	{
		setAudioSampleBuffer(asp->getBuffer(), asp->getFileName(), dontSendNotification);
		setRange(asp->getRange());
	}
}

void HiseAudioSampleBufferComponent::poolItemWasDropped(const PoolReference& /*ref*/)
{

}

void HiseAudioSampleBufferComponent::mouseDoubleClick(const MouseEvent&)
{
	if (auto asp = dynamic_cast<AudioSampleProcessor*>(connectedProcessor.get()))
	{
		buffer = nullptr;
		asp->setLoadedFile("", true);
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

	areas[PlayArea]->setAreaEnabled(true);

	areas[SampleStartArea]->leftEdge->setVisible(false);
	areas[LoopCrossfadeArea]->rightEdge->setVisible(false);

	setOpaque(true);

#ifdef JUCE_DEBUG

	startTimer(30);

#else

	startTimer(30);

#endif


};

SamplerSoundWaveform::~SamplerSoundWaveform()
{

}



void SamplerSoundWaveform::timerCallback()
{
	if (sampler->getLastStartedVoice() != nullptr)
	{
		ModulatorSamplerSound *s = dynamic_cast<ModulatorSamplerVoice*>(sampler->getLastStartedVoice())->getCurrentlyPlayingSamplerSound();

		if (s == currentSound)
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

	AudioDisplayComponent::paint(g);

	if (getTotalSampleAmount() == 0) return;

	if (areas[SampleStartArea]->getSampleRange().getLength() != 0)
	{
		drawSampleStartBar(g);
	};

	if (!onInterface && currentSound.get() != nullptr)
	{
		if (currentSound->getReferenceToSound()->isMonolithic())
		{
			g.setColour(Colour(0x22000000));
			g.fillRect(0, 0, 80, 20);
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
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
	if (s != nullptr && !s->isMissing() && !s->isPurged())
	{
		currentSound = const_cast<ModulatorSamplerSound*>(s);

		StreamingSamplerSound::Ptr sound = s->getReferenceToSound(multiMicIndex);

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

			preview->setReader(afr.release(), numSamplesInCurrentSample);

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



float SamplerSoundWaveform::getNormalizedPeak()
{
	const ModulatorSamplerSound *s = getCurrentSound();

	if (s != nullptr)
	{
		return s->getNormalizedPeak();
	}
	else return 1.0f;
}

}