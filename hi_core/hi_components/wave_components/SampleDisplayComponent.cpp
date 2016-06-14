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

#include "SampleDisplayComponent.h"

SamplerSoundWaveform::SamplerSoundWaveform(const ModulatorSampler *ownerSampler):
	AudioDisplayComponent(ownerSampler->getCache()),
	sampler(ownerSampler),
	sampleStartPosition(-1.0),
	currentSound(nullptr)
{
	areas.add(new SampleArea(PlayArea, this));
	areas.add(new SampleArea(SampleStartArea, this));
	areas.add(new SampleArea(LoopArea, this));
	areas.add(new SampleArea(LoopCrossfadeArea, this));

	setColour(Slider::backgroundColourId, Colour(0xFF383838));

	addAndMakeVisible(areas[PlayArea]);
	areas[PlayArea]->addAndMakeVisible(areas[SampleStartArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopArea]);
	areas[PlayArea]->addAndMakeVisible(areas[LoopCrossfadeArea]);
	
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
	if(sampler->getLastStartedVoice() != nullptr)
	{
		ModulatorSamplerSound *s = dynamic_cast<ModulatorSamplerVoice*>(sampler->getLastStartedVoice())->getCurrentlyPlayingSamplerSound();

		if(s == currentSound)
		{
			sampleStartPosition = sampler->getSamplerDisplayValues().currentSampleStartPos;
			setPlaybackPosition(sampler->getSamplerDisplayValues().currentSamplePos);
		}
	}
	
};


void SamplerSoundWaveform::updateRanges(SampleArea *areaToSkip)
{
	if(currentSound != nullptr)
	{
		areas[PlayArea]->setSampleRange(Range<int>(currentSound->getProperty(ModulatorSamplerSound::SampleStart),
											  currentSound->getProperty(ModulatorSamplerSound::SampleEnd)));

		
		areas[PlayArea]->setAllowedPixelRanges(currentSound->getPropertyRange(ModulatorSamplerSound::SampleStart),
											   currentSound->getPropertyRange(ModulatorSamplerSound::SampleEnd));



		areas[LoopArea]->setVisible(currentSound->getProperty(ModulatorSamplerSound::LoopEnabled));

		areas[LoopArea]->setSampleRange(Range<int>(currentSound->getProperty(ModulatorSamplerSound::LoopStart),
								  currentSound->getProperty(ModulatorSamplerSound::LoopEnd)));

		areas[LoopArea]->setAllowedPixelRanges(currentSound->getPropertyRange(ModulatorSamplerSound::LoopStart),
											   currentSound->getPropertyRange(ModulatorSamplerSound::LoopEnd));

		const int64 start = (int64)currentSound->getProperty(ModulatorSamplerSound::LoopStart) - (int64)currentSound->getProperty(ModulatorSamplerSound::LoopXFade);
		
		areas[LoopCrossfadeArea]->setSampleRange(Range<int>((int)start, currentSound->getProperty(ModulatorSamplerSound::LoopStart)));

		areas[SampleStartArea]->setSampleRange(Range<int>(currentSound->getProperty(ModulatorSamplerSound::SampleStart), (int)currentSound->getProperty(ModulatorSamplerSound::SampleStart) + (int)currentSound->getProperty(ModulatorSamplerSound::SampleStartMod)));

		areas[SampleStartArea]->setAllowedPixelRanges(currentSound->getPropertyRange(ModulatorSamplerSound::SampleStart),
			currentSound->getPropertyRange(ModulatorSamplerSound::SampleStartMod));

	}

	refreshSampleAreaBounds(areaToSkip);

	//AudioDisplayComponent::updateRanges(skipArea);
}

void SamplerSoundWaveform::toggleRangeEnabled(AreaTypes type)
{
	areas[type]->toggleEnabled();
}

