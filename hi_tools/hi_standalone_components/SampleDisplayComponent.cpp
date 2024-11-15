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
		if (auto tlaf = dynamic_cast<HiseAudioThumbnail::LookAndFeelMethods*>(&getThumbnail()->getLookAndFeel()))
		{
			NormalisableRange<double> range((double)getSampleArea(0)->getX(), (double)getSampleArea(0)->getRight());

			playBackPosition = jlimit<double>(0.0, 1.0, playBackPosition);

			const int x = (int)(range.convertFrom0to1(playBackPosition));

			tlaf->drawThumbnailRuler(g, *getThumbnail(), x);
		}
		else
			jassertfalse;
	}
}

void AudioDisplayComponent::refreshSampleAreaBounds(SampleArea* areaToSkip/*=nullptr*/)
{
	for (int i = 0; i < areas.size(); i++)
	{
		if (areas[i] == areaToSkip) continue;

		//areas[i]->setVisible(somethingVisible);

		Range<int> sampleRange = areas[i]->getSampleRange();

		const int x = areas[i]->getXForSample(sampleRange.getStart(), false);
		const int right = areas[i]->getXForSample(sampleRange.getEnd(), false);

		areas[i]->leftEdge->setTooltip(String(sampleRange.getStart()));
		areas[i]->rightEdge->setTooltip(String(sampleRange.getEnd()));

		if (i == 0)
			preview->setRange(x, right);

		areas[i]->setBounds(x, 0, right - x, getHeight());
	}

	repaint();
}

void AudioDisplayComponent::paintOverChildren(Graphics &g)
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

Range<int> AudioDisplayComponent::SampleArea::getSampleRange() const
{	return range; }

void AudioDisplayComponent::SampleArea::setSampleRange(Range<int> r)
{ 
	range = r; 

	repaint();
}

bool AudioDisplayComponent::SampleArea::isAreaEnabled() const
{ return areaEnabled; }

void AudioDisplayComponent::SampleArea::setAllowedPixelRanges(Range<int> leftRangeInSamples,
	Range<int> rightRangeInSamples)
{
	useConstrainer = !(leftRangeInSamples.isEmpty() && rightRangeInSamples.isEmpty());
			
	if(!useConstrainer) return;
			

	leftEdgeRangeInPixels = Range<int>(getXForSample(leftRangeInSamples.getStart(), false),
	                                   getXForSample(leftRangeInSamples.getEnd(), false));

	rightEdgeRangeInPixels = Range<int>(getXForSample(rightRangeInSamples.getStart(), false),
	                                    getXForSample(rightRangeInSamples.getEnd(), false));
}

void AudioDisplayComponent::SampleArea::setAreaEnabled(bool shouldBeEnabled)
{
	areaEnabled = shouldBeEnabled;

	leftEdge->setInterceptsMouseClicks(areaEnabled, false);
	rightEdge->setInterceptsMouseClicks(areaEnabled, false);

	repaint();
}

void AudioDisplayComponent::SampleArea::toggleEnabled()
{
	setAreaEnabled(!areaEnabled);

	repaint();
}

AudioDisplayComponent::SampleArea::AreaEdge::AreaEdge(Component* componentToResize,
	ComponentBoundsConstrainer* constrainer, Edge edgeToResize):
	ResizableEdgeComponent(componentToResize, constrainer, edgeToResize)
{}

void AudioDisplayComponent::SampleArea::setReversed(bool isReversed)
{
	reversed = isReversed;
	repaint();
}

AudioDisplayComponent::SampleArea::EdgeLookAndFeel::EdgeLookAndFeel(SampleArea* areaParent): parentArea(areaParent)
{}

void AudioDisplayComponent::setPlaybackPosition(double normalizedPlaybackPosition)
{
	if(playBackPosition != normalizedPlaybackPosition)
	{
		playBackPosition = normalizedPlaybackPosition;

		SafeAsyncCall::repaint(this);
	}
}

AudioDisplayComponent::AudioDisplayComponent():
	playBackPosition(0.0)
{
	afm.registerBasicFormats();

	addAndMakeVisible(preview = new HiseAudioThumbnail());

	preview->setLookAndFeel(&defaultLaf);
}

AudioDisplayComponent::~AudioDisplayComponent()
{
	preview = nullptr;

	list.clear();		
}

AudioDisplayComponent::Listener::~Listener()
{}

void AudioDisplayComponent::addAreaListener(Listener* l)
{
	list.add(l);
}

void AudioDisplayComponent::removeAreaListener(Listener* l)
{
	list.remove(l);
}

void AudioDisplayComponent::setCurrentArea(SampleArea* area)
{
	currentArea = area;
}

void AudioDisplayComponent::sendAreaChangedMessage()
{
	list.call(&Listener::rangeChanged, this, areas.indexOf(currentArea));
	repaint();
}

void AudioDisplayComponent::resized()
{
	preview->setBounds(getLocalBounds());
	preview->resized();
	refreshSampleAreaBounds();
	updateRanges();
}

HiseAudioThumbnail* AudioDisplayComponent::getThumbnail()
{
	return preview;
}

int AudioDisplayComponent::getTotalSampleAmount() const
{
	return (int)(preview->getTotalLength() * getSampleRate());
}

void AudioDisplayComponent::setIsOnInterface(bool isOnInterface)
{
	onInterface = isOnInterface;
}

AudioDisplayComponent::SampleArea* AudioDisplayComponent::getSampleArea(int index)
{return areas[index];}

float AudioDisplayComponent::getNormalizedPeak()
{ return 1.0f; }

void AudioDisplayComponent::mouseMove(const MouseEvent& e)
{
	auto xNormalised = (float)e.getPosition().getX() / (float)getWidth();

	hoverPosition = xNormalised * (float)getTotalSampleAmount();
}

float AudioDisplayComponent::getHoverPosition() const
{
	return hoverPosition;
}

MultiChannelAudioBuffer::SampleReference::SampleReference(bool ok, const String& ref):
	r(ok ? Result::ok() : Result::fail(ref + " not found")),
	reference(ref)
{}

MultiChannelAudioBuffer::SampleReference::operator bool()
{ return r.wasOk(); }

bool MultiChannelAudioBuffer::SampleReference::operator==(const SampleReference& other) const
{
	return reference == other.reference;
}

MultiChannelAudioBuffer::DataProvider::~DataProvider() = default;

File MultiChannelAudioBuffer::DataProvider::getRootDirectory()
{ return rootDir; }

void MultiChannelAudioBuffer::DataProvider::setRootDirectory(const File& rootDirectory)
{ 
	rootDir = rootDirectory; 
}

bool MultiChannelAudioBuffer::XYZItem::matches(int n, int v, int r)
{
	return veloRange.contains(v) &&
		keyRange.contains(n) &&
		rrGroup == r;
}

int MultiChannelAudioBuffer::XYZPool::indexOf(const String& ref) const
{
	for (int i = 0; i < pool.size(); i++)
		if (pool[i]->reference == ref)
			return i;

	return -1;
}

MultiChannelAudioBuffer::SampleReference::Ptr MultiChannelAudioBuffer::XYZPool::loadFile(const String& ref)
{
	for (auto i : pool)
	{
		if (i->reference == ref)
			return i;
	}

	return new SampleReference(false, ref);
}

MultiChannelAudioBuffer::XYZProviderBase::XYZProviderBase(XYZPool* pool_): pool(pool_)
{}

void MultiChannelAudioBuffer::XYZProviderBase::removeFromPool(SampleReference::Ptr p)
{
	if(pool != nullptr)
		pool->pool.removeObject(p);
}

String MultiChannelAudioBuffer::XYZProviderBase::getWildcard() const
{ 
	String s;
	s << "{XYZ::" << getId() << "}";
	return s;
}

MultiChannelAudioBuffer::SampleReference::Ptr MultiChannelAudioBuffer::XYZProviderBase::getPooledItem(int idx) const
{
	if (pool != nullptr)
	{
		if (isPositiveAndBelow(idx, pool->pool.size()))
			return pool->pool[idx];
	}
			
	return nullptr;
}

Identifier MultiChannelAudioBuffer::XYZProviderFactory::parseID(const String& referenceString)
{
	static const String wildcard("{XYZ::");
	if (referenceString.startsWith(wildcard))
	{
		auto wc = referenceString.upToFirstOccurrenceOf("}", false, false);
		return Identifier(wc.fromLastOccurrenceOf(":", false, false));
	}

	return Identifier();
}

void MultiChannelAudioBuffer::XYZProviderFactory::registerXYZProvider(const Identifier& id,
	const std::function<XYZProviderBase*()>& f)
{
	for (const auto& i : items)
		if (i.id == id)
			return;

	items.add({ id, f });
}

MultiChannelAudioBuffer::XYZProviderBase* MultiChannelAudioBuffer::XYZProviderFactory::create(const Identifier& id)
{
	for (const auto& i : items)
		if (i.id == id)
			return i.f();

	return nullptr;
}

