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

namespace hise { using namespace juce;



void AudioDisplayComponent::drawPlaybackBar(Graphics &g)
{
	if(playBackPosition > 0.0 && getSampleArea(0)->getWidth() != 0)
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

void AudioDisplayComponent::paint(Graphics &g)
{
	//ProcessorEditorLookAndFeel::drawNoiseBackground(g, getLocalBounds(), Colours::darkgrey);

	g.setColour(Colours::lightgrey.withAlpha(0.1f));
	//g.drawRect(getLocalBounds(), 1);

	if (preview->getTotalLength() == 0.0) return;

	if (auto numSamples = getTotalSampleAmount())
	{
		const int widthPerSample = getWidth() / numSamples;

		if (widthPerSample >= 10)
		{
			for (int i = 0; i < numSamples; i++)
			{
				auto x = getSampleArea(0)->getXForSample(i);
				g.setColour(Colours::white.withAlpha(0.05f));
				g.drawVerticalLine(x, 0.0f, (float)getHeight());
			}
		}
	}


	drawPlaybackBar(g);
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
		
	addAndMakeVisible(leftEdge = new AreaEdge(this, nullptr, ResizableEdgeComponent::leftEdge));
	addAndMakeVisible(rightEdge = new AreaEdge(this, nullptr, ResizableEdgeComponent::rightEdge));

	setAreaEnabled(false);

	leftEdge->setLookAndFeel(edgeLaf);
	rightEdge->setLookAndFeel(edgeLaf);

	leftEdge->addMouseListener(this, true);
	rightEdge->addMouseListener(this, true);

	

};

AudioDisplayComponent::SampleArea::~SampleArea()
{
	leftEdge->setLookAndFeel(nullptr);
	rightEdge->setLookAndFeel(nullptr);
}

void AudioDisplayComponent::SampleArea::mouseUp(const MouseEvent &e)
{
	checkBounds();

	const int  dragEndWidth = getWidth();

	leftEdgeClicked = e.eventComponent == leftEdge;

	if(dragEndWidth != prevDragWidth)
	{
		if (leftEdgeClicked)
			range.setStart(getSampleForX(getX(), false));
		else
			range.setEnd(getSampleForX(getRight(), false));


		parentWaveform->sendAreaChangedMessage();
	}
}

int AudioDisplayComponent::SampleArea::getXForSample(int sample, bool relativeToAudioDisplayComponent/*=false*/) const
{
	const double proportion = jmin<double>(1.0, (double)sample / (double)(parentWaveform->getTotalSampleAmount()-1));
	int xInWaveform = roundToInt((double)parentWaveform->getWidth() * proportion);
	


	const int xInParent = getParentComponent()->getLocalPoint(parentWaveform, Point<int>(xInWaveform, 0)).getX();

	return relativeToAudioDisplayComponent ? xInWaveform : xInParent;
}

int AudioDisplayComponent::SampleArea::getSampleForX(int x, bool relativeToAudioDisplayComponent/*=false*/) const
{
	// This will be useless if the width is zero!
	jassert(parentWaveform->getWidth() != 0);

	if (!relativeToAudioDisplayComponent)
		x = parentWaveform->getLocalPoint(getParentComponent(), Point<int>(x, 0)).getX();

	const int widthOfWaveForm = parentWaveform->getWidth();
	const double proportion = (double)x / (double)widthOfWaveForm;
	const int sample = (int)((double)proportion * parentWaveform->getTotalSampleAmount());

	return sample;
}

void AudioDisplayComponent::SampleArea::mouseDown(const MouseEvent &e)
{
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
	
	if(area == AreaTypes::LoopCrossfadeArea)
	{
		Path fadeInPath;

		fadeInPath.startNewSubPath(0.0f, (float)getHeight());
		fadeInPath.lineTo((float)getWidth(), 0.0f);
		fadeInPath.lineTo((float)getWidth(), (float)getHeight());
		fadeInPath.closeSubPath();

		g.setColour(getAreaColour().withAlpha(areaEnabled ? 0.1f : 0.05f));
		g.fillPath(fadeInPath);

		g.setColour(getAreaColour().withAlpha(0.3f));
		PathStrokeType stroke(1.0f);
		g.strokePath(fadeInPath, stroke);
	}
	else if (area == AreaTypes::PlayArea)
	{
		g.setColour(getAreaColour().withAlpha(areaEnabled ? 0.1f : 0.02f));
		g.fillAll();

		g.setColour(getAreaColour().withAlpha(0.3f));
		g.drawRect(getLocalBounds(), 1);
	}
	else
	{
		g.setColour(getAreaColour().withAlpha(areaEnabled ? 0.15f : 0.06f));
		g.fillAll();

		g.setColour(getAreaColour().withAlpha(0.3f));
		g.drawRect(getLocalBounds(), 1);

		g.setColour(getAreaColour());
		g.drawVerticalLine(0, 0.0, (float)getHeight());
		g.drawVerticalLine(getWidth() - 1, 0.0, (float)getHeight());
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
	case AreaTypes::PlayArea:			return Colours::white;
	case AreaTypes::SampleStartArea:	return Colours::blue;
	case AreaTypes::LoopArea:			return Colours::green;
	case AreaTypes::LoopCrossfadeArea:	return Colours::yellow;
	default:		jassertfalse;		return Colours::transparentBlack;
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

AudioSampleBufferComponentBase::AudioSampleBufferComponentBase(SafeChangeBroadcaster* p) :
	AudioDisplayComponent(),
	buffer(nullptr),
	itemDragged(false),

	bgColour(Colour(0xFF555555)),
	connectedProcessor(p)
{
	setColour(ColourIds::bgColour, Colour(0xFF555555));

	setOpaque(true);

	areas.add(new SampleArea(AreaTypes::PlayArea, this));
	addAndMakeVisible(areas[0]);
	areas[0]->setAreaEnabled(true);

	setAudioSampleProcessor(p);

	static const unsigned char pathData[] = { 110,109,0,23,2,67,128,20,106,67,108,0,0,230,66,128,92,119,67,108,0,23,2,67,128,82,130,67,108,0,23,2,67,128,92,123,67,108,0,0,7,67,128,92,123,67,98,146,36,8,67,128,92,123,67,243,196,9,67,58,18,124,67,128,5,11,67,128,88,125,67,98,13,70,12,67,198,158,126,
		67,0,0,13,67,53,39,128,67,0,0,13,67,64,197,128,67,98,0,0,13,67,109,97,129,67,151,72,12,67,91,58,130,67,0,11,11,67,128,221,130,67,98,105,205,9,67,165,128,131,67,219,50,8,67,0,220,131,67,0,0,7,67,0,220,131,67,108,0,0,210,66,0,220,131,67,98,30,54,205,66,
		0,220,131,67,0,0,198,66,234,39,130,67,0,0,198,66,64,197,128,67,98,0,0,198,66,202,43,128,67,60,123,199,66,26,166,126,67,255,10,202,66,0,92,125,67,98,196,154,204,66,230,17,124,67,238,244,207,66,128,92,123,67,0,0,210,66,128,92,123,67,108,0,240,223,66,128,
		92,123,67,108,0,240,223,66,128,92,115,67,108,0,0,210,66,128,92,115,67,98,241,91,202,66,128,92,115,67,115,181,195,66,237,49,117,67,0,177,190,66,128,184,119,67,98,141,172,185,66,18,63,122,67,0,0,182,66,178,164,125,67,0,0,182,66,64,197,128,67,98,0,0,182,
		66,251,172,132,67,16,201,194,66,0,220,135,67,0,0,210,66,0,220,135,67,108,0,0,7,67,0,220,135,67,98,51,228,10,67,0,220,135,67,218,73,14,67,139,238,134,67,128,198,16,67,128,167,133,67,98,37,67,19,67,117,96,132,67,0,0,21,67,8,174,130,67,0,0,21,67,64,197,
		128,67,98,0,0,21,67,186,175,125,67,243,57,19,67,94,72,122,67,128,186,16,67,128,189,119,67,98,13,59,14,67,162,50,117,67,110,219,10,67,128,92,115,67,0,0,7,67,128,92,115,67,108,0,23,2,67,128,92,115,67,108,0,23,2,67,128,20,106,67,99,101,0,0 };

	loopPath.loadPathFromData(pathData, sizeof(pathData));
}

AudioSampleBufferComponentBase::~AudioSampleBufferComponentBase()
{
	if (connectedProcessor != nullptr)
	{
		connectedProcessor->removeChangeListener(this);
	}
}

void AudioSampleBufferComponentBase::setAudioSampleProcessor(SafeChangeBroadcaster* newProcessor)
{
	connectedProcessor = newProcessor;

	
}


void AudioSampleBufferComponentBase::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	over = isInterestedInDragSource(dragSourceDetails);
	repaint();
}

void AudioSampleBufferComponentBase::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	over = false;
	repaint();
}

bool AudioSampleBufferComponentBase::isInterestedInFileDrag(const StringArray &files)
{
	return files.size() == 1 && isAudioFile(files[0]);
}

bool AudioSampleBufferComponentBase::isAudioFile(const String &s)
{
    AudioFormatManager afm;

    
	
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
	
	return File(s).existsAsFile() && afm.findFormatForFileExtension(File(s).getFileExtension()) != nullptr;
}


void AudioSampleBufferComponentBase::filesDropped(const StringArray &fileNames, int, int)
{
	if (fileNames.size() > 0)
	{
		auto f = File(fileNames[0]);

		loadFile(f);
	}
}

void AudioSampleBufferComponentBase::updateRanges(SampleArea *areaToSkip/*=nullptr*/)
{
	areas[PlayArea]->setSampleRange(Range<int>(0, buffer == nullptr ? 0 : buffer->getNumSamples()));
	refreshSampleAreaBounds(areaToSkip);
}

void AudioSampleBufferComponentBase::setRange(Range<int> newRange)
{
	const bool isSomethingLoaded = currentFileName.isNotEmpty();

	getSampleArea(0)->setVisible(isSomethingLoaded);

	if (getSampleArea(0)->getSampleRange() != newRange)
	{
		getSampleArea(0)->setSampleRange(newRange);
		refreshSampleAreaBounds();
	}
}



void AudioSampleBufferComponentBase::setAudioSampleBuffer(const AudioSampleBuffer *b, const String &fileName, NotificationType notifyListeners)
{
	if (b != nullptr)
	{
		currentFileName = fileName;

		buffer = b;

		auto data = const_cast<float**>(b->getArrayOfReadPointers());

		VariantBuffer::Ptr l = new VariantBuffer(data[0], b->getNumSamples());

		var lVar = var(l.get());
		var rVar;

		if (b->getNumChannels() > 1)
		{
			VariantBuffer::Ptr r = new VariantBuffer(data[1], b->getNumSamples());
			rVar = var(r.get());
		}

		preview->setBuffer(lVar, rVar);
	}
	else
	{
		currentFileName = {};

		buffer = nullptr;

		preview->clear();
	}

	updateRanges();

	setCurrentArea(getSampleArea(0));

	if (notifyListeners)
		sendAreaChangedMessage();
}



void AudioSampleBufferComponentBase::mouseDown(const MouseEvent &e)
{
	if (e.mods.isRightButtonDown() || e.mods.isCtrlDown())
	{
		String patterns = "*.wav;*.aif;*.aiff;*.WAV;*.AIFF;*.hlac;*.flac;*.HLAC;*.FLAC";

		File searchDirectory = getDefaultDirectory();

		FileChooser fc("Load File", searchDirectory, patterns, true);

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();

			loadFile(f);
		}
	}
}

void AudioSampleBufferComponentBase::paint(Graphics &g)
{
	bgColour = findColour(AudioDisplayComponent::ColourIds::bgColour);
	g.fillAll(bgColour);

	if (over)
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(getLocalBounds(), 2);
	}
}


