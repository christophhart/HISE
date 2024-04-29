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


SimpleRingBuffer::SimpleRingBuffer()
{
	getUpdater().addEventListener(this);
	setPropertyObject(new PropertyObject(nullptr));
}

void SimpleRingBuffer::setupReadBuffer(AudioSampleBuffer& b)
{
    ScopedLock sl(getReadBufferLock());
	auto numChannels = internalBuffer.getNumChannels();
	auto numSamples = internalBuffer.getNumSamples();

	if (b.getNumChannels() != numChannels || b.getNumSamples() != numSamples)
	{
		// must be called during a write lock
		jassert(getDataLock().writeAccessIsLocked());

		Array<var> newData;

		for (int i = 0; i < numChannels; i++)
		{
			auto p = new VariantBuffer(numSamples);
			externalBufferChannels[i] = p->buffer.getWritePointer(0);
			newData.add(var(p));
		}

		newData.swapWith(externalBufferData);
		b.setDataToReferTo(externalBufferChannels, numChannels, numSamples);
	}
}

void SimpleRingBuffer::clear()
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		internalBuffer.clear();
		numAvailable = 0;
		writeIndex = 0;
		updateCounter = 0;
	}
}

int SimpleRingBuffer::read(AudioSampleBuffer& b)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		while (isBeingWritten.load()) // busy wait, but OK
		{

		}

		int numSamples = b.getNumSamples();
		int numChannels = b.getNumChannels();

		if(maxLength != -1.0)
		{
			numSamples = getMaxLengthInSamples();
			
			for (int i = 0; i < numChannels; i++)
			{
				auto buffer = internalBuffer.getReadPointer(i);
				auto dst = b.getWritePointer(i);
				
				FloatVectorOperations::copy(dst, buffer, numSamples);
			}
		
			interpolatedReadIndex = hmath::fmod(interpolatedReadIndex + maxLength, (double)internalBuffer.getNumSamples());
			
		}
		else
		{
			bool shortBuffer = internalBuffer.getNumSamples() < 4096;

			int thisWriteIndex = shortBuffer ? 0 : writeIndex.load();
			
			int numBeforeIndex = thisWriteIndex;
			int offsetBeforeIndex = internalBuffer.getNumSamples() - numBeforeIndex;

			jassert(b.getNumSamples() == internalBuffer.getNumSamples() && b.getNumChannels() == internalBuffer.getNumChannels());

			for (int i = 0; i < numChannels; i++)
			{
				auto buffer = internalBuffer.getReadPointer(i);
				auto dst = b.getWritePointer(i);

				if (shortBuffer)
				{
					FloatVectorOperations::copy(dst, buffer, offsetBeforeIndex);
					//FloatVectorOperations::multiply(dst, 0.5f, b.getNumSamples());
					//FloatVectorOperations::addWithMultiply(dst, buffer, 0.5f, offsetBeforeIndex);
				}
				else
				{
					FloatVectorOperations::copy(dst + offsetBeforeIndex, buffer, numBeforeIndex);
					FloatVectorOperations::copy(dst, buffer + thisWriteIndex, offsetBeforeIndex);
				}

				
				FloatSanitizers::sanitizeArray(dst, numSamples);
			}

			int numToReturn = numAvailable;
			numAvailable = 0;
			return numToReturn;
		}
	}

	return 0;
}

void SimpleRingBuffer::write(double value, int numSamples)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		if (numSamples == 1)
		{
			isBeingWritten = true;

			for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				internalBuffer.setSample(i, writeIndex, (float)value);

			if (++writeIndex >= internalBuffer.getNumSamples())
				writeIndex.store(0);

			++numAvailable;

			isBeingWritten = false;

			if (updateCounter++ >= 1024)
			{
				getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
				updateCounter = 0;
			}
		}
		else
		{
			isBeingWritten = true;

			int numBeforeWrap = jmin(numSamples, internalBuffer.getNumSamples() - writeIndex);
			int numAfterWrap = numSamples - numBeforeWrap;

			if (numBeforeWrap > 0)
			{
				for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				{
					auto buffer = internalBuffer.getWritePointer(i);
					FloatVectorOperations::fill(buffer + writeIndex, (float)value, numBeforeWrap);
				}
			}

			writeIndex += numBeforeWrap;

			if (numAfterWrap > 0)
			{
				for (int i = 0; i < internalBuffer.getNumChannels(); i++)
				{
					auto buffer = internalBuffer.getWritePointer(i);
					FloatVectorOperations::fill(buffer, (float)value, numAfterWrap);
				}

				writeIndex = numAfterWrap;
			}

			numAvailable += numSamples;
			isBeingWritten = false;

			getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
		}
	}
}