Array<Identifier> MultiChannelAudioBuffer::XYZProviderFactory::getIds()
{
	Array<Identifier> ids;

	for (auto& i : items)
	{
		ids.add(i.id);
	}

	return ids;
}

MultiChannelAudioBuffer::Listener::Listener() = default;
MultiChannelAudioBuffer::Listener::~Listener() = default;

void MultiChannelAudioBuffer::Listener::sampleIndexChanged(int newSampleIndex)
{}

void MultiChannelAudioBuffer::Listener::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var v)
{
	switch (d)
	{
	case ComplexDataUIUpdaterBase::EventType::ContentChange:
		bufferWasModified();
		break;
	case ComplexDataUIUpdaterBase::EventType::ContentRedirected:
		bufferWasLoaded();
		break;
	case ComplexDataUIUpdaterBase::EventType::DisplayIndex:
		sampleIndexChanged((int)v);
		break;
	default:
		break;
	}
}

void MultiChannelAudioBuffer::addListener(Listener* l)
{
	internalUpdater.addEventListener(l);
}

void MultiChannelAudioBuffer::removeListener(Listener* l)
{
	internalUpdater.removeEventListener(l);
}

String MultiChannelAudioBuffer::toBase64String() const
{ 
	return referenceString;
}

void MultiChannelAudioBuffer::setRange(Range<int> sampleRange)
{
	sampleRange.setStart(jmax(0, sampleRange.getStart()));
	sampleRange.setEnd(jmin(originalBuffer.getNumSamples(), sampleRange.getEnd()));

	if (sampleRange != bufferRange)
	{
		{
			auto nb = createNewDataBuffer(sampleRange);

			SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
			bufferRange = sampleRange;
			setDataBuffer(nb);
		}
	}
}

void MultiChannelAudioBuffer::loadFromEmbeddedData(SampleReference::Ptr r)
{
	referenceString = "{INTERNAL}";
		
	auto mag = r->buffer.getMagnitude(0, r->buffer.getNumSamples());

	ignoreUnused(mag);
	jassert(mag < 1.0f);

	originalBuffer.makeCopyOf(r->buffer);

	auto nb = createNewDataBuffer({ 0, originalBuffer.getNumSamples() });
	SimpleReadWriteLock::ScopedWriteLock l(getDataLock());
	sampleRate = r->sampleRate;
	bufferRange = { 0, originalBuffer.getNumSamples() };
	loopRange = r->loopRange;
	setDataBuffer(nb);
}

void MultiChannelAudioBuffer::loadBuffer(const AudioSampleBuffer& b, double sr)
{
	referenceString = "{INTERNAL}";
		
	originalBuffer.makeCopyOf(b);

	auto nb = createNewDataBuffer({ 0, b.getNumSamples() });
	SimpleReadWriteLock::ScopedWriteLock l(getDataLock());
	sampleRate = sr;
	bufferRange = { 0, b.getNumSamples() };
	setDataBuffer(nb);
}

void MultiChannelAudioBuffer::setLoopRange(Range<int> newLoopRange, NotificationType n)
{
	newLoopRange.setStart(jmax(bufferRange.getStart(), newLoopRange.getStart()));
	newLoopRange.setEnd(jmin(bufferRange.getEnd(), newLoopRange.getEnd()));

	if (newLoopRange != loopRange)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
			loopRange = newLoopRange;
		}
			
		if(n != dontSendNotification)
			getUpdater().sendContentChangeMessage(sendNotificationSync, -1);
	}
}

var MultiChannelAudioBuffer::getChannelBuffer(int channelIndex, bool getFullContent)
{
	auto& bToUse = getFullContent ? originalBuffer : currentData;

	if(isPositiveAndBelow(channelIndex, bToUse.getNumChannels()))
		return var(new VariantBuffer(bToUse.getWritePointer(channelIndex, 0), bToUse.getNumSamples()));

	return {};
}

void MultiChannelAudioBuffer::setProvider(DataProvider::Ptr p)
{
	provider = p;
}

Range<int> MultiChannelAudioBuffer::getCurrentRange() const
{
	return bufferRange;
}

Range<int> MultiChannelAudioBuffer::getTotalRange() const
{
	return { 0, originalBuffer.getNumSamples() };
}

Range<int> MultiChannelAudioBuffer::getLoopRange(bool subtractStart) const
{
	bool useLoop = !loopRange.isEmpty() && loopRange.getStart() < bufferRange.getEnd();

	auto delta = (int)subtractStart * bufferRange.getStart();

	return (useLoop ? loopRange.getIntersectionWith(bufferRange): bufferRange) - delta;
}

AudioSampleBuffer& MultiChannelAudioBuffer::getBuffer()
{ return currentData; }

const AudioSampleBuffer& MultiChannelAudioBuffer::getBuffer() const
{ return currentData; }

MultiChannelAudioBuffer::DataProvider::Ptr MultiChannelAudioBuffer::getProvider()
{
	return provider;
}

bool MultiChannelAudioBuffer::isEmpty() const
{
	return originalBuffer.getNumChannels() == 0 || originalBuffer.getNumSamples() == 0;
}

bool MultiChannelAudioBuffer::isNotEmpty() const
{
	return originalBuffer.getNumChannels() != 0 || originalBuffer.getNumSamples() != 0;
}

float** MultiChannelAudioBuffer::getDataPtrs()
{
	jassert(!isXYZ());
	return currentData.getArrayOfWritePointers();
}

Array<Identifier> MultiChannelAudioBuffer::getAvailableXYZProviders()
{
	auto ids = factory->getIds();

	for (int i = 0; i < ids.size(); i++)
	{
		if (deactivatedXYZIds.contains(ids[i]))
			ids.remove(i--);
	}

	return ids;
}

Identifier MultiChannelAudioBuffer::getCurrentXYZId() const
{
	if (xyzProvider != nullptr)
		return xyzProvider->getId();

	return Identifier();
}

bool MultiChannelAudioBuffer::isXYZ() const
{
	return xyzProvider != nullptr;
}

void MultiChannelAudioBuffer::registerXYZProvider(const Identifier& id, const std::function<XYZProviderBase*()>& f)
{
	factory->registerXYZProvider(id, f);
}

const MultiChannelAudioBuffer::XYZItem::List& MultiChannelAudioBuffer::getXYZItems() const
{ return xyzItems; }

MultiChannelAudioBuffer::XYZItem::List& MultiChannelAudioBuffer::getXYZItems()
{ return xyzItems; }

MultiChannelAudioBuffer::SampleReference::Ptr MultiChannelAudioBuffer::getFirstXYZData()
{
	if (xyzItems.isEmpty())
		return nullptr;

	return xyzItems[0].data;
}

void MultiChannelAudioBuffer::setDisabledXYZProviders(const Array<Identifier>& ids)
{
	deactivatedXYZIds = ids;
}

void MultiChannelAudioBuffer::setDataBuffer(AudioSampleBuffer& newBuffer)
{
	// Never call this without holding the lock
	jassert(getDataLock().writeAccessIsLocked());

	std::swap(currentData, newBuffer);
	getUpdater().sendContentRedirectMessage();
}

AudioSampleBuffer MultiChannelAudioBuffer::createNewDataBuffer(Range<int> newRange)
{
	if (newRange.isEmpty())
		return {};
		
	SimpleReadWriteLock::ScopedReadLock l(getDataLock());

	AudioSampleBuffer newDataBuffer(originalBuffer.getNumChannels(), newRange.getLength());

	for (int i = 0; i < newDataBuffer.getNumChannels(); i++)
		newDataBuffer.copyFrom(i, 0, originalBuffer.getReadPointer(i, newRange.getStart()), newDataBuffer.getNumSamples());

	return newDataBuffer;
}

void AudioDisplayComponent::SampleArea::mouseUp(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

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
	CHECK_MIDDLE_MOUSE_DOWN(e);

	prevDragWidth = getWidth();
	leftEdgeClicked = e.eventComponent == leftEdge;

	parentWaveform->setCurrentArea(this);

}

void AudioDisplayComponent::SampleArea::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	checkBounds();

	parentWaveform->refreshSampleAreaBounds(this);
};

void HiseAudioThumbnail::LookAndFeelMethods::drawThumbnailRange(Graphics& g, HiseAudioThumbnail& te, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled)
{
    UnblurryGraphics ug(g, te, true);
    
    g.setColour(c.withAlpha(areaEnabled ? 0.1f : 0.02f));
    g.fillAll();

    g.setColour(c.withAlpha(0.3f));
    ug.draw1PxRect(area);
}

void HiseAudioThumbnail::LookAndFeelMethods::drawThumbnailRuler(Graphics& g, HiseAudioThumbnail& te, int x)
{
	auto l = Line<float>((float)x, 0.0f, (float)x, (float)te.getHeight());
	auto r = Rectangle<float>((float)x, 0.0f, x == 0 ? 5.0f : 10.0f, (float)te.getHeight());

	g.setColour(Colours::lightgrey.withAlpha(0.05f));
	g.fillRect(r);

	g.setColour(Colours::white.withAlpha(0.6f));
	g.drawLine(l, 0.5f);
}