void AudioSampleBufferComponentBase::paintOverChildren(Graphics& g)
{

	Font f = GLOBAL_BOLD_FONT();

	g.setFont(f);


    static const String text = "Drop audio file or Right click to open browser";
    
    const int w = f.getStringWidth(text) + 20;

	if (getWidth() > (w+10) && (buffer == nullptr || buffer->getNumSamples() == 0))
	{
		g.setColour(Colours::white.withAlpha(0.3f));

		
		g.setColour(Colours::black.withAlpha(0.5f));
		Rectangle<int> r((getWidth() - w) / 2, (getHeight() - 20) / 2, w, 20);
		g.fillRect(r);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawRect(r, 1);

		g.drawText(text, getLocalBounds(), Justification::centred);
	}

	AudioDisplayComponent::paint(g);

	const String fileNameToShow = File::isAbsolutePath(currentFileName) ? File(currentFileName).getFileName() : currentFileName;

	if (showFileName && fileNameToShow.isNotEmpty())
	{
		const int w2 = f.getStringWidth(fileNameToShow) + 20;
		g.setColour(Colours::black.withAlpha(0.5f));
		Rectangle<int> r(getWidth() - w2 - 5, 5, w2, 20);
		g.fillRect(r);
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(r, 1);
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawText(fileNameToShow, r, Justification::centred);
	}

	if (showLoop)
	{
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawVerticalLine(xPositionOfLoop.getStart(), 0.0f, (float)getHeight());
		g.drawVerticalLine(xPositionOfLoop.getEnd(), 0.0f, (float)getHeight());

		Path t1;

		float x1 = (float)xPositionOfLoop.getStart() + 1;
		float x2 = (float)xPositionOfLoop.getEnd();

		t1.startNewSubPath(x1, 0.0f);
		t1.lineTo(x1 + 10.0f, 0.0f);
		t1.lineTo(x1, 10.0f);
		t1.closeSubPath();

		g.fillPath(t1);

		Path t2;

		t2.startNewSubPath(x2, 0.0f);
		t2.lineTo(x2 - 10.0f, 0.0f);
		t2.lineTo(x2, 10.0f);
		t2.closeSubPath();

		g.fillPath(t2);

		loopPath.scaleToFit(x1 + 5.0f, 4.0f, 20.0f, 10.0f, true);
		g.fillPath(loopPath);
	}
}