void SimpleRingBuffer::write(const float** data, int numChannels, int numSamples)
{
	if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getDataLock()))
	{
		if (internalBuffer.getNumSamples() == 0)
			return;

		isBeingWritten = true;

		if (numSamples > 0)
		{
			int numChannelsToWrite = jmin(numChannels, internalBuffer.getNumChannels());

			if(maxLength != -1.0)
			{
				auto upperLimit = maxLength;
				auto numToWrite = (double)numSamples;

				while(numToWrite > 0)
				{
					auto numThisTime = jmin(upperLimit, (double)numToWrite);

					auto numBeforeWrap = jlimit(0.0, jmin(numToWrite, upperLimit), upperLimit - interpolatedWriteIndex);
					auto numAfterWrap = jmax(0.0, numThisTime - numBeforeWrap);

					for(int i = 0; i < numChannelsToWrite; i++)
					{
						auto buffer = internalBuffer.getWritePointer(i);

						FloatVectorOperations::copy(buffer + roundToInt(interpolatedWriteIndex), data[i], roundToInt(numBeforeWrap));
						FloatVectorOperations::copy(buffer, data[i] + roundToInt(numBeforeWrap), roundToInt(numAfterWrap));
					}

					interpolatedWriteIndex = hmath::fmod(interpolatedWriteIndex + numThisTime, maxLength);

					numToWrite -= numThisTime;
				}

				
			}
			else
			{
				
				int numBeforeWrap = jmin(numSamples, internalBuffer.getNumSamples() - writeIndex);

				if (numBeforeWrap > 0)
				{
					for (int i = 0; i < numChannelsToWrite; i++)
					{
						auto buffer = internalBuffer.getWritePointer(i);
						FloatVectorOperations::copy(buffer + writeIndex, data[i], numBeforeWrap);
					}
				}

				writeIndex += numBeforeWrap;
				int numAfterWrap = numSamples - numBeforeWrap;

				if(numAfterWrap > 0)
				{
					auto numThisTime = jmin(internalBuffer.getNumSamples(), numAfterWrap);

					for (int i = 0; i < numChannelsToWrite; i++)
					{
						auto buffer = internalBuffer.getWritePointer(i);
						FloatVectorOperations::copy(buffer, data[i] + numBeforeWrap, numThisTime);
					}

					writeIndex = (writeIndex + numAfterWrap) % internalBuffer.getNumSamples();
				}

				numAvailable += numSamples;
			}
		}

		isBeingWritten = false;

		getUpdater().sendDisplayChangeMessage((int)numAvailable, sendNotificationAsync, true);
	}
}

void SimpleRingBuffer::write(const AudioSampleBuffer& b, int startSample, int numSamples)
{
	if (startSample != 0)
	{
		auto numBytes = sizeof(float*)*b.getNumChannels();
		const float** channels = (const float**)alloca(numBytes);

		memcpy(channels, b.getArrayOfReadPointers(), numBytes);
		for (int i = 0; i < b.getNumChannels(); i++)
			channels[i] += startSample;

		write(channels, b.getNumChannels(), b.getNumSamples());
	}

	write(b.getArrayOfReadPointers(), b.getNumChannels(), b.getNumSamples());
}

void SimpleRingBuffer::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n)
{
	if(t == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
		setupReadBuffer(externalBuffer);
	else
	{
        ScopedLock sl(getReadBufferLock());
        
		read(externalBuffer);

		if (properties != nullptr && getReferenceCount() > 1)
			properties->transformReadBuffer(externalBuffer);
	}
}

void SimpleRingBuffer::setProperty(const Identifier& id, const var& newValue)
{
	if (properties != nullptr)
		properties->setProperty(id, newValue);
}

var SimpleRingBuffer::getProperty(const Identifier& id) const
{
	if (properties != nullptr)
		return properties->getProperty(id);

	return {};
}

juce::Array<juce::Identifier> SimpleRingBuffer::getIdentifiers() const
{
	if (properties != nullptr)
		return properties->getPropertyList();

	return {};
}

void SimpleRingBuffer::setPropertyObject(PropertyObject* newObject)
{
#if HI_EXPORT_AS_PROJECT_DLL
	jassertfalse;
#endif

	properties = newObject;

	properties->initialiseRingBuffer(this);

	auto ns = internalBuffer.getNumSamples();
	auto nc = internalBuffer.getNumChannels();

	bool updateBuffer = false;

	if (ns == 0 && properties->getPropertyList().contains(RingBufferIds::BufferLength))
	{
		ns = (int)properties->getPropertyInternal(RingBufferIds::BufferLength.toString());
		updateBuffer = true;
	}

	if (nc == 0 && properties->getPropertyList().contains(RingBufferIds::NumChannels))
	{
		nc = (int)properties->getPropertyInternal(RingBufferIds::NumChannels.toString());
		updateBuffer = true;
	}
		

	if (validateChannels(nc) ||
		validateLength(ns) || updateBuffer)
	{
		setRingBufferSize(nc, ns);
	}
		

	getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	clear();
}

bool SimpleRingBuffer::validateChannels(int& v)
{
	if (properties != nullptr)
		return properties->validateInt("NumChannels", v);

	return false;
}

bool SimpleRingBuffer::validateLength(int& v)
{
	if (properties != nullptr)
		return properties->validateInt("BufferLength", v);

	return false;
}

ModPlotter::ModPlotter()
{
	setColour(backgroundColour, Colours::black.withAlpha(0.3f));
	setColour(outlineColour, Colours::black.withAlpha(0.3f));

	setColour(pathColour, Colour(0xFF999999));

	setOpaque(true);
	setInterceptsMouseClicks(false, false);
}

void ModPlotter::paint(Graphics& g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	laf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());
	laf->drawOscilloscopePath(g, *this, p);

}