void AudioDisplayComponent::SampleArea::paint(Graphics &g)
{
	if(area == AreaTypes::LoopCrossfadeArea)
	{
		Path fadeInPath;

		auto w = (float)getWidth();
		auto h = (float)getHeight();
		auto z = 0.0f;

		

		if (!reversed)
		{
			fadeInPath.startNewSubPath(z, h);

			if (gamma == 1.0f)
			{
				fadeInPath.lineTo(w, z);
				fadeInPath.lineTo(w, h);
			}
			else
			{
				for (float x = z; x < w; x += 3.0f)
				{
					float gain = (x - z) / w;
					gain = std::pow(gain, gamma);
					float y = h - gain * h;
					fadeInPath.lineTo(x, y);
				}

				fadeInPath.lineTo(w, h);
			}
			
			fadeInPath.closeSubPath();
		}
		else
		{
			

			if (gamma == 1.0f)
			{
				fadeInPath.startNewSubPath(z, z);
				fadeInPath.lineTo(w, h);
				fadeInPath.lineTo(z, h);
			}
			else
			{
				fadeInPath.startNewSubPath(w, h);
				for (float x = z; x < w; x += 3.0f)
				{
					float gain = (x - z) / w;
					gain = std::pow(gain, gamma);
					float y = h - gain * h;
					fadeInPath.lineTo(w - x, y);
				}

				fadeInPath.lineTo(z, h);
			}
			
			fadeInPath.closeSubPath();
		}

		g.setColour(getAreaColour((AreaTypes)area).withAlpha(areaEnabled ? 0.1f : 0.05f));
		g.fillPath(fadeInPath);

		g.setColour(getAreaColour((AreaTypes)area).withAlpha(0.3f));
		PathStrokeType stroke(1.0f);
		g.strokePath(fadeInPath, stroke);
	}
	else
	{
        auto p = findParentComponentOfClass<AudioDisplayComponent>();
        if(auto laf = dynamic_cast<HiseAudioThumbnail::LookAndFeelMethods*>(&p->getThumbnail()->getLookAndFeel()))
        {
            auto a = getLocalBounds().toFloat();
            
            laf->drawThumbnailRange(g, *p->getThumbnail(), a, (int)area, getAreaColour((AreaTypes)area), areaEnabled);
        }
        else
            jassertfalse;
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

Colour AudioDisplayComponent::SampleArea::getAreaColour(AreaTypes area)
{
	switch(area)
	{
	case AreaTypes::PlayArea:			return Colours::white;
	case AreaTypes::SampleStartArea:	return JUCE_LIVE_CONSTANT_OFF(Colour(0xff5e892f));
	case AreaTypes::LoopArea:			return JUCE_LIVE_CONSTANT_OFF(Colour(0xff59a2b1));
	case AreaTypes::LoopCrossfadeArea:	return JUCE_LIVE_CONSTANT_OFF(Colour(0xffcfc75c));
	default:		jassertfalse;		return Colours::transparentBlack;
	}
}



void AudioDisplayComponent::SampleArea::setGamma(float newGamma)
{
	gamma = jlimit<float>(0.125f, 16.0f, newGamma);
	repaint();
}

void AudioDisplayComponent::SampleArea::EdgeLookAndFeel::drawStretchableLayoutResizerBar (Graphics &g,
																   Component& resizer,
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
		g.setColour(parentArea->getAreaColour((AreaTypes)parentArea->area).withAlpha(isMouseOver ? 0.2f : 0.0f));
		g.fillAll();
	}
};



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

	AudioSampleBuffer specBuffer;

	float* d[2];

	if (reader != nullptr)
	{
		VariantBuffer::Ptr l = new VariantBuffer((int)reader->lengthInSamples);
		VariantBuffer::Ptr r;

		if (reader->numChannels > 1)
			r = new VariantBuffer((int)reader->lengthInSamples);

		d[0] = l->buffer.getWritePointer(0);
		d[1] = r != nullptr ? r->buffer.getWritePointer(0) : nullptr;

		specBuffer = AudioSampleBuffer(d, reader->numChannels, (int)reader->lengthInSamples);
		
		if (threadShouldExit())
			return;

		reader->read(&specBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
            
		if (threadShouldExit())
			return;

		lb = var(l.get());

		if (reader->numChannels > 1)
		{
			rb = var(r.get());
		}

		parent->sampleProcessor.sendMessage(sendNotificationSync, lb, rb);

		if (parent.get() != nullptr)
		{
			ScopedLock sl(parent->lock);

			

			parent->lBuffer = lb;
			parent->rBuffer = rb;
		}
	}
	else if(lb.isBuffer())
	{
		d[0] = lb.getBuffer()->buffer.getWritePointer(0);
		d[1] = rb.isBuffer() ? rb.getBuffer()->buffer.getWritePointer(0) : nullptr;
		specBuffer = AudioSampleBuffer(d, rb.isBuffer() ? 2 : 1, lb.getBuffer()->size);
	}

	Image newSpec;

	if (parent->specDirty && specBuffer.getNumSamples() > 0 && parent->spectrumAlpha != 0.0f)
	{
		Spectrum2D spec(parent, specBuffer);

		spec.parameters = parent->getParameters();

		if (spec.parameters->Spectrum2DSize == 0)
			spec.parameters->setFromBuffer(specBuffer);

		auto b = spec.createSpectrumBuffer(false);
		newSpec = spec.createSpectrumImage(b);
		parent->specDirty = false;
	}
	else
	{
		newSpec = parent->spectrum;
		parent->specDirty = false;
	}

	Path lPath;
	Path rPath;

	RectangleListType lRects, rRects;

	auto sf = UnblurryGraphics::getScaleFactorForComponent(parent, false);
	float width = (float)bounds.getWidth() * sf;

	VariantBuffer::Ptr r = rb.getBuffer();
	VariantBuffer::Ptr l = lb.getBuffer();

	if (l != nullptr && l->size != 0)
	{
		const float* data = l->buffer.getReadPointer(0);
		const int numSamples = l->size;

		calculatePath(lPath, width, data, numSamples, lRects, true);
	}

	if (r != nullptr && r->size != 0)
	{
		const float* data = r->buffer.getReadPointer(0);
		const int numSamples = r->size;

		calculatePath(rPath, width, data, numSamples, rRects, false);
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
			std::swap(newSpec, parent->spectrum);

            std::swap(parent->downsampledValues, tempBuffer);
            
			parent->isClear = false;

			parent->refresh();
		}
	}
}

void HiseAudioThumbnail::LoadingThread::scalePathFromLevels(Path &p, RectangleListType& rects, Rectangle<float> bounds, const float* data, const int numSamples, bool scaleVertically)
{
	if (!rects.isEmpty())
	{
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
		if(!std::isinf(bounds.getY()) && !std::isinf(bounds.getHeight()) &&
		   !std::isnan(bounds.getY()) && !std::isnan(bounds.getHeight()))
			p.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
	}
}