void HiseAudioThumbnail::LoadingThread::run()
{
	Rectangle<int> bounds;
	var lb;
	var rb;
	ScopedPointer<AudioFormatReader> reader;

	bool sv = false;

	{
		if (parent.get() == nullptr)
			return;

		ScopedLock sl(parent->lock);

		sv = parent->shouldScaleVertically();

		bounds = parent->getBounds();

		if (parent->currentReader != nullptr)
		{
			reader.swapWith(parent->currentReader);
		}
		else
		{
			lb = parent->lBuffer;
			rb = parent->rBuffer;
		}
	}

	if (reader != nullptr)
	{
		VariantBuffer::Ptr l = new VariantBuffer((int)reader->lengthInSamples);
		VariantBuffer::Ptr r;

		if (reader->numChannels > 1)
			r = new VariantBuffer((int)reader->lengthInSamples);

		float* d[2];

		d[0] = l->buffer.getWritePointer(0);
		d[1] = r != nullptr ? r->buffer.getWritePointer(0) : nullptr;

		AudioSampleBuffer tempBuffer = AudioSampleBuffer(d, reader->numChannels, (int)reader->lengthInSamples);
		

		if (threadShouldExit())
			return;

		reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

		if (threadShouldExit())
			return;

		lb = var(l.get());

		if (reader->numChannels > 1)
		{
			rb = var(r.get());
		}

		if (parent.get() != nullptr)
		{
			ScopedLock sl(parent->lock);

			parent->lBuffer = lb;
			parent->rBuffer = rb;
		}
	}

	Path lPath;
	Path rPath;

	RectangleList<float> lRects, rRects;

	float width = (float)bounds.getWidth();


	VariantBuffer::Ptr r = rb.getBuffer();
	VariantBuffer::Ptr l = lb.getBuffer();

	
	if (l != nullptr && l->size != 0)
	{
		const float* data = l->buffer.getReadPointer(0);
		const int numSamples = l->size;

		calculatePath(lPath, width, data, numSamples, lRects);
	}

	if (r != nullptr && r->size != 0)
	{
		const float* data = r->buffer.getReadPointer(0);
		const int numSamples = r->size;

		calculatePath(rPath, width, data, numSamples, rRects);
	}
	
	const bool isMono = rPath.isEmpty() && rRects.isEmpty();

	if (isMono)
	{
		if (l != nullptr && l->size != 0)
		{
			const float* data = l->buffer.getReadPointer(0);
			const int numSamples = l->size;

			scalePathFromLevels(lPath, rRects, { 0.0f, 0.0f, (float)bounds.getWidth(), (float)bounds.getHeight() }, data, numSamples, sv);
		}
	}
	else
	{
		float h = (float)bounds.getHeight() / 2.0f;

		if (l != nullptr && l->size != 0)
		{
			const float* data = l->buffer.getReadPointer(0);
			const int numSamples = l->size;

			scalePathFromLevels(lPath, lRects, { 0.0f, 0.0f, (float)bounds.getWidth(), h }, data, numSamples, sv);
		}

		if (r != nullptr && r->size != 0)
		{
			const float* data = r->buffer.getReadPointer(0);
			const int numSamples = r->size;

			scalePathFromLevels(rPath, rRects, { 0.0f, h, (float)bounds.getWidth(), h }, data, numSamples, sv);
		}
	}

	{
		if (parent.get() != nullptr)
		{
			ScopedLock sl(parent->lock);

			parent->leftWaveform.swapWithPath(lPath);
			parent->rightWaveform.swapWithPath(rPath);
			parent->leftPeaks.swapWith(lRects);
			parent->rightPeaks.swapWith(rRects);

			parent->isClear = false;

			parent->refresh();
		}
	}
}