juce::Colour ModPlotter::getColourForAnalyserBase(int colourId)
{
	switch (colourId)
	{
	case RingBufferComponentBase::ColourId::bgColour: return findColour(ColourIds::backgroundColour);
	case RingBufferComponentBase::ColourId::fillColour: return findColour(ColourIds::pathColour);
	case RingBufferComponentBase::ColourId::lineColour: return findColour(ColourIds::outlineColour);
    default:
        return Colours::transparentBlack;
	}
}

int ModPlotter::getSamplesPerPixel(float rectangleWidth) const
{
	float offset = 2.0f;

	auto width = (float)getWidth() - 2.0f * offset;

	if (rb != nullptr)
	{
		auto numSamples = (int)rb->getReadBuffer().getNumSamples();

		int samplesPerPixel = numSamples / jmax((int)(width / rectangleWidth), 1);
		return samplesPerPixel;
	}

	return 1;
}

void RingBufferComponentBase::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	SafeAsyncCall::callAsyncIfNotOnMessageThread<RingBufferComponentBase>(*this, [](RingBufferComponentBase& c)
	{
		c.refresh();
	});
}

void RingBufferComponentBase::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (rb != nullptr)
		rb->getUpdater().removeEventListener(this);

	rb = dynamic_cast<SimpleRingBuffer*>(newData);

	if (rb != nullptr)
	{
		rb->getUpdater().addEventListener(this);
	}

	refresh();
}

RingBufferComponentBase::LookAndFeelMethods::~LookAndFeelMethods()
{}

RingBufferComponentBase::RingBufferComponentBase()
{
	setSpecialLookAndFeel(new DefaultLookAndFeel(), true);
}

Colour RingBufferComponentBase::getColourForAnalyserBase(int colourId)
{ 
	switch (colourId)
	{
	case ColourId::bgColour: return Colours::transparentBlack;
	case ColourId::fillColour: return Colours::white.withAlpha(0.05f);
	case ColourId::lineColour: return Colours::white.withAlpha(0.9f);
	}
		
	return Colours::transparentBlack;
}

ModPlotter::ModPlotterPropertyObject::ModPlotterPropertyObject(SimpleRingBuffer::WriterBase* wb):
	PropertyObject(wb)
{
	setProperty(RingBufferIds::BufferLength, 32768 * 2);
	setProperty(RingBufferIds::NumChannels, 1);
}

int ModPlotter::ModPlotterPropertyObject::getClassIndex() const
{ return PropertyIndex; }

bool ModPlotter::ModPlotterPropertyObject::allowModDragger() const
{ return true; }

bool ModPlotter::ModPlotterPropertyObject::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
	{
		bool wasPowerOfTwo = isPowerOfTwo(v);

		if (!wasPowerOfTwo)
			v = nextPowerOfTwo(v);
				
		return SimpleRingBuffer::withinRange<4096, 32768 * 4>(v) && wasPowerOfTwo;
	}

	if (id == RingBufferIds::NumChannels)
		return SimpleRingBuffer::toFixSize<1>(v);

	return false;
}

RingBufferComponentBase* ModPlotter::ModPlotterPropertyObject::createComponent()
{
	return new ModPlotter();
}

void ModPlotter::ModPlotterPropertyObject::transformReadBuffer(AudioSampleBuffer& b)
{
	if (transformFunction)
		transformFunction(b.getWritePointer(0), b.getNumSamples());
}

Rectangle<int> ModPlotter::getFixedBounds() const
{ return { 0, 0, 256, 80 }; }

void ModPlotter::refresh()
{
	if (rb != nullptr)
	{
		jassert(getSamplesPerPixel(1.0f) > 0);
		
		float offset = 2.0f;

		float rectangleWidth = 1.0f;

		rectangleWidth = 1.0f; 

		auto sf = jmin(1.0f, 1.0f / UnblurryGraphics::getScaleFactorForComponent(this, false));

		rectangleWidth *= sf;

		rectangleWidth = jmax(1.0f, rectangleWidth);

		
		
		auto width = (float)getWidth() - 2.0f * offset;
		auto maxHeight = (float)getHeight() - 2.0f * offset;

		int samplesPerPixel = getSamplesPerPixel(rectangleWidth);
		rectangles.clear();
		int sampleIndex = 0;
		const auto& buffer = rb->getReadBuffer();

		p.clear();
		p.startNewSubPath(offset, offset + maxHeight);


		for (float i = 0; i <= width; i += rectangleWidth)
		{
			auto numThisTime = jmin(samplesPerPixel, buffer.getNumSamples() - sampleIndex);

			if (numThisTime <= 0)
				break;

			float maxValue = jlimit(0.0f, 1.0f, buffer.getMagnitude(0, sampleIndex, numThisTime));
			FloatSanitizers::sanitizeFloatNumber(maxValue);
			float height = maxValue * maxHeight;
			float y = offset + maxHeight - height;

			sampleIndex += samplesPerPixel;

			p.lineTo(i + offset, maxHeight - height + offset);

			rectangles.addWithoutMerging({ i + offset, y, rectangleWidth, height});
		}

		p.lineTo(width + offset, offset + maxHeight);

		repaint();
	}
}