void HiseAudioThumbnail::LoadingThread::calculatePath(Path &p, float width, const float* l_, int numSamples, RectangleListType& rects, bool isLeft)
{
	auto rawStride = (float)numSamples / width;
    
	int stride = roundToInt(rawStride);

    auto downSamplingFactor = jlimit(1, 3, roundToInt(width / 1000));
    
	if (parent->currentOptions.manualDownSampleFactor != -1.0f)
		downSamplingFactor = (int)parent->currentOptions.manualDownSampleFactor;

	stride = jmax<int>(1, stride * downSamplingFactor);

	if (parent->currentOptions.displayMode == DisplayMode::DownsampledCurve)
	{
		p.clear();
		stride = jmax(1, roundToInt(rawStride));

        parent->currentOptions.useRectList = stride > JUCE_LIVE_CONSTANT_OFF(20);
        
        if(parent->currentOptions.useRectList)
            stride /= 2;
        
		auto numDownsampled = numSamples / stride;
        
        if(isLeft)
            tempBuffer = AudioSampleBuffer(parent->rBuffer.isBuffer() ? 2 : 1, numDownsampled);

		bool useMax = false;

        auto getBufferValue = [&](int i)
        {
            int numToCheck = jmin(numSamples - i, parent->currentOptions.useRectList ? stride * 2 : stride);
            auto range = FloatVectorOperations::findMinAndMax(l_ + i, numToCheck);

            float v = useMax ? range.getStart() : range.getEnd();

            if (stride < 10)
            {
                if (std::abs(range.getStart()) > std::abs(range.getEnd()))
                    v = range.getStart();
                else
                    v = range.getEnd();
            }
            
            v = jlimit<float>(-1.0f, 1.0f, v);
            
            return v;
        };
        
		for (int i = 0; i < numSamples; i += stride)
		{
            auto b1 = jlimit(0, numDownsampled - 1, i / stride);
            auto v1 = getBufferValue(i);
            
            useMax = !useMax;
            
			if (isPositiveAndBelow(b1, tempBuffer.getNumSamples()))
			{
				auto c = jmin(tempBuffer.getNumChannels()-1, isLeft ? 0 : 1);
				tempBuffer.setSample(c, b1, v1);
			}
				
		}

		return;
	}
    
	if (numSamples != 0)
	{
		p.clear();
        
		if (parent->shouldScaleVertically())
		{
			auto levels = FloatVectorOperations::findMinAndMax(l_, numSamples);
			auto gain = jmax(std::abs(levels.getStart()), std::abs(levels.getEnd()));

			p.startNewSubPath(0.0, 1.0f * gain);
			p.startNewSubPath(0.0, -1.f * gain);
		}
		else
		{
			p.startNewSubPath(0.0, -1.f);
			p.startNewSubPath(0.0, 1.f);
		}
		

        
        
		p.startNewSubPath(0.0f, 0.0f);
		
        bool useSymmetricWaveforms = stride > JUCE_LIVE_CONSTANT_OFF(60);
        
        if(parent->currentOptions.forceSymmetry != 0)
        {
            useSymmetricWaveforms = parent->currentOptions.forceSymmetry == 2;
        }
        
        if(useSymmetricWaveforms)
        {
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
                value = parent->applyDisplayGain(value);
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

        }
        else
        {
            if(rawStride > 1.0f)
            {
                for (int i = stride; i < numSamples; i += stride)
                {
                    if (threadShouldExit())
                        return;

                    const int numToCheck = jmin<int>(stride, numSamples - i);
                    auto value = jmax<float>(0.0f, FloatVectorOperations::findMaximum(l_ + i, numToCheck));
                    value = parent->applyDisplayGain(value);
                    p.lineTo((float)i, -1.0f * value);
                };
                
                for (int i = numSamples - 1; i >= 0; i -= stride)
                {
                    if (threadShouldExit())
                        return;

                    const int numToCheck = jmin<int>(stride, numSamples - i);
                    auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));
                    value = parent->applyDisplayGain(value);
                    p.lineTo((float)i, -1.0f * value);
                };
            }
            else
            {
                for (int i = 1; i < numSamples; i++)
                {
                    if (threadShouldExit())
                        return;

                    auto value = l_[i];
                    value = parent->applyDisplayGain(value);
                    p.lineTo((float)i, -1.0f * value);
                };
                
                p.lineTo((float)numSamples, 0.0f);
            }
        }
        
		p.closeSubPath();
	}
	else
	{
		p.clear();
	}
}


HiseAudioThumbnail::LookAndFeelMethods::~LookAndFeelMethods()
{}

HiseAudioThumbnail::RenderOptions HiseAudioThumbnail::LookAndFeelMethods::getThumbnailRenderOptions(
	HiseAudioThumbnail& te, const RenderOptions& defaultRenderOptions)
{ return defaultRenderOptions; }

bool HiseAudioThumbnail::shouldScaleVertically() const
{ return currentOptions.scaleVertically; }

void HiseAudioThumbnail::setShouldScaleVertically(bool shouldScale)
{
	currentOptions.scaleVertically = shouldScale;
}

Spectrum2D::Parameters::Ptr HiseAudioThumbnail::getParameters() const
{ return spectrumParameters; }

void HiseAudioThumbnail::lookAndFeelChanged()
{
	auto prevOptions = currentOptions;
        
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		currentOptions = laf->getThumbnailRenderOptions(*this, currentOptions);
        
	if(!(prevOptions == currentOptions))
		rebuildPaths();
}

void HiseAudioThumbnail::setDisplayMode(DisplayMode newDisplayMode)
{
	if (newDisplayMode != currentOptions.displayMode)
	{
		currentOptions.displayMode = newDisplayMode;
		rebuildPaths();
	}
}

void HiseAudioThumbnail::setManualDownsampleFactor(float newDownSampleFactor)
{
	FloatSanitizers::sanitizeFloatNumber(newDownSampleFactor);

	if (newDownSampleFactor == -1)
		currentOptions.manualDownSampleFactor = -1.0f;
	else
		currentOptions.manualDownSampleFactor = jlimit<float>(1.0f, 10.0f, newDownSampleFactor);

}

void HiseAudioThumbnail::setDrawHorizontalLines(bool shouldDrawHorizontalLines)
{
	currentOptions.drawHorizontalLines = shouldDrawHorizontalLines;
	repaint();
}

void HiseAudioThumbnail::handleAsyncUpdate()
{
	if (rebuildOnUpdate)
	{
		loadingThread.stopThread(-1);
		loadingThread.startThread(5);
				
		repaint();
		rebuildOnUpdate = false;
	}

	if (repaintOnUpdate)
	{
		repaint();
		repaintOnUpdate = false;
	}
		
}

void HiseAudioThumbnail::setRebuildOnResize(bool shouldRebuild)
{
	rebuildOnResize = shouldRebuild;
}

bool HiseAudioThumbnail::isEmpty() const noexcept
{
	return isClear || !lBuffer.isBuffer();
}

HiseAudioThumbnail::AudioDataProcessor& HiseAudioThumbnail::getAudioDataProcessor()
{ return sampleProcessor; }

float HiseAudioThumbnail::applyDisplayGain(float value)
{
	return jlimit(-1.0f, 1.0f, value * currentOptions.displayGain);
}

void HiseAudioThumbnail::refresh()
{
	repaintOnUpdate = true;
	triggerAsyncUpdate();
}

void HiseAudioThumbnail::rebuildPaths(bool synchronously)
{
	// refresh the options here...
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
	{
		if(!optionsInitialised)
		{
			currentOptions = laf->getThumbnailRenderOptions(*this, currentOptions);
			optionsInitialised = !currentOptions.dynamicOptions;
		}
	}
	
	auto numActualSamples = lengthInSeconds * sampleRate;

	if(currentOptions.manualDownSampleFactor > 0.0)
		numActualSamples /= currentOptions.manualDownSampleFactor;

	if(roundToInt(numActualSamples) < currentOptions.multithreadThreshold)
	{
		synchronously = true;
	}
	
	if (synchronously)
	{
		isClear = true;
			
		loadingThread.run();

		Component::SafePointer<Component> thisSafe = this;

		auto f = [thisSafe]()
		{
			if (thisSafe.getComponent() != nullptr)
				thisSafe.getComponent()->repaint();
		};

		MessageManager::callAsync(f);
	}
	else
	{
		rebuildOnUpdate = true;
		triggerAsyncUpdate();
	}
}

HiseAudioThumbnail::LoadingThread::LoadingThread(HiseAudioThumbnail* parent_):
	Thread("Thumbnail Generator"),
	parent(parent_)
{
}

void HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area)
{
	Colour bgColour = th.findColour(AudioDisplayComponent::ColourIds::bgColour);
	Colour fillColour = th.findColour(AudioDisplayComponent::ColourIds::fillColour);
	Colour outlineColour = th.findColour(AudioDisplayComponent::ColourIds::outlineColour);

	if (!areaIsEnabled)
	{
		fillColour = fillColour.withMultipliedAlpha(0.3f);
		outlineColour = outlineColour.withMultipliedAlpha(0.3f);
		bgColour = bgColour.withMultipliedAlpha(0.3f);
	}

	if (!bgColour.isTransparent())
	{
		g.setColour(bgColour);
		g.fillRect(area);
	}
	

    float wAlpha = th.waveformAlpha * th.waveformAlpha;
    
	g.setColour(fillColour.withAlpha(0.15f * wAlpha));

	if (th.currentOptions.drawHorizontalLines)
	{
		g.drawHorizontalLine(area.getY() + area.getHeight() / 4, 0.0f, (float)th.getWidth());
		g.drawHorizontalLine(area.getY() + 3 * area.getHeight() / 4, 0.0f, (float)th.getWidth());
	}
}

void HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path)
{
    float wAlpha = th.waveformAlpha * th.waveformAlpha;
    
	Colour fillColour = th.findColour(AudioDisplayComponent::ColourIds::fillColour).withMultipliedAlpha(wAlpha);
	Colour outlineColour = th.findColour(AudioDisplayComponent::ColourIds::outlineColour).withMultipliedAlpha(wAlpha);

    
    
	if (!areaIsEnabled)
	{
		fillColour = fillColour.withMultipliedAlpha(0.3f);
		outlineColour = outlineColour.withMultipliedAlpha(0.3f);
	}

	if (!fillColour.isTransparent())
	{
		g.setColour(fillColour);
		g.fillPath(path);
	}

	if (!outlineColour.isTransparent())
	{
		g.setColour(outlineColour);
		g.strokePath(path, PathStrokeType(1.0f));
	}
}

void HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const RectangleListType& rectList)
{
    float wAlpha = th.waveformAlpha * th.waveformAlpha;
    
	Colour fillColour = th.findColour(AudioDisplayComponent::ColourIds::fillColour).withMultipliedAlpha(wAlpha);
	Colour outlineColour = th.findColour(AudioDisplayComponent::ColourIds::outlineColour).withMultipliedAlpha(wAlpha);

    if(th.currentOptions.displayMode == DisplayMode::DownsampledCurve)
        fillColour = outlineColour;
    
	if (!areaIsEnabled)
	{
		fillColour = fillColour.withMultipliedAlpha(0.3f);
		outlineColour = outlineColour.withMultipliedAlpha(0.3f);
	}

    if(!fillColour.isTransparent())
    {
        g.setColour(fillColour);
        g.fillRectList(rectList);
    }
}


void HiseAudioThumbnail::LookAndFeelMethods::drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area)
{
	Font f = GLOBAL_BOLD_FONT();
	g.setFont(f);

	g.setColour(Colours::white.withAlpha(0.3f));
	g.setColour(Colours::black.withAlpha(0.5f));
	g.fillRect(area);
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawRect(area, 1);
	g.drawText(text, area, Justification::centred);
}

Image HiseAudioThumbnail::createPreview(const AudioSampleBuffer* buffer, int width)
{
	jassert(buffer != nullptr);

	HiseAudioThumbnail thumbnail;

	thumbnail.setSize(width, 150);

	auto data = const_cast<float**>(buffer->getArrayOfReadPointers());

	VariantBuffer::Ptr l = new VariantBuffer(data[0], buffer->getNumSamples());

	var lVar = var(l.get());
	var rVar;

	thumbnail.lBuffer = var(l.get());

	if (data[1] != nullptr)
	{
		VariantBuffer::Ptr r = new VariantBuffer(data[1], buffer->getNumSamples());
		thumbnail.rBuffer = var(r.get());
	}

	thumbnail.setDrawHorizontalLines(true);

	thumbnail.loadingThread.run();

	return thumbnail.createComponentSnapshot(thumbnail.getLocalBounds());
}

HiseAudioThumbnail::HiseAudioThumbnail() :
	loadingThread(this),
	spectrumParameters(new Spectrum2D::Parameters())
{
	spectrumParameters->notifier.addListener(*this, [](HiseAudioThumbnail& t, Identifier id, int newValue)
		{
			t.specDirty = true;
			t.rebuildPaths();
		});

	setLookAndFeel(&defaultLaf);

	setColour(AudioDisplayComponent::ColourIds::bgColour, JUCE_LIVE_CONSTANT_OFF(Colours::transparentBlack));
	setColour(AudioDisplayComponent::ColourIds::fillColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xffcccccc)));
	setColour(AudioDisplayComponent::ColourIds::outlineColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xa2181818)));

	setInterceptsMouseClicks(false, false);
	setBufferedToImage(true);
}

HiseAudioThumbnail::~HiseAudioThumbnail()
{
    setLookAndFeel(nullptr);
	loadingThread.stopThread(400);
}

void HiseAudioThumbnail::setBufferAndSampleRate(double newSampleRate, var bufferL, var bufferR /*= var()*/, bool synchronously /*= false*/)
{
	if (newSampleRate > 0.0)
		sampleRate = newSampleRate;

	setBuffer(bufferL, bufferR, synchronously);
}

void HiseAudioThumbnail::setBuffer(var bufferL, var bufferR /*= var()*/, bool synchronously)
{
	ScopedLock sl(lock);

	currentReader = nullptr;

	const bool shouldBeNotEmpty = bufferL.isBuffer() && bufferL.getBuffer()->size != 0;
	const bool isNotEmpty = lBuffer.isBuffer() && lBuffer.getBuffer()->size != 0;

	if (!isNotEmpty && !shouldBeNotEmpty)
		return;

	lBuffer = bufferL;
	rBuffer = bufferR;

	if (auto l = bufferL.getBuffer())
	{
		lengthInSeconds = l->size / sampleRate;
	}

	rebuildPaths(synchronously);
}



void HiseAudioThumbnail::fillAudioSampleBuffer(AudioSampleBuffer& b)
{
	ScopedLock sl(lock);

	if (currentReader != nullptr)
	{

		b.setSize(currentReader->numChannels, (int)currentReader->lengthInSamples);
		currentReader->read(&b, 0, (int)currentReader->lengthInSamples, 0, true, true);
	}
	else
	{
		auto numChannels = rBuffer.isBuffer() ? 2 : 1;
		auto numSamples = lBuffer.isBuffer() ? lBuffer.getBuffer()->size : 0;

		b.setSize(numChannels, numSamples);

		if (auto lb = lBuffer.getBuffer())
			FloatVectorOperations::copy(b.getWritePointer(0), lb->buffer.getReadPointer(0), numSamples);

		if (auto rb = rBuffer.getBuffer())
			FloatVectorOperations::copy(b.getWritePointer(1), rb->buffer.getReadPointer(0), numSamples);
	}


}

juce::AudioSampleBuffer HiseAudioThumbnail::getBufferCopy(Range<int> sampleRange) const
{
	auto numChannels = rBuffer.isBuffer() ? 2 : 1;
	auto numSamples = lBuffer.isBuffer() ? lBuffer.getBuffer()->size : 0;

	if (numSamples == 0)
		return {};

	sampleRange.setEnd(jmin(numSamples, sampleRange.getEnd()));
	
	AudioSampleBuffer b(2, sampleRange.getLength());

	FloatVectorOperations::copy(b.getWritePointer(0), lBuffer.getBuffer()->buffer.getReadPointer(0, sampleRange.getStart()), sampleRange.getLength());

	if (numChannels == 2)
		FloatVectorOperations::copy(b.getWritePointer(1), rBuffer.getBuffer()->buffer.getReadPointer(0, sampleRange.getStart()), sampleRange.getLength());
	else
		FloatVectorOperations::copy(b.getWritePointer(1), b.getReadPointer(0), b.getNumSamples());


	return b;
}

void HiseAudioThumbnail::paint(Graphics& g)
{
	if (isClear)
		return;

	ScopedLock sl(lock);

	{
		g.setColour(Colours::black.withAlpha(spectrumAlpha));
		g.saveState();
		g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

		if (auto vp = findParentComponentOfClass<Viewport>())
		{
			auto b = getLocalBounds();
			auto vb = vp->getViewArea().withHeight(b.getHeight());

			auto normX = (float)vb.getX() / (float)b.getWidth();
			auto normR = (float)vb.getRight() / (float)b.getWidth();
			

#if 0
			auto specW = (float)spectrum.getWidth();
			Range<int> specRange(roundToInt(normX * specW), roundToInt(normR * specW));

			auto clipSpec = spectrum.getClippedImage({ specRange.getStart(), 0, specRange.getLength(), spectrum.getHeight() });
#endif


			auto specW = (float)spectrum.getHeight();
			Range<int> specRange(roundToInt(normX * specW), roundToInt(normR * specW));

			auto clipSpec = spectrum.getClippedImage({ 0, specRange.getStart(), spectrum.getWidth(), specRange.getLength() });


			Spectrum2D::draw(g, clipSpec, vb, Graphics::highResamplingQuality);

			//g.drawImageWithin(clipSpec, vb.getX(), vb.getY(), vb.getWidth(), vb.getHeight(), RectanglePlacement::stretchToFit);
		}
		else
		{
			Spectrum2D::draw(g, spectrum, getLocalBounds(), Graphics::highResamplingQuality);

			//g.drawImageWithin(spectrum, 0, 0, getWidth(), getHeight(), RectanglePlacement::stretchToFit);
		}

		g.restoreState();
	}

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
		g.saveState();
        g.excludeClipRegion(bounds);

        drawSection(g, false);
		g.restoreState();
    }
    else
    {
        drawSection(g, true);
    }
}

int HiseAudioThumbnail::getNextZero(int value) const
{
    if(!lBuffer.isBuffer())
        return value;
    
    const auto& b = lBuffer.getBuffer()->buffer;
    
    if(isPositiveAndBelow(value, b.getNumSamples()))
    {
        auto start = b.getSample(0, value);
        
        auto sig = start > 0.0f;
        
		int deltaUp = -1;
		int deltaDown = -1;

        for(int i = value; i < b.getNumSamples(); i++)
        {
			auto thisSample = b.getSample(0, i);

			if (thisSample == 0.0f)
				continue;

            auto thisSig = thisSample > 0.0f;
            
			if (sig != thisSig)
			{
				deltaUp = i;

				if (std::abs(b.getSample(0, i - 1)) < std::abs(b.getSample(0, i)))
					deltaUp = i - 1;

				break;
			}
        }

		for (int i = value; i >= 0; i--)
		{
			auto thisSample = b.getSample(0, i);

			if (thisSample == 0.0f)
				continue;

			auto thisSig = thisSample > 0.0f;

			if (sig != thisSig)
			{
				deltaDown = i;

				if (std::abs(b.getSample(0, i + 1)) < std::abs(b.getSample(0, i)))
					deltaDown = i + 1;

				break;
			}
		}

		if (deltaDown == -1 && deltaUp == -1)
			return value;
		if (deltaDown == -1)
			return deltaUp;
		if (deltaUp == -1)
			return deltaDown;

		if (std::abs(deltaUp - value) > std::abs(deltaDown - value))
			return deltaDown;
		else
			return deltaUp;
    }
    
    return value;
}