void HiseAudioThumbnail::LoadingThread::scalePathFromLevels(Path &p, RectangleList<float>& rects, Rectangle<float> bounds, const float* data, const int numSamples, bool scaleVertically)
{
	if (!rects.isEmpty())
	{
		auto levels = FloatVectorOperations::findMinAndMax(data, numSamples);

		auto b = rects.getBounds();
		
		rects.offsetAll(bounds.getX(), bounds.getY() + bounds.getHeight()*0.5f);

		return;
	}

	if (p.isEmpty())
		return;

	if (p.getBounds().getHeight() == 0)
		return;

	auto levels = FloatVectorOperations::findMinAndMax(data, numSamples);

	if (levels.isEmpty())
	{
		p.clear();
		p.startNewSubPath(bounds.getX(), bounds.getCentreY());
		p.lineTo(bounds.getRight(), bounds.getCentreY());
		p.closeSubPath();
	}
	else
	{
		auto h = bounds.getHeight();

		float half = h / 2.0f;

		auto trimmedTop = (1.0f - std::fabs(levels.getEnd())) * half;
		auto trimmedBottom = (1.0f - std::fabs(levels.getStart())) * half;

		if (!scaleVertically)
		{
			bounds.removeFromTop(trimmedTop);
			bounds.removeFromBottom(trimmedBottom);
		}
		
		if(!std::isinf(bounds.getY()) && !std::isinf(bounds.getHeight()) &&
		   !std::isnan(bounds.getY()) && !std::isnan(bounds.getHeight()))
			p.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
	}
}