void RingBufferComponentBase::LookAndFeelMethods::drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> areaToFill)
{
	g.setColour(ac.getColourForAnalyserBase(RingBufferComponentBase::bgColour));
	g.fillRect(areaToFill);
}

void RingBufferComponentBase::LookAndFeelMethods::drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p)
{
	g.setColour(ac.getColourForAnalyserBase(RingBufferComponentBase::fillColour));
	g.fillPath(p);
}

void RingBufferComponentBase::LookAndFeelMethods::drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index)
{
	auto c = ac.getColourForAnalyserBase(RingBufferComponentBase::fillColour);

	const float alphas[6] = { 1.0f, 0.5f, 0.3f, 0.2f, 0.1f, 0.05f };
	g.setColour(c.withAlpha(alphas[index]));
	g.fillRectList(dots);

}

void RingBufferComponentBase::LookAndFeelMethods::drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p)
{
	g.setColour(ac.getColourForAnalyserBase(RingBufferComponentBase::lineColour));
	g.strokePath(p, PathStrokeType(1.0f));
}



AhdsrGraph::AhdsrGraph()
{
	setSpecialLookAndFeel(new DefaultLookAndFeel(), true);
	setBufferedToImage(true);
	setColour(lineColour, Colours::lightgrey.withAlpha(0.3f));
}

AhdsrGraph::~AhdsrGraph()
{

}

void AhdsrGraph::paint(Graphics &g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	laf->drawAhdsrBackground(g, *this);
	laf->drawAhdsrPathSection(g, *this, envelopePath, false);



	float xPos = 0.0f;

	float tToUse = 1.0f;

	Path* pToUse = nullptr;

	auto s = (State)(int)ballPos;

	switch (s)
	{
	case State::ATTACK: pToUse = &attackPath; tToUse = attack; break;
	case State::HOLD: pToUse = &holdPath; tToUse = hold; break;
	case State::DECAY: pToUse = &decayPath; tToUse = 0.5f * decay; break;
	case State::SUSTAIN: pToUse = &releasePath;

		break;
	case State::RELEASE: pToUse = &releasePath; tToUse = 0.8f * release; break;
	default:
		break;
	}

	if (pToUse != nullptr)
	{
		laf->drawAhdsrPathSection(g, *this, *pToUse, true);

		auto bounds = pToUse->getBounds();
		auto normalizedDuration = 0.0f;

		normalizedDuration = fmod(ballPos, 1.0);

		xPos = bounds.getX() + normalizedDuration * bounds.getWidth();

		const float margin = 3.0f;

		auto l = Line<float>(xPos, 0.0f, xPos, (float)getHeight() - 1.0f - margin);

		auto clippedLine = envelopePath.getClippedLine(l, false);

		if (clippedLine.getLength() == 0.0f)
			return;

		auto pos = clippedLine.getStart();

		laf->drawAhdsrBallPosition(g, *this, pos);


	}
}

void AhdsrGraph::setUseFlatDesign(bool shouldUseFlatDesign)
{
	flatDesign = shouldUseFlatDesign;
	repaint();
}

void AhdsrGraph::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	if (e == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		ballPos = (float)newValue;
		repaint();
	}
	else
	{
		refresh();
	}
}

void AhdsrGraph::refresh()
{
	const auto& b = rb->getReadBuffer();
	
	if (b.getNumSamples() != 9)
		return;

	float this_attack = b.getSample(0, (int)Parameters::Attack);
	float this_attackLevel = b.getSample(0, (int)Parameters::AttackLevel);
	float this_hold = b.getSample(0, (int)Parameters::Hold);
	float this_decay = b.getSample(0, (int)Parameters::Decay);
	float this_sustain = b.getSample(0, (int)Parameters::Sustain);
	float this_release = b.getSample(0, (int)Parameters::Release);
	float this_attackCurve = b.getSample(0, (int)Parameters::AttackCurve);

	//lastState = dynamic_cast<AhdsrEnvelope*>(processor.get())->getStateInfo();

	if (this_attack != attack ||
		this_attackCurve != attackCurve ||
		this_attackLevel != attackLevel ||
		this_decay != decay ||
		this_sustain != sustain ||
		this_hold != hold ||
		this_release != release)
	{
		attack = this_attack;
		attackLevel = this_attackLevel;
		hold = this_hold;
		decay = this_decay;
		sustain = this_sustain;
		release = this_release;
		attackCurve = this_attackCurve;

		rebuildGraph();
	}

	repaint();
}





