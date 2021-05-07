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
			;

		bool shortBuffer = internalBuffer.getNumSamples() < 4096;

		int thisWriteIndex = shortBuffer ? 0 : writeIndex.load();
		int numBeforeIndex = thisWriteIndex;
		int offsetBeforeIndex = internalBuffer.getNumSamples() - numBeforeIndex;

		jassert(b.getNumSamples() == internalBuffer.getNumSamples() && b.getNumChannels() == internalBuffer.getNumChannels());

		int numSamples = b.getNumSamples();
		int numChannels = b.getNumChannels();

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
			jassert(numChannels == internalBuffer.getNumChannels());

			int numBeforeWrap = jmin(numSamples, internalBuffer.getNumSamples() - writeIndex);

			if (numBeforeWrap > 0)
			{
				for (int i = 0; i < numChannels; i++)
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

				for (int i = 0; i < numChannels; i++)
				{
					auto buffer = internalBuffer.getWritePointer(i);
					FloatVectorOperations::copy(buffer, data[i] + numBeforeWrap, numThisTime);
				}

				writeIndex = (writeIndex + numAfterWrap) % internalBuffer.getNumSamples();
			}

			numAvailable += numSamples;
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
	properties = newObject;

	properties->initialiseRingBuffer(this);

	auto ns = internalBuffer.getNumSamples();
	auto nc = internalBuffer.getNumChannels();

	if (validateChannels(nc) ||
		validateLength(ns))
		setRingBufferSize(nc, ns);

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

int ModPlotter::getSamplesPerPixel(float rectangleWidth) const
{
	float offset = 2.0f;

	auto width = (float)getWidth() - 2.0f * offset;

	int samplesPerPixel = SimpleRingBuffer::RingBufferSize / jmax((int)(width / rectangleWidth), 1);
	return samplesPerPixel;
}

void RingBufferComponentBase::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	refresh();
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
	getSpecialLookAndFeel<RingBufferComponentBase::LookAndFeelMethods>()->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());

	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

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
		auto duration = 10.0;// (float)(processor->getMainController()->getUptime() - lastState.changeTime) * 1000.0f;

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
	jassert(b.getNumSamples() == 9);

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



void AhdsrGraph::DefaultLookAndFeel::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
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

void AhdsrGraph::DefaultLookAndFeel::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p)
{
	auto circle = Rectangle<float>(p, p).withSizeKeepingCentre(6.0f, 6.0f);
	g.setColour(graph.findColour(lineColour).withAlpha(1.0f));
	g.fillRoundedRectangle(circle, 2.0f);
}

hise::RingBufferComponentBase* SimpleRingBuffer::PropertyObject::createComponent()
{
	return new ModPlotter();
}

} // namespace hise