#define USE_RECTS_FOR_WAVEFORM 0


void HiseAudioThumbnail::LoadingThread::calculatePath(Path &p, float width, const float* l_, int numSamples, RectangleList<float>& rects)
{
	int stride = roundToInt((float)numSamples / width);
	stride = jmax<int>(1, stride * 2);

	if (numSamples != 0)
	{
		p.clear();
		p.startNewSubPath(0.0f, 0.0f);
		
#if HISE_USE_SYMMETRIC_WAVEFORMS

		Array<Point<float>> values;

		values.ensureStorageAllocated(numSamples / stride + 2);

		for (int i = 0; i < numSamples; i+= stride)
		{
			if (threadShouldExit())
				return;

			const int numToCheck = jmin<int>(stride, numSamples - i);
			auto minMax = FloatVectorOperations::findMinAndMax(l_ + i, numToCheck);
			auto value = jmax(std::abs(minMax.getStart()), std::abs(minMax.getEnd()));

			value = jlimit<float>(0.0f, 1.0f, value);
			value *= 10.f;
			values.add({ (float)i / stride, value });
		};

		int numRemoved = 0;

		float distanceThreshold = JUCE_LIVE_CONSTANT_OFF(0.00f);

		bool lastWasZero = false;

		for (int i = 1; i < values.size()-1; i++)
		{
			auto prev = values[i - 1];
			auto next = values[i + 1];

			if (values[i].y <= distanceThreshold && prev.y == 0.0f && next.y == 0.0f)
			{
				lastWasZero = true;
				numRemoved++;
				values.remove(i--);
				continue;
			}

			if (lastWasZero)
			{
				Point<float> newZero(values[i].x, 0.0f);
				values.insert(i, newZero);
			}

			lastWasZero = false;

			auto distance = (next.y + prev.y) / 2.0f;

			if (distance < distanceThreshold)
			{
				values.remove(i);
				numRemoved++;
			}
		}


		int numPoints = values.size();

		for (int i = 0; i < numPoints; i++)
			p.lineTo(values[i]);

		for (int i = numPoints - 1; i >= 0; i--)
			p.lineTo(values[i].withY(values[i].y * -1.0f));


#else



		for (int i = stride; i < numSamples; i += stride)
		{
			if (threadShouldExit())
				return;

			const int numToCheck = jmin<int>(stride, numSamples - i);
			auto value = jmax<float>(0.0f, FloatVectorOperations::findMaximum(l_ + i, numToCheck));
			value = jlimit<float>(-1.0f, 1.0f, value);
			p.lineTo((float)i, -1.0f * value);
		};

		for (int i = numSamples - 1; i >= 0; i -= stride)
		{
			if (threadShouldExit())
				return;

			const int numToCheck = jmin<int>(stride, numSamples - i);
			auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));
			value = jlimit<float>(-1.0f, 1.0f, value);
			p.lineTo((float)i, -1.0f * value);
		};