void HiseAudioThumbnail::drawSection(Graphics &g, bool enabled)
{
	bool isStereo = rBuffer.isBuffer();

	auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());

	if (laf == nullptr)
		return;

	if (!isStereo)
	{
		auto a = getLocalBounds();

		laf->drawHiseThumbnailBackground(g, *this, enabled, a);

		createCurvePathForCurrentView(true, a);

        if (!leftWaveform.isEmpty())
			laf->drawHiseThumbnailPath(g, *this, enabled, leftWaveform);
		else if (!leftPeaks.isEmpty())
			laf->drawHiseThumbnailRectList(g, *this, enabled, leftPeaks);
	}
	else
	{
		auto a1 = getLocalBounds();
		auto a2 = a1.removeFromBottom(a1.getHeight() / 2);

		laf->drawHiseThumbnailBackground(g, *this, enabled, a1);

        createCurvePathForCurrentView(true, a1);
		createCurvePathForCurrentView(false, a2);

		if (!leftWaveform.isEmpty())
			laf->drawHiseThumbnailPath(g, *this, enabled, leftWaveform);
		else if (!leftPeaks.isEmpty())
			laf->drawHiseThumbnailRectList(g, *this, enabled, leftPeaks);

		laf->drawHiseThumbnailBackground(g, *this, enabled, a2);

        g.setOpacity(waveformAlpha);
        
		if (!rightWaveform.isEmpty())
			laf->drawHiseThumbnailPath(g, *this, enabled, rightWaveform);
		else if (!rightPeaks.isEmpty())
			laf->drawHiseThumbnailRectList(g, *this, enabled, rightPeaks);

#if 0
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
#endif
	}
}

double HiseAudioThumbnail::getTotalLength() const
{
	return lengthInSeconds;
}

void HiseAudioThumbnail::setDisplayGain(float gainToApply, NotificationType notify)
{
	if (gainToApply != 1.0f)
		currentOptions.scaleVertically = false;

	if (gainToApply != currentOptions.displayGain)
	{
		currentOptions.displayGain = gainToApply;

		switch (notify)
		{
		case sendNotification:
		case sendNotificationSync: rebuildPaths(true);
		case sendNotificationAsync: rebuildPaths(false);
		default: break;
		}
	}
}

void HiseAudioThumbnail::setReader(AudioFormatReader* r, int64 actualNumSamples)
{
	currentReader = r;

	if (actualNumSamples == -1)
		actualNumSamples = currentReader != nullptr ? currentReader->lengthInSamples : 0;

	if (currentReader != nullptr)
	{
		lengthInSeconds = actualNumSamples / currentReader->sampleRate;
	}
	else
	{
		clear();
	}
		

	specDirty = true;
	rebuildPaths();
}

void HiseAudioThumbnail::clear()
{
	ScopedLock sl(lock);

	lBuffer = var();
	rBuffer = var();

	leftWaveform.clear();
	rightWaveform.clear();

	spectrum = {};
	isClear = true;
	currentReader = nullptr;

	repaint();
}

void HiseAudioThumbnail::resized()
{
	if (rebuildOnResize)
		rebuildPaths();
	else
	{
		repaint();
	}
			
}

void HiseAudioThumbnail::setSpectrumAndWaveformAlpha(float wAlpha, float sAlpha)
{
	auto wChanged = (spectrumAlpha == 0.0f) && (sAlpha != 0.0f);
	
    waveformAlpha = wAlpha;
	spectrumAlpha = sAlpha;

	if (wChanged)
	{
		specDirty = true;
		rebuildPaths();
	}

	repaint();
}

void HiseAudioThumbnail::setRange(const int left, const int right) 
{
	leftBound = left;
	rightBound = getWidth() - right;
	repaint();
}


void HiseAudioThumbnail::createCurvePathForCurrentView(bool isLeft, Rectangle<int> area)
{
	if (currentOptions.displayMode != DisplayMode::DownsampledCurve)
		return;

    auto& rToUse = isLeft ? leftPeaks : rightPeaks;
	auto& pToUse = isLeft ? leftWaveform : rightWaveform;

    pToUse.clear();
    rToUse.clear();
    
	if (downsampledValues.getNumSamples() == 0)
		return;

    Rectangle<int> vBounds = getLocalBounds();
    Rectangle<float> va = vBounds.toFloat();
    auto w = (float)area.getWidth();
    
	if (auto vp = findParentComponentOfClass<Viewport>())
	{
		// this must be the same size
		vBounds = vp->getViewedComponent()->getLocalBounds();
		jassert(vBounds.getWidth() == area.getWidth());

		
		
		va = vp->getViewArea().toFloat();
    }
		
    Range<float> rangeToDisplay(va.getX() / w, va.getRight() / w);

    auto numSamples = downsampledValues.getNumSamples();

    auto start = roundToInt((float)numSamples * rangeToDisplay.getStart());
    auto end = roundToInt((float)numSamples * rangeToDisplay.getEnd());

    start = jlimit(0, numSamples - 1, start);
    end = jlimit(0, numSamples - 1, end);

    auto getBufferValue = [&](int index)
    {
		auto c = jmin(downsampledValues.getNumChannels() - 1, isLeft ? 0 : 1);
        auto v = downsampledValues.getSample(c, index);
        v = applyDisplayGain(v);
        
        FloatSanitizers::sanitizeFloatNumber(v);
        return -1.0f * v;
    };
    
    if(!currentOptions.useRectList)
    {
        pToUse.preallocateSpace((end - start) + 3);
        
        pToUse.startNewSubPath((float)start, -1.0f);
        pToUse.startNewSubPath((float)end, 1.0f);
        pToUse.startNewSubPath(start, getBufferValue(start));
        
        for (int i = start + 1; i < end; i++)
        {
            pToUse.lineTo(i, getBufferValue(i));
        }
        
        pToUse.scaleToFit(va.getX(), (float)area.getY(), va.getWidth(), (float)area.getHeight(), false);
    }
    else
    {
        rToUse.ensureStorageAllocated(end - start);
        
        auto pw = (float)va.getWidth() / (float)(end - start);
        
        
        for (int i = start; i < end; i++)
        {
            auto v = std::abs(getBufferValue(i));
            auto x = (float)va.getX() + (i - start) * pw;
            auto y = (float)area.getCentreY() - v *  (float)area.getHeight() * 0.5f;
            auto w = roundToInt(pw * 1.5f);
            auto h = (float)area.getHeight() * v;
            
			RectangleListType::RectangleType r(x, y, w, h);

            rToUse.addWithoutMerging(r);
        }
    }
}

void XYZMultiChannelAudioBufferEditor::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (auto df = dynamic_cast<MultiChannelAudioBuffer*>(newData))
	{
		currentBuffer = df;
		rebuildButtons();
		rebuildEditor();
	}
}

void XYZMultiChannelAudioBufferEditor::addButton(const Identifier& id, const Identifier& currentId)
{
	auto tb = new TextButton(id.toString());
	tb->setClickingTogglesState(true);
	tb->setRadioGroupId(912451, dontSendNotification);

	bool shouldBeOn = (currentId == id ||
		id == Identifier("Single Sample")) && currentId.isNull();

	tb->setToggleState(shouldBeOn, dontSendNotification);
	addAndMakeVisible(tb);
	tb->addListener(this);

	tb->setLookAndFeel(getSpecialLookAndFeel<LookAndFeel>());

	buttons.add(tb);
}

void XYZMultiChannelAudioBufferEditor::buttonClicked(Button* b)
{
	Identifier id(b->getName());

	if (currentBuffer != nullptr)
	{
		currentBuffer->setXYZProvider(id);
		rebuildEditor();
	}
}

void XYZMultiChannelAudioBufferEditor::rebuildButtons()
{
	buttons.clear();

	if (currentBuffer != nullptr)
	{
		auto l = currentBuffer->getAvailableXYZProviders();
		auto cId = currentBuffer->getCurrentXYZId();

		addButton("Single Sample", cId);

		for (auto id : l)
			addButton(id, cId);
	}
}

void XYZMultiChannelAudioBufferEditor::rebuildEditor()
{
	if (currentBuffer != nullptr)
	{
		currentEditor = dynamic_cast<Component*>(currentBuffer->createEditor());

		addAndMakeVisible(currentEditor);
		resized();
	}
}

void XYZMultiChannelAudioBufferEditor::paint(Graphics& g)
{
}