void AhdsrGraph::rebuildGraph()
{
	if (getLocalBounds().isEmpty())
		return;

	float aln = pow((1.0f - (attackLevel + 100.0f) / 100.0f), 0.4f);
	const float sn = pow((1.0f - (sustain + 100.0f) / 100.0f), 0.4f);

	const float margin = 3.0f;

	aln = sn < aln ? sn : aln;

	const float width = (float)getWidth() - 2.0f*margin;
	const float height = (float)getHeight() - 2.0f*margin;

	const float an = pow((attack / 20000.0f), 0.2f) * (0.2f * width);
	const float hn = pow((hold / 20000.0f), 0.2f) * (0.2f * width);
	const float dn = pow((decay / 20000.0f), 0.2f) * (0.2f * width);
	const float rn = pow((release / 20000.0f), 0.2f) * (0.2f * width);

	float x = margin;
	float lastX = x;

	envelopePath.clear();

	attackPath.clear();
	decayPath.clear();
	holdPath.clear();
	releasePath.clear();

	envelopePath.startNewSubPath(x, margin + height);
	attackPath.startNewSubPath(x, margin + height);

	// Attack Curve

	lastX = x;
	x += an;

	const float controlY = margin + aln * height + attackCurve * (height - aln * height);

	envelopePath.quadraticTo((lastX + x) / 2, controlY, x, margin + aln * height);

	attackPath.quadraticTo((lastX + x) / 2, controlY, x, margin + aln * height);
	attackPath.lineTo(x, margin + height);
	attackPath.closeSubPath();

	holdPath.startNewSubPath(x, margin + height);
	holdPath.lineTo(x, margin + aln * height);

	x += hn;

	envelopePath.lineTo(x, margin + aln * height);
	holdPath.lineTo(x, margin + aln * height);
	holdPath.lineTo(x, margin + height);
	holdPath.closeSubPath();

	decayPath.startNewSubPath(x, margin + height);
	decayPath.lineTo(x, margin + aln * height);

	lastX = x;
	x = jmin<float>(x + (dn * 4), 0.8f * width);

	envelopePath.quadraticTo(lastX, margin + sn * height, x, margin + sn * height);
	decayPath.quadraticTo(lastX, margin + sn * height, x, margin + sn * height);

	x = 0.8f * width;

	envelopePath.lineTo(x, margin + sn * height);
	decayPath.lineTo(x, margin + sn * height);

	decayPath.lineTo(x, margin + height);
	decayPath.closeSubPath();

	releasePath.startNewSubPath(x, margin + height);
	releasePath.lineTo(x, margin + sn * height);

	lastX = x;
	x += rn;

	envelopePath.quadraticTo(lastX, margin + height, x, margin + height);
	releasePath.quadraticTo(lastX, margin + height, x, margin + height);

	releasePath.closeSubPath();
	//envelopePath.closeSubPath();
}

void AhdsrGraph::LookAndFeelMethods::drawAhdsrBackground(Graphics& g, AhdsrGraph& graph)
{
	g.setColour(graph.getColourForAnalyserBase(RingBufferComponentBase::bgColour));
	g.fillRect(graph.getLocalBounds());
}

void AhdsrGraph::LookAndFeelMethods::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
{
	if (isActive)
	{
		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillPath(s);
	}
	else
	{
		if (graph.flatDesign)
		{
			g.setColour(graph.findColour(bgColour));
			g.fillAll();
			g.setColour(graph.findColour(fillColour));
			g.fillPath(s);
			g.setColour(graph.findColour(lineColour));
			g.strokePath(s, PathStrokeType(1.0f));
			g.setColour(graph.findColour(outlineColour));
			g.drawRect(graph.getLocalBounds(), 1);
		}
		else
		{
			GlobalHiseLookAndFeel::fillPathHiStyle(g, s, graph.getWidth(), graph.getHeight());
			g.setColour(graph.findColour(lineColour));
			g.strokePath(s, PathStrokeType(1.0f));
			g.setColour(Colours::lightgrey.withAlpha(0.1f));
			g.drawRect(graph.getLocalBounds(), 1);
		}
	}
}

void AhdsrGraph::LookAndFeelMethods::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p)
{
	auto circle = Rectangle<float>(p, p).withSizeKeepingCentre(6.0f, 6.0f);
	g.setColour(graph.findColour(lineColour).withAlpha(1.0f));
	g.fillRoundedRectangle(circle, 2.0f);
}

SimpleRingBuffer::WriterBase::~WriterBase()
{}

SimpleRingBuffer::PropertyObject::PropertyObject(WriterBase* b):
	writerBase(b)
{
	// This object must not be created from the DLL space...
	JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;
}