void AudioDisplayComponent::drawWaveForm(Graphics &g)
{
    
    
	if(preview->getTotalLength() == 0.0) return; // Nothing to draw

	g.setGradientFill (ColourGradient (Colour (0xaaffffff),
							0.0f, 0.0f,
							Colour (0x88ffffff),
							0.0f, (float) getHeight(),
							false));

	const double totalSamples = getTotalSampleAmount();

	const float normalizedGain = getNormalizedPeak();

	// Draw the pre part

	const double prePart = areas[0]->getSampleRange().getStart() / totalSamples;
	const double preSeconds = prePart * preview->getTotalLength();

	const Rectangle<int> preArea = Rectangle<int>(0, 0, (int)(prePart * getWidth()), getHeight());

	

	g.setColour(Colours::white.withAlpha(0.3f));

	preview->drawChannels(g, preArea, 0.0, preSeconds, normalizedGain);

	// Draw the play part

	const double playPart = areas[0]->getSampleRange().getLength() / totalSamples;
	const double playSeconds = playPart * preview->getTotalLength();

	const Rectangle<int> playArea = Rectangle<int>(preArea.getRight(), 0, (int)(playPart * getWidth()), getHeight());

	g.setColour(Colours::white.withAlpha(0.8f));

	preview->drawChannels(g, playArea, preSeconds, preSeconds + playSeconds, normalizedGain);

	// Draw the post part

	const double postPart = (totalSamples - areas[0]->getSampleRange().getEnd()) / totalSamples;

	const Rectangle<int> postArea = Rectangle<int>(playArea.getRight(), 0, (int)(postPart * getWidth()), getHeight());

	g.setColour(Colours::white.withAlpha(0.3f));

	preview->drawChannels(g, postArea, preSeconds + playSeconds, preview->getTotalLength(), normalizedGain);
}

double SamplerSoundWaveform::getSampleRate() const
{
	return currentSound != nullptr ? currentSound->getSampleRate() : -1.0;
}

void AudioDisplayComponent::drawPlaybackBar(Graphics &g)
{
	if(getSampleArea(0)->getWidth() != 0)
	{

		NormalisableRange<double> range((double)getSampleArea(0)->getX(), (double)getSampleArea(0)->getRight());

		playBackPosition = jlimit<double>(0.0, 1.0, playBackPosition);

		const int x = (int)(range.convertFrom0to1(playBackPosition));

		g.setColour(Colours::lightgrey.withAlpha(0.05f));
		g.fillRect((float)x, 0.0f, x == 0 ? 5.0f : 10.0f, (float)getHeight());
		
		//g.fillRect(jmax(0.0f, value * (float)getWidth()-5.0f), 0.0f, value == 0.0f ? 5.0f : 10.0f, (float)getHeight());
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawLine(Line<float>((float)x, 0.0f, (float)x, (float)getHeight()), 0.5f);

	}
}

void SamplerSoundWaveform::drawSampleStartBar(Graphics &g)
{
	if(sampleStartPosition != -1.0)
	{
		g.setColour(Colours::darkblue.withAlpha(0.5f));
			
		const int x = areas[PlayArea]->getX() + (int)(sampleStartPosition * areas[SampleStartArea]->getWidth());

		g.drawLine((float)x, 0.0f, (float)x, (float)getBottom() - 2.0f, 1.0f);

		g.setColour(Colours::blue.withAlpha(0.1f));

		g.fillRect(x - 5, 0, 10, getHeight());
	}
}

void SamplerSoundWaveform::paint(Graphics &g)
{
	g.fillAll(findColour(Slider::backgroundColourId));

	AudioDisplayComponent::paint(g);

	if(getTotalSampleAmount() == 0) return;

	if(areas[SampleStartArea]->getSampleRange().getLength() != 0)
	{
		drawSampleStartBar(g);
	};
}