void XYZMultiChannelAudioBufferEditor::resized()
{
	auto b = getLocalBounds();
	auto top = b.removeFromTop(24);

	if (!buttons.isEmpty())
	{
		int bWidth = getWidth() / buttons.size();

		for (auto tb : buttons)
			tb->setBounds(top.removeFromLeft(bWidth));
	}

	if (currentEditor != nullptr)
		currentEditor->setBounds(b);
}

void MultiChannelAudioBufferDisplay::setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn)
{
	preview->setLookAndFeel(l);
	EditorBase::setSpecialLookAndFeel(l, shouldOwn);
}

void MultiChannelAudioBufferDisplay::setShowFileName(bool shouldShowFileName)
{
	showFileName = shouldShowFileName;
	repaint();
}

void MultiChannelAudioBufferDisplay::rangeChanged(AudioDisplayComponent*, int)
{
	auto range = areas[0]->getSampleRange();

	if (connectedBuffer != nullptr)
	{
		connectedBuffer->setRange(range);
	}
}

void MultiChannelAudioBufferDisplay::setBackgroundColour(Colour c)
{ bgColour = c; }

void MultiChannelAudioBufferDisplay::mouseDoubleClick(const MouseEvent& mouseEvent)
{
	if (connectedBuffer != nullptr)
	{
		MultiChannelAudioBuffer::ScopedUndoActivator sv(*connectedBuffer);
		connectedBuffer->fromBase64String({});
	}
}

String MultiChannelAudioBufferDisplay::getCurrentlyLoadedFileName() const
{
	if (connectedBuffer != nullptr)
	{
		auto a = connectedBuffer->toBase64String();

		if (a == "-1")
			return {};

		return a;
	}

	return {};
}

double MultiChannelAudioBufferDisplay::getSampleRate() const
{
	if (connectedBuffer != nullptr)
		return connectedBuffer->sampleRate;

	return 0.0;
}

void MultiChannelAudioBufferDisplay::setShowLoop(bool shouldShowLoop)
{
	if (showLoop != shouldShowLoop)
	{
		showLoop = shouldShowLoop;

		WeakReference<Component> safeThis(this);

		MessageManager::callAsync([safeThis]()
		{
			if (safeThis != nullptr)
				safeThis->repaint();
		});
	}
}

void MultiChannelAudioBufferDisplay::bufferWasLoaded()
{
	Component::SafePointer<MultiChannelAudioBufferDisplay> safeThis(this);

	auto f = [safeThis]()
	{
		if (safeThis == nullptr)
			return;

		auto cb = safeThis.getComponent()->connectedBuffer;

		if (cb != nullptr)
			safeThis->preview->setBufferAndSampleRate(cb->sampleRate, cb->getChannelBuffer(0, true), cb->getChannelBuffer(1, true));
		else
			safeThis->preview->setBuffer({}, {});

		auto shouldShowLoop = cb != nullptr && cb->getLoopRange() != cb->getCurrentRange();
		safeThis->setShowLoop(shouldShowLoop);

		safeThis->updateRanges(nullptr);
	};

	if (MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
		f();
	else
		MessageManager::callAsync(f);
		
}

void MultiChannelAudioBufferDisplay::bufferWasModified()
{
	updateRanges(nullptr);
}

void MultiChannelAudioBufferDisplay::sampleIndexChanged(int newSampleIndex)
{
	if (connectedBuffer != nullptr)
	{
		auto s = connectedBuffer->getCurrentRange().getLength();
		AudioDisplayComponent::setPlaybackPosition((double)newSampleIndex / s);
	}
}

void MultiChannelAudioBufferDisplay::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(newData))
		setAudioFile(af);
}

void MultiChannelAudioBufferDisplay::setAudioFile(MultiChannelAudioBuffer* af)
{
	if (af != connectedBuffer)
	{
		if (connectedBuffer != nullptr)
			connectedBuffer->removeListener(this);

		connectedBuffer = af;
		bufferWasLoaded();

		if (connectedBuffer != nullptr)
			connectedBuffer->addListener(this);
	}
}

MultiChannelAudioBuffer* MultiChannelAudioBufferDisplay::getBuffer()
{ return connectedBuffer.get(); }

MultiChannelAudioBufferDisplay::MultiChannelAudioBufferDisplay() :
	AudioDisplayComponent(),
	itemDragged(false),
	bgColour(Colour(0xFF555555))
{
	setColour(ColourIds::bgColour, Colour(0xFF555555));

	setSpecialLookAndFeel(new BufferLookAndFeel(), true);

	setOpaque(true);

	areas.add(new SampleArea(AreaTypes::PlayArea, this));
	addAndMakeVisible(areas[0]);
	areas[0]->setAreaEnabled(true);
	addAreaListener(this);

	static const unsigned char pathData[] = { 110,109,0,23,2,67,128,20,106,67,108,0,0,230,66,128,92,119,67,108,0,23,2,67,128,82,130,67,108,0,23,2,67,128,92,123,67,108,0,0,7,67,128,92,123,67,98,146,36,8,67,128,92,123,67,243,196,9,67,58,18,124,67,128,5,11,67,128,88,125,67,98,13,70,12,67,198,158,126,
		67,0,0,13,67,53,39,128,67,0,0,13,67,64,197,128,67,98,0,0,13,67,109,97,129,67,151,72,12,67,91,58,130,67,0,11,11,67,128,221,130,67,98,105,205,9,67,165,128,131,67,219,50,8,67,0,220,131,67,0,0,7,67,0,220,131,67,108,0,0,210,66,0,220,131,67,98,30,54,205,66,
		0,220,131,67,0,0,198,66,234,39,130,67,0,0,198,66,64,197,128,67,98,0,0,198,66,202,43,128,67,60,123,199,66,26,166,126,67,255,10,202,66,0,92,125,67,98,196,154,204,66,230,17,124,67,238,244,207,66,128,92,123,67,0,0,210,66,128,92,123,67,108,0,240,223,66,128,
		92,123,67,108,0,240,223,66,128,92,115,67,108,0,0,210,66,128,92,115,67,98,241,91,202,66,128,92,115,67,115,181,195,66,237,49,117,67,0,177,190,66,128,184,119,67,98,141,172,185,66,18,63,122,67,0,0,182,66,178,164,125,67,0,0,182,66,64,197,128,67,98,0,0,182,
		66,251,172,132,67,16,201,194,66,0,220,135,67,0,0,210,66,0,220,135,67,108,0,0,7,67,0,220,135,67,98,51,228,10,67,0,220,135,67,218,73,14,67,139,238,134,67,128,198,16,67,128,167,133,67,98,37,67,19,67,117,96,132,67,0,0,21,67,8,174,130,67,0,0,21,67,64,197,
		128,67,98,0,0,21,67,186,175,125,67,243,57,19,67,94,72,122,67,128,186,16,67,128,189,119,67,98,13,59,14,67,162,50,117,67,110,219,10,67,128,92,115,67,0,0,7,67,128,92,115,67,108,0,23,2,67,128,92,115,67,108,0,23,2,67,128,20,106,67,99,101,0,0 };

	loopPath.loadPathFromData(pathData, sizeof(pathData));
}

MultiChannelAudioBufferDisplay::~MultiChannelAudioBufferDisplay()
{
	setAudioFile(nullptr);
}


void MultiChannelAudioBufferDisplay::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	over = isInterestedInDragSource(dragSourceDetails);
	repaint();
}

void MultiChannelAudioBufferDisplay::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	over = false;
	repaint();
}

bool MultiChannelAudioBufferDisplay::isInterestedInFileDrag(const StringArray &files)
{
	return files.size() == 1 && isAudioFile(files[0]);
}

bool MultiChannelAudioBufferDisplay::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	return isAudioFile(dragSourceDetails.description.toString());
}

void MultiChannelAudioBufferDisplay::itemDropped(const SourceDetails& dragSourceDetails)
{
	if (connectedBuffer != nullptr)
	{
		MultiChannelAudioBuffer::ScopedUndoActivator sv(*connectedBuffer);
		connectedBuffer->fromBase64String(dragSourceDetails.description.toString());
	}
}

bool MultiChannelAudioBufferDisplay::isAudioFile(const String &s)
{
	AudioFormatManager afm;

	afm.registerBasicFormats();
#if !HISE_NO_GUI_TOOLS
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
#endif

	if (File::isAbsolutePath(s))
	{
		return File(s).existsAsFile() && afm.findFormatForFileExtension(File(s).getFileExtension()) != nullptr;
	}

	return false;	
}


void MultiChannelAudioBufferDisplay::filesDropped(const StringArray &fileNames, int, int)
{
	if (fileNames.size() > 0 && connectedBuffer != nullptr)
	{
		MultiChannelAudioBuffer::ScopedUndoActivator sv(*connectedBuffer);
		connectedBuffer->fromBase64String(fileNames[0]);
	}
}