int SimpleRingBuffer::PropertyObject::getClassIndex() const
{ return 0; }

SimpleRingBuffer::PropertyObject::~PropertyObject()
{}

bool SimpleRingBuffer::PropertyObject::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
		return withinRange<512, 65536*2>(v);

	if (id == RingBufferIds::NumChannels)
		return withinRange<1, 2>(v);

	return false;
}

bool SimpleRingBuffer::PropertyObject::allowModDragger() const
{ return false; }

void SimpleRingBuffer::PropertyObject::initialiseRingBuffer(SimpleRingBuffer* b)
{
	buffer = b;
}

var SimpleRingBuffer::PropertyObject::getProperty(const Identifier& id) const
{
	jassert(std::any_of(properties.begin(), properties.end(), [id](const std::pair<String, var>& v)
	{
		return v.first == id.toString();
	}));
	
	if (buffer != nullptr)
	{
		if (id.toString() == "BufferLength")
			return var(buffer->internalBuffer.getNumSamples());

		if (id.toString() == "NumChannels")
			return var(buffer->internalBuffer.getNumChannels());
	}

	return getPropertyInternal(id.toString());
}

void SimpleRingBuffer::PropertyObject::setProperty(const Identifier& id, const var& newValue)
{
	setPropertyInternal(id.toString(), newValue);

	if (buffer != nullptr)
	{
		if ((id.toString() == "BufferLength") && (int)newValue > 0)
			buffer->setRingBufferSize(buffer->internalBuffer.getNumChannels(), (int)newValue);

		if ((id.toString() == "NumChannels") && (int)newValue > 0)
			buffer->setRingBufferSize((int)newValue, buffer->internalBuffer.getNumSamples());
	}
}

void SimpleRingBuffer::PropertyObject::transformReadBuffer(AudioSampleBuffer& b)
{
}

Array<Identifier> SimpleRingBuffer::PropertyObject::getPropertyList() const
{
	Array<Identifier> ids;

	for(int i = 0; i < properties.size(); i++)
		ids.add(properties[i].first);
	
	return ids;
}

bool SimpleRingBuffer::fromBase64String(const String& b64)
{
	return true;
}

void SimpleRingBuffer::setRingBufferSize(int numChannels, int numSamples, bool acquireLock)
{
	validateLength(numSamples);
	validateChannels(numChannels);

	if (numChannels != internalBuffer.getNumChannels() ||
		numSamples != internalBuffer.getNumSamples())
	{
		jassert(!isBeingWritten);

		SimpleReadWriteLock::ScopedWriteLock sl(getDataLock(), acquireLock);
		internalBuffer.setSize(numChannels, numSamples);
		internalBuffer.clear();
		numAvailable = 0;
		writeIndex = 0;
		updateCounter = 0;

		setupReadBuffer(externalBuffer);

		if (!currentlyChanged)
		{
			ScopedValueSetter<bool> svs(currentlyChanged, true);
			getUpdater().sendContentRedirectMessage();
		}
	}
}

String SimpleRingBuffer::toBase64String() const
{ return {}; }

void SimpleRingBuffer::setActive(bool shouldBeActive)
{
	active = shouldBeActive;
}

bool SimpleRingBuffer::isActive() const noexcept
{
	return active;
}

var SimpleRingBuffer::getReadBufferAsVar()
{ return externalBufferData; }

const AudioSampleBuffer& SimpleRingBuffer::getReadBuffer() const
{ return externalBuffer; }

AudioSampleBuffer& SimpleRingBuffer::getWriteBuffer()
{ return internalBuffer; }

void SimpleRingBuffer::setSamplerate(double newSampleRate)
{
	sr = newSampleRate;
}

double SimpleRingBuffer::getSamplerate() const
{ return sr; }

bool SimpleRingBuffer::isConnectedToWriter(WriterBase* b) const
{ return currentWriter == b; }

SimpleRingBuffer::PropertyObject::Ptr SimpleRingBuffer::getPropertyObject() const
{ return properties; }

SimpleRingBuffer::WriterBase* SimpleRingBuffer::getCurrentWriter() const
{ return currentWriter.get(); }

void SimpleRingBuffer::setCurrentWriter(WriterBase* newWriter)
{
	currentWriter = newWriter;
}

SimpleRingBuffer::ScopedPropertyCreator::ScopedPropertyCreator(ComplexDataUIBase* obj):
	p(dynamic_cast<SimpleRingBuffer*>(obj))
{
	JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

}

SimpleRingBuffer::ScopedPropertyCreator::~ScopedPropertyCreator()
{
	JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;
			
	if(p != nullptr)
		p->refreshPropertyObject();
}

CriticalSection& SimpleRingBuffer::getReadBufferLock()
{ return readBufferLock; }

int SimpleRingBuffer::getMaxLengthInSamples() const
{
	if(maxLength != -1.0) 
		return jmin(internalBuffer.getNumSamples(), roundToInt(maxLength));
	else 
		return internalBuffer.getNumSamples();
}