void SamplerSoundWaveform::setSoundToDisplay(const ModulatorSamplerSound *s)
{
	if(s != nullptr && !s->isMissing() && !s->isPurged())
	{
		currentSound = const_cast<ModulatorSamplerSound*>(s);

		WavAudioFormat waf;

		AudioFormatReader *afr = PresetHandler::getReaderForFile(File(s->getProperty(ModulatorSamplerSound::FileName)));

		jassert(afr != nullptr);

		preview->setReader(afr, (int64)s->getProperty(ModulatorSamplerSound::ID));

		numSamplesInCurrentSample = (int)afr->lengthInSamples;

		updateRanges();

	}
	else
	{
		currentSound = nullptr;

		for(int i = 0; i < areas.size(); i++)
		{
			areas[i]->setBounds(0,0,0,0);
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

// =================================================================================================================== SampleArea

AudioDisplayComponent::SampleArea::SampleArea(int area_, AudioDisplayComponent *parentWaveform_):
		area(area_),
		areaEnabled(true),
		useConstrainer(false),
		parentWaveform(parentWaveform_)
{
	setInterceptsMouseClicks(false, true);
	edgeLaf = new EdgeLookAndFeel(this);
		
	addAndMakeVisible(leftEdge = new ResizableEdgeComponent(this, nullptr, ResizableEdgeComponent::leftEdge));
	addAndMakeVisible(rightEdge = new ResizableEdgeComponent(this, nullptr, ResizableEdgeComponent::rightEdge));

	setAreaEnabled(false);

	leftEdge->setLookAndFeel(edgeLaf);
	rightEdge->setLookAndFeel(edgeLaf);

	leftEdge->addMouseListener(this, true);
	rightEdge->addMouseListener(this, true);

	

};

void AudioDisplayComponent::SampleArea::mouseUp(const MouseEvent &e)
{
	checkBounds();

	const int  dragEndWidth = getWidth();

	leftEdgeClicked = e.eventComponent == leftEdge;

	if(dragEndWidth != prevDragWidth)
	{
		range.setStart(getSampleForX(getX(), false));
		range.setEnd(getSampleForX(getRight(), false));

		parentWaveform->sendAreaChangedMessage();
	}
}

void AudioDisplayComponent::SampleArea::mouseDown(const MouseEvent &e)
{
	SET_CHANGED_FROM_PARENT_EDITOR()

	prevDragWidth = getWidth();
	leftEdgeClicked = e.eventComponent == leftEdge;

	parentWaveform->setCurrentArea(this);

}

void AudioDisplayComponent::SampleArea::mouseDrag(const MouseEvent &)
{
	checkBounds();

	parentWaveform->refreshSampleAreaBounds(this);
};

void AudioDisplayComponent::SampleArea::paint(Graphics &g)
{
   
	if(area == SamplerSoundWaveform::LoopCrossfadeArea)
	{
		Path fadeInPath;

		fadeInPath.startNewSubPath(0.0f, (float)getHeight());
		fadeInPath.lineTo((float)getWidth(), 0.0f);
		fadeInPath.lineTo((float)getWidth(), (float)getHeight());
		fadeInPath.closeSubPath();

		g.setColour(getAreaColour().withAlpha(areaEnabled ? 0.1f : 0.02f));
		g.fillPath(fadeInPath);

		g.setColour(getAreaColour().withAlpha(0.3f));
		PathStrokeType stroke(1.0f);
		g.strokePath(fadeInPath, stroke);
	}
	else
	{
		g.setColour(getAreaColour().withAlpha(areaEnabled ? 0.1f : 0.02f));
		g.fillAll();

		g.setColour(getAreaColour().withAlpha(0.3f));
		g.drawRect(getLocalBounds(), 1);
	}

}

void AudioDisplayComponent::SampleArea::checkBounds()
{
	int x = getX();
	int r = getRight();
	int y = 0;
	int w = getWidth();
	int h = getHeight();

	if(w < 2*EDGE_WIDTH)
	{
		setBounds((leftEdgeClicked ? (r - 2*EDGE_WIDTH) : x), y, 2*EDGE_WIDTH, h);	
	}

	if(x < 0)
	{
		x = 0;
		w = r;
		setBounds(x,y,w,h);
	}

	if(r > getParentComponent()->getWidth())
	{
		w = getParentComponent()->getWidth() - x;
		setBounds(x,y,w,h);
	}

	if(useConstrainer && (x < leftEdgeRangeInPixels.getStart()))
	{
		x = leftEdgeRangeInPixels.getStart();

		w = r - x;

		setBounds(x,y,w,h);
	}
	else if(useConstrainer && (x > leftEdgeRangeInPixels.getEnd()))
	{
		x = leftEdgeRangeInPixels.getEnd();

		w = r - x;

		setBounds(x,y,w,h);
	}
	else if(useConstrainer && (r < rightEdgeRangeInPixels.getStart()))
	{
		w = rightEdgeRangeInPixels.getStart() - x;

		setBounds(x,y,w,h);
	}
	else if(useConstrainer && (r > rightEdgeRangeInPixels.getEnd()))
	{
		w = rightEdgeRangeInPixels.getEnd() - x;

		setBounds(x,y,w,h);
	}

}

void AudioDisplayComponent::SampleArea::resized()
{
	leftEdge->setBounds(0, 0, EDGE_WIDTH, getHeight());

	rightEdge->setBounds(getWidth() - EDGE_WIDTH, 0, EDGE_WIDTH, getHeight());
}

Colour AudioDisplayComponent::SampleArea::getAreaColour() const
{
	switch(area)
	{
	case SamplerSoundWaveform::PlayArea:			return Colours::white;
	case SamplerSoundWaveform::SampleStartArea:		return Colours::blue;
	case SamplerSoundWaveform::LoopArea:			return Colours::green;
	case SamplerSoundWaveform::LoopCrossfadeArea:	return Colours::yellow;
	default:				jassertfalse;			return Colours::transparentBlack;
	}
}

#pragma warning( push )
#pragma warning( disable: 4100 )

void AudioDisplayComponent::SampleArea::EdgeLookAndFeel::drawStretchableLayoutResizerBar (Graphics &g, 
																   int w, int h, 
																   bool isVerticalBar, 
																   bool isMouseOver, 
																   bool isMouseDragging)
{
	jassert(isVerticalBar);

	if(isMouseDragging)
	{
		g.setColour(Colours::white.withAlpha(0.3f));
		g.fillAll();

		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawVerticalLine(parentArea->leftEdgeClicked ? 0 : w - 1, 0.0f, (float)h);
	}
	else
	{
		g.setColour(parentArea->getAreaColour().withAlpha(isMouseOver ? 0.2f : 0.0f));
		g.fillAll();
	}
};

#pragma warning( pop )

AudioSampleBufferComponent::AudioSampleBufferComponent(AudioThumbnailCache &cache) :
AudioDisplayComponent(cache),
buffer(nullptr),
itemDragged(false),
bgColour(Colour(0xFF555555))
{
	setOpaque(true);

	areas.add(new SampleArea(AreaTypes::PlayArea, this));
	addAndMakeVisible(areas[0]);
	areas[0]->setAreaEnabled(true);
}

bool AudioSampleBufferComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
	Component *c = dragSourceDetails.sourceComponent.get();

	if (dynamic_cast<FileTreeComponent*>(c) != nullptr)
	{
		String firstName = dragSourceDetails.description.toString();

		return isAudioFile(firstName);
	}

	if (c == this) return false;

#if USE_BACKEND
	const bool dragSourceIsTable = c != nullptr && dynamic_cast<ExternalFileTable<AudioSampleBuffer>*>(c->getParentComponent());
#else
	const bool dragSourceIsTable = false;

#endif

	const bool dragSourceIsOtherDisplayComponent = dynamic_cast<AudioSampleBufferComponent*>(c) != nullptr;

	return dragSourceIsTable || dragSourceIsOtherDisplayComponent;
}



void AudioSampleBufferComponent::changeListenerCallback(SafeChangeBroadcaster *b)
{
	AudioSampleProcessor *asp = dynamic_cast<AudioSampleProcessor*>(b);

	if (asp != nullptr && asp->getBuffer() != buffer)
	{
		setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());
	}

	repaint();
}

void AudioSampleBufferComponent::mouseDown(const MouseEvent &e)
{
	if (e.mods.isRightButtonDown())
	{
		SET_CHANGED_FROM_PARENT_EDITOR()
        
		String patterns = "*.wav;*.aif;*.aiff;*.WAV;*.AIFF";

#if USE_BACKEND

		File searchDirectory;

		if (ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>())
		{
			searchDirectory = GET_PROJECT_HANDLER(editor->getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);
		}
		else
		{
			searchDirectory = File::nonexistent;
		}
		
#else
		File searchDirectory = File::nonexistent;
#endif

		FileChooser fc("Load File", searchDirectory, patterns, true);

		if (fc.browseForFileToOpen())
		{
			currentFileName = fc.getResult().getFullPathName();
			sendSynchronousChangeMessage();
		}
	}
}

void AudioSampleBufferComponent::paint(Graphics &g)
{
	if(isOpaque()) g.fillAll(bgColour);

	AudioDisplayComponent::paint(g);

	g.setFont(GLOBAL_FONT());
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawText(ProjectHandler::isAbsolutePathCrossPlatform(currentFileName) ? File(currentFileName).getFileName() : currentFileName, getWidth() - 400, 0, 395, 20, Justification::centredRight);

	if (itemDragged)
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawRect(getLocalBounds(), 4);
	}
}