void MultiChannelAudioBufferDisplay::updateRanges(SampleArea *areaToSkip/*=nullptr*/)
{
	areas[PlayArea]->setSampleRange(connectedBuffer != nullptr ? connectedBuffer->getCurrentRange() : Range<int>());
	refreshSampleAreaBounds(areaToSkip);
}

void MultiChannelAudioBufferDisplay::setRange(Range<int> newRange)
{
	const bool isSomethingLoaded = connectedBuffer != nullptr && connectedBuffer->toBase64String().isNotEmpty();

	getSampleArea(0)->setVisible(isSomethingLoaded);

	if (getSampleArea(0)->getSampleRange() != newRange)
	{
		getSampleArea(0)->setSampleRange(newRange);
		refreshSampleAreaBounds();
	}
}



void MultiChannelAudioBufferDisplay::mouseDown(const MouseEvent &e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (connectedBuffer != nullptr && (e.mods.isRightButtonDown() || e.mods.isCtrlDown() || (e.mods.isLeftButtonDown() && loadWithLeftClick)))
	{
		if (auto p = connectedBuffer->getProvider())
		{
			String patterns = "*.wav;*.aif;*.aiff;*.WAV;*.AIFF;*.hlac;*.flac;*.HLAC;*.FLAC";

			File searchDirectory = connectedBuffer->getProvider()->getRootDirectory();

			auto f = connectedBuffer->getProvider()->parseFileReference(connectedBuffer->toBase64String());

			if(f.existsAsFile())
				searchDirectory = f.getParentDirectory();
			
			FileChooser fc("Load File", searchDirectory, patterns, true);

			if (fc.browseForFileToOpen())
			{
				auto f = fc.getResult();

				MultiChannelAudioBuffer::ScopedUndoActivator sv(*connectedBuffer);
				connectedBuffer->fromBase64String(f.getFullPathName());
			}
		}
	}
}

void MultiChannelAudioBufferDisplay::paint(Graphics &g)
{


	bgColour = findColour(AudioDisplayComponent::ColourIds::bgColour);
	g.fillAll(bgColour);

	//AudioDisplayComponent::paint(g);

	if (over)
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(getLocalBounds(), 2);
	}
}


void MultiChannelAudioBufferDisplay::paintOverChildren(Graphics& g)
{
	auto laf = dynamic_cast<HiseAudioThumbnail::LookAndFeelMethods*>(&preview->getLookAndFeel());

	jassert(laf != nullptr);


	static const String text = "Drop audio file or Right click to open browser";

	auto f = GLOBAL_BOLD_FONT();

	const int w = f.getStringWidth(text) + 20;

	if (getWidth() > (w + 10) && (connectedBuffer == nullptr || connectedBuffer->getBuffer().getNumSamples() == 0))
	{
		Rectangle<int> r((getWidth() - w) / 2, (getHeight() - 20) / 2, w, 20);
		laf->drawTextOverlay(g, *preview, text, r.toFloat());
	}

	AudioDisplayComponent::paintOverChildren(g);

	String fileNameToShow = getCurrentlyLoadedFileName();

	if (showFileName && fileNameToShow.isNotEmpty())
	{
		fileNameToShow = fileNameToShow.replace("\\", "/");
		fileNameToShow = fileNameToShow.fromLastOccurrenceOf("}", false, false);
		fileNameToShow = fileNameToShow.fromLastOccurrenceOf("/", false, false);


		const int w2 = f.getStringWidth(fileNameToShow) + 20;
		Rectangle<int> r(getWidth() - w2 - 5, 5, w2, 20);
		laf->drawTextOverlay(g, *preview, fileNameToShow, r.toFloat());
	}

	if (showLoop)
	{
		if (connectedBuffer != nullptr && !connectedBuffer->isEmpty())
		{
			auto loopRange = connectedBuffer->getLoopRange();

			float factor = (float)getWidth() / (float)connectedBuffer->getTotalRange().getLength();

			xPositionOfLoop.setStart((float)loopRange.getStart() * factor);
			xPositionOfLoop.setEnd((float)loopRange.getEnd() * factor);
		}

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

void MultiChannelAudioBuffer::setXYZProvider(const Identifier& id)
{
	if (id.isNull() || id.toString() == "Single Sample" || deactivatedXYZIds.contains(id))
	{
		xyzProvider = nullptr;
	}
	else
	{
		if (xyzProvider == nullptr || xyzProvider->getId() != id)
			xyzProvider = factory->create(id);
	}
}

struct UndoableBufferLoad: public UndoableAction
{
	UndoableBufferLoad(MultiChannelAudioBuffer::Ptr buffer_, const String& b64):
	  buffer(buffer_),
	  newFile(b64),
	  oldFile(buffer->toBase64String())
	{}

	bool perform() override
	{
		if(buffer != nullptr)
		{
			MultiChannelAudioBuffer::ScopedUndoActivator svs(*dynamic_cast<MultiChannelAudioBuffer*>(buffer.get()), false);
			auto unusedOk = buffer->fromBase64String(newFile);
			// We need to ignore the load success (because it will return false when you clear a sample)
			ignoreUnused(unusedOk);
			return true;
		}

		return false;
	}

	bool undo() override
	{
		if(buffer != nullptr)
		{
			MultiChannelAudioBuffer::ScopedUndoActivator svs(*dynamic_cast<MultiChannelAudioBuffer*>(buffer.get()), false);
			auto unusedOk = buffer->fromBase64String(oldFile);
			ignoreUnused(unusedOk);
			return true;
		}

		return false;
	}

	MultiChannelAudioBuffer::Ptr buffer;
	String newFile, oldFile;
};

bool MultiChannelAudioBuffer::fromBase64String(const String& b64)
{
	if(auto um = getUndoManager(useUndoManagerForLoadOperation))
	{
		return um->perform(new UndoableBufferLoad(this, b64));
	}
	else
	{
		if (b64 != referenceString)
		{
			referenceString = b64;
			
			if (referenceString.isEmpty() && xyzProvider != nullptr)
			{
				SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
				xyzItems.clear();
				getUpdater().sendContentRedirectMessage();
				return true;
			}

			Identifier xyzId = XYZProviderFactory::parseID(referenceString);

			if (xyzId.isValid())
			{
				setXYZProvider(xyzId);

				if (xyzProvider != nullptr)
				{
					SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
					xyzItems.clear();

					try
					{
						auto ok = xyzProvider->parse(b64, xyzItems);

						getUpdater().sendContentRedirectMessage();

						return ok;
					}
					catch (String&)
					{
						jassertfalse;
						return false;
					}
				}

				return false;
			}
			else
			{
				xyzProvider = nullptr;

				jassert(provider != nullptr);

				if (provider != nullptr)
				{
					if (auto lr = provider->loadFile(referenceString))
					{
						originalBuffer = lr->buffer;
						auto nb = createNewDataBuffer({ 0, originalBuffer.getNumSamples() });

						referenceString = lr->reference;

						{
							SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
							bufferRange = { 0, originalBuffer.getNumSamples() };
							sampleRate = lr->sampleRate;
							setLoopRange(lr->loopRange, dontSendNotification);
							setDataBuffer(nb);
						}

						return true;
					}
					else
					{
						SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
						originalBuffer = {};
						bufferRange = {};
						currentData = {};
						getUpdater().sendContentRedirectMessage();
						return false;
					}
				}
			}
		}
	}

	return false;
}

hise::ComplexDataUIBase::EditorBase* MultiChannelAudioBuffer::createEditor()
{
	if (xyzProvider != nullptr)
	{
		auto c = xyzProvider->createEditor(this);
		c->setComplexDataUIBase(this);
		return c;
	}
	else
	{
		auto c = new MultiChannelAudioBufferDisplay();
		c->setComplexDataUIBase(this);
		return c;
	}
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr MultiChannelAudioBuffer::XYZProviderBase::loadFileFromReference(const String& f)
{
	if (pool != nullptr)
	{
		if (auto pr = pool->loadFile(f))
		{
			if (*pr)
				return pr;
		}
	}

	auto lr = getDataProvider()->loadFile(f);

	if (!lr->r.wasOk())
		throw lr->r.getErrorMessage();

	pool->pool.add(lr);

	return lr;
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr MultiChannelAudioBuffer::DataProvider::loadAbsoluteFile(const File& f, const String& refString)
{
	// call registerBasicFormats and thank me later...
	jassert(afm.getNumKnownFormats() > 0);

	auto fis = new FileInputStream(f);

	auto reader = afm.createReaderFor(std::unique_ptr<InputStream>(fis));

	if (reader != nullptr)
	{
		MultiChannelAudioBuffer::SampleReference::Ptr lr = new MultiChannelAudioBuffer::SampleReference();
		lr->buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
		reader->read(&lr->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
		lr->reference = refString;
		lr->sampleRate = reader->sampleRate;
		return lr;
	}

	return new MultiChannelAudioBuffer::SampleReference(false, f.getFileName() + " can't be loaded");
}

} // namespace hise