void SimpleRingBuffer::refreshPropertyObject()
{
	auto pIndex = properties != nullptr ? properties->getClassIndex() : 0;

	if (currentPropertyIndex != pIndex)
	{
		JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

		auto p = createPropertyObject(currentPropertyIndex, currentWriter.get());

		if (p == nullptr)
			p = new PropertyObject(currentWriter.get());

		setPropertyObject(p);
	}
}

hise::RingBufferComponentBase* SimpleRingBuffer::PropertyObject::createComponent()
{
	return new ModPlotter();
}

juce::Path SimpleRingBuffer::PropertyObject::createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const
{
	if (buffer == nullptr)
	{
		jassertfalse;
		return {};
	}

	int numValues = sampleRange.getLength();
	int numPixels = targetBounds.getWidth();
	auto stride = jmax(1, roundToInt((float)numValues / (float)numPixels));
	
	auto& b = buffer->getReadBuffer();

	if (b.getNumChannels() == 0 || b.getNumSamples() == 0)
		return {};

	//auto startValue = b.getSample(0, 0);

	auto startv = valueRange.getEnd() - (double)startValue * valueRange.getLength();

	Path p;

	p.preallocateSpace(numValues / stride);

	p.startNewSubPath(0.0f, valueRange.getStart());
	p.startNewSubPath(0.0f, valueRange.getEnd());
	p.startNewSubPath(0.0f, startv);

	for (int i = sampleRange.getStart(); i < numValues; i += stride)
	{
		int numToLook = jmin(stride, numValues - i);

		auto avg = FloatVectorOperations::findMinAndMax(b.getReadPointer(0, i), numToLook);

		auto value = avg.getEnd();

		if (std::abs(avg.getStart()) > std::abs(avg.getEnd()))
			value = avg.getStart();

        FloatSanitizers::sanitizeFloatNumber(value);
        
		p.lineTo((float)i, valueRange.getEnd() - valueRange.clipValue(value));
	};

	p.lineTo(numValues, startv);

    if(!p.getBounds().isEmpty())
        p.scaleToFit(targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), targetBounds.getHeight(), false);

	return p;
}



void FFTDisplayBase::drawSpectrum(Graphics& g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	auto targetBounds = dynamic_cast<Component*>(this)->getLocalBounds().toFloat();
	laf->drawOscilloscopeBackground(g, *this, targetBounds);

	if (rb != nullptr)
	{
		auto lPath = rb->getPropertyObject()->createPath({}, {}, targetBounds, 0.0);

		Path grid;

		auto x0 = FFTHelpers::getPixelValueForLogXAxis(100.0f, targetBounds.getWidth());
		auto x1 = FFTHelpers::getPixelValueForLogXAxis(1000.0f, targetBounds.getWidth());
		auto x2 = FFTHelpers::getPixelValueForLogXAxis(10000.0f, targetBounds.getWidth());

		grid.startNewSubPath(0.0f, 0.0f);
		grid.startNewSubPath(targetBounds.getWidth(), 1.0f);
		grid.startNewSubPath(x0, 0.0f);
		grid.lineTo(x0, 1.0f);
		grid.startNewSubPath(x1, 0.0f);
		grid.lineTo(x1, 1.0f);
		grid.startNewSubPath(x2, 0.0f);
		grid.lineTo(x2, 1.0f);
		grid.scaleToFit(targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), targetBounds.getHeight(), false);

		laf->drawAnalyserGrid(g, *this, grid);
		laf->drawOscilloscopePath(g, *this, lPath);
	}
}

void OscilloscopeBase::drawWaveform(Graphics& g)
{
	if (rb != nullptr)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(rb->getDataLock()))
		{
			auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

			auto b = dynamic_cast<Component*>(this)->getLocalBounds().toFloat();

			laf->drawOscilloscopeBackground(g, *this, b);

			Path grid;



			auto top = b.removeFromTop(b.getHeight() / 2.0f).reduced(2.0f);
			auto bottom = b.reduced(2.0f);

			grid.startNewSubPath(top.getX(), top.getCentreY());
			grid.lineTo(top.getRight(), top.getCentreY());
			grid.startNewSubPath(bottom.getX(), bottom.getCentreY());
			grid.lineTo(bottom.getRight(), bottom.getCentreY());

			grid.startNewSubPath(bottom.getX(), bottom.getY() - 2.0f);
			grid.lineTo(bottom.getRight(), bottom.getY() - 2.0f);


			laf->drawAnalyserGrid(g, *this, grid);

			drawOscilloscope(g, rb->getReadBuffer());

			

		}
	}
}