#endif

		p.closeSubPath();
	}
	else
	{
		p.clear();
	}
}

HiseAudioThumbnail::HiseAudioThumbnail() :
	loadingThread(this)
{
	setEnablePaintProfiling("AudioThumbnail");
	setColour(AudioDisplayComponent::ColourIds::fillColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xffcccccc)));
	setColour(AudioDisplayComponent::ColourIds::outlineColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xa2181818)));

	setInterceptsMouseClicks(false, false);
	setBufferedToImage(true);
}

HiseAudioThumbnail::~HiseAudioThumbnail()
{
	loadingThread.stopThread(400);
}

void HiseAudioThumbnail::setBuffer(var bufferL, var bufferR /*= var()*/, bool synchronously)
{
	currentReader = nullptr;

	const bool shouldBeNotEmpty = bufferL.isBuffer() && bufferL.getBuffer()->size != 0;
	const bool isNotEmpty = lBuffer.isBuffer() && lBuffer.getBuffer()->size != 0;

	if (!isNotEmpty && !shouldBeNotEmpty)
		return;

	lBuffer = bufferL;
	rBuffer = bufferR;

	if (auto l = bufferL.getBuffer())
	{
		lengthInSeconds = l->size / 44100.0;
	}

	rebuildPaths(synchronously);
}

void HiseAudioThumbnail::paint(Graphics& g)
{
	if (isClear || rebuildOnUpdate)
		return;

	ScopedTryLock sl(lock);

	if (sl.isLocked())
	{


		auto bounds = getLocalBounds();


		if (leftBound > 0 || rightBound > 0)
		{
			auto left = bounds.removeFromLeft(leftBound);
			auto right = bounds.removeFromRight(rightBound);

			g.saveState();

			g.excludeClipRegion(left);
			g.excludeClipRegion(right);



			drawSection(g, true);

			g.restoreState();
			g.excludeClipRegion(bounds);

			drawSection(g, false);
		}
		else
		{
			drawSection(g, true);
		}
	}
	
}

void HiseAudioThumbnail::drawSection(Graphics &g, bool enabled)
{
	bool isStereo = rBuffer.isBuffer();

	Colour fillColour = findColour(AudioDisplayComponent::ColourIds::fillColour);
	Colour outlineColour = findColour(AudioDisplayComponent::ColourIds::outlineColour);

	if (!enabled)
	{
		fillColour = fillColour.withMultipliedAlpha(0.3f);
		outlineColour = outlineColour.withMultipliedAlpha(0.3f);
	}
		

	if (!isStereo)
	{
		g.setColour(fillColour.withAlpha(0.05f));

		int h = getHeight();

		if (drawHorizontalLines)
		{
			g.drawHorizontalLine(h / 4, 0.0f, (float)getWidth());
			g.drawHorizontalLine(3 * h / 4, 0.0f, (float)getWidth());
		}

		g.setColour(fillColour);

		if (!leftWaveform.isEmpty())
			g.fillPath(leftWaveform);
		else if (!leftPeaks.isEmpty())
			g.fillRectList(leftPeaks);

		if (!outlineColour.isTransparent() && !leftWaveform.isEmpty())
		{
			g.setColour(outlineColour);
			g.strokePath(leftWaveform, PathStrokeType(1.0f));
		}
	}
	else
	{
		g.setColour(fillColour.withAlpha(0.08f));

		int h = getHeight()/2;

		if (drawHorizontalLines)
		{
			g.drawHorizontalLine(h / 4, 0.0f, (float)getWidth());
			g.drawHorizontalLine(3 * h / 4, 0.0f, (float)getWidth());
			g.drawHorizontalLine(h + h / 4, 0.0f, (float)getWidth());
			g.drawHorizontalLine(h + 3 * h / 4, 0.0f, (float)getWidth());
		}

		g.setColour(fillColour);

		if (!leftWaveform.isEmpty() || !rightWaveform.isEmpty())
		{
			g.fillPath(leftWaveform);
			g.fillPath(rightWaveform);
		}
		else if (!leftPeaks.isEmpty() || !rightPeaks.isEmpty())
		{
			g.fillRectList(leftPeaks);
			g.fillRectList(rightPeaks);
		}

		if (!outlineColour.isTransparent())
		{
			g.setColour(outlineColour);
			g.strokePath(leftWaveform, PathStrokeType(1.0f));
			g.strokePath(rightWaveform, PathStrokeType(1.0f));
		}
	}
}

void HiseAudioThumbnail::setReader(AudioFormatReader* r, int64 actualNumSamples)
{
	currentReader = r;

	if (actualNumSamples == -1)
		actualNumSamples = currentReader->lengthInSamples;

	if (currentReader != nullptr)
	{
		lengthInSeconds = actualNumSamples / currentReader->sampleRate;
	}

	rebuildPaths();
}

void HiseAudioThumbnail::clear()
{
	ScopedLock sl(lock);

	lBuffer = var();
	rBuffer = var();

	leftWaveform.clear();
	rightWaveform.clear();

	isClear = true;

	currentReader = nullptr;

	repaint();
}

void HiseAudioThumbnail::setRange(const int left, const int right) 
{
	leftBound = left;
	rightBound = getWidth() - right;
	repaint();
}

} // namespace hise