void OscilloscopeBase::drawPath(const float* l_, int numSamples, int width, Path& p)
{
	int stride = roundToInt((float)numSamples / width);
	stride = jmax<int>(1, stride * 2);

	if (numSamples != 0)
	{
		p.clear();

		p.startNewSubPath(0.0f, 1.0f);
		p.startNewSubPath(0.0f, -1.0f);
		p.startNewSubPath(0.0f, 0.0f);

		float x = 0.0f;

		bool mirrorMode = stride > 100;

		for (int i = 0; i < numSamples; i += stride)
		{
			const int numToCheck = jmin<int>(stride, numSamples - i);

			auto value = FloatVectorOperations::findMaximum(l_ + i, numToCheck);

			if (mirrorMode)
				value = jmax(0.0f, value);

			x = (float)i;
			p.lineTo(x, -1.0f * value);
		};

		if (mirrorMode)
		{
			for (int i = numSamples - 1; i >= 0; i -= stride)
			{
				const int numToCheck = jmin<int>(stride, numSamples - i);

				auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));

				x = (float)i;

				p.lineTo(x, -1.0f * value);
			};

			p.lineTo(x, 0.0f);
		}
		else
		{
			p.lineTo(x, 0.0f);
		}
	}
	else
	{
		p.clear();
	}
}

void OscilloscopeBase::drawOscilloscope(Graphics &g, const AudioSampleBuffer &b)
{
	auto asComponent = dynamic_cast<Component*>(this);
	auto lb = asComponent->getLocalBounds().toFloat();
	auto path = rb->getPropertyObject()->createPath({ 0, b.getNumSamples() }, { -1.0f, 1.0f }, lb, 0.0);

	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	jassert(laf != nullptr);

	laf->drawOscilloscopePath(g, *this, path);

#if 0
	auto dataL = b.getReadPointer(0);
	auto dataR = b.getReadPointer(jmin(1, b.getNumChannels() -1));
	
	auto asComponent = dynamic_cast<Component*>(this);

	drawPath(dataL, b.getNumSamples(), asComponent->getWidth(), lPath);
	drawPath(dataR, b.getNumSamples(), asComponent->getWidth(), rPath);

	auto lb = asComponent->getLocalBounds().toFloat();
	auto top = lb.removeFromTop(lb.getHeight() / 2.0f).reduced(2.0f);
	auto bottom = lb.reduced(2.0f);

	lPath.scaleToFit(top.getX(), top.getY(), top.getWidth(), top.getHeight(), false);
	rPath.scaleToFit(bottom.getX(), bottom.getY(), bottom.getWidth(), bottom.getHeight(), false);

	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	jassert(laf != nullptr);

	laf->drawOscilloscopePath(g, *this, lPath);
	laf->drawOscilloscopePath(g, *this, rPath);
#endif
}

GoniometerBase::Shape::Shape(const AudioSampleBuffer& buffer, Rectangle<int> area)
{
	const int stepSize = buffer.getNumSamples() / 128;

	for (int i = 0; i < 128; i++)
	{
		auto p = createPointFromSample(buffer.getSample(0, i * stepSize), buffer.getSample(1, i*stepSize), (float)area.getWidth());

		points.addWithoutMerging({ p.x + area.getX(), p.y + area.getY(), 2.0f, 2.0f });
	}
}


juce::Point<float> GoniometerBase::Shape::createPointFromSample(float left, float right, float size)
{
	float lScaled = sqrt(fabsf(left));
	float rScaled = sqrt(fabsf(right));

	if (left < 0.0f)
		lScaled *= -1.0f;

	if (right < 0.0f)
		rScaled *= -1.0f;

	float lValue = lScaled / -2.0f + 0.5f;
	float rValue = rScaled / 2.0f + 0.5f;

	float x = ((lValue + rValue) / 2.0f * size);
	float y = ((lValue + 1.0f - rValue) / 2.0f * size);

	return { x, y };
}

void GoniometerBase::Shape::draw(Graphics& g, Colour c)
{
	g.setColour(c);
	g.fillRectList(points);
}

void GoniometerBase::paintSpacialDots(Graphics& g)
{
	if (rb != nullptr)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(rb->getDataLock()))
		{
			auto asComponent = dynamic_cast<Component*>(this);

			auto size = jmin<int>(asComponent->getWidth(), asComponent->getHeight());

			Rectangle<int> area = { (asComponent->getWidth() - size) / 2, (asComponent->getHeight() - size) / 2, size, size };

			auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

			Array<Line<float>> lines;

			lines.add({ (float)area.getX(), (float)area.getY(), (float)area.getRight(), (float)area.getBottom() });
			lines.add({ (float)area.getX(), (float)area.getBottom(), (float)area.getRight(), (float)area.getY() });

			Path grid;

			for (auto l : lines)
			{
				grid.startNewSubPath(l.getStart());
				grid.lineTo(l.getEnd());
			}

			laf->drawAnalyserGrid(g, *this, grid);

			shapeIndex = (shapeIndex + 1) % 6;
			shapes[shapeIndex] = Shape(rb->getReadBuffer(), area);

			for (int i = 0; i < 6; i++)
			{
				auto& p = shapes[(shapeIndex + i) % 6].points;
				laf->drawGonioMeterDots(g, *this, p, i);
			}

		}
	}

}

} // namespace hise
