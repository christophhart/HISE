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

hise::RingBufferComponentBase* SimpleRingBuffer::PropertyObject::createComponent()
{
	return new ModPlotter();
}

void FFTDisplayBase::Properties::applyFFT(SimpleRingBuffer::Ptr p)
{

}

void FFTDisplayBase::drawSpectrum(Graphics& g)
{
	auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();

	laf->drawOscilloscopeBackground(g, *this, dynamic_cast<Component*>(this)->getLocalBounds().toFloat());

	if (rb != nullptr)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(rb->getDataLock()))
		{
			const auto& b = rb->getReadBuffer();

			int size = b.getNumSamples();

            auto order = log2(size);
            
            auto fft = juce::dsp::FFT(order);
            
			if (windowBuffer.getNumSamples() == 0 || windowBuffer.getNumSamples() != size)
			{
				windowBuffer = AudioSampleBuffer(1, size);
				fftBuffer = AudioSampleBuffer(1, size);
				fftBuffer.clear();
			}

			if (lastWindowType != fftProperties.window)
			{
				auto d = windowBuffer.getWritePointer(0);

                juce::dsp::WindowingFunction<float> w(size, juce::dsp::WindowingFunction<float>::blackmanHarris);

                
                FloatVectorOperations::fill(d, 1.0f, size);
                w.multiplyWithWindowingTable(d, size);
                
				lastWindowType = fftProperties.window;
			}

			AudioSampleBuffer b2(2, size * 2);
            b2.clear();
            
            auto data = b2.getWritePointer(0);
            FloatVectorOperations::copy(data, rb->getReadBuffer().getReadPointer(0), size);
            
			

			FloatVectorOperations::multiply(data, windowBuffer.getReadPointer(0), size);

			auto lastValues = fftBuffer.getWritePointer(0);

			auto sampleRate = getSamplerate();

            fft.performRealOnlyForwardTransform(data);
            
			if (fftProperties.domain == Amplitude)
			{
				for (int i = 2; i < size; i += 2)
				{
					data[i] = sqrt(data[i] * data[i] + data[i + 1] * data[i + 1]);
					data[i + 1] = data[i];
				}
			}
			else
			{
				auto threshhold = FloatVectorOperations::findMaximum(data, size) / 10000.0;

				data[0] = 0.0f;
				data[1] = 0.0f;

				for (int i = 2; i < size; i += 2)
				{
					auto real = data[i];
					auto img = data[i + 1];

					if (real < threshhold) real = 0.0f;
					if (img < threshhold) img = 0.0f;

					auto phase = atan2f(img, real);

					data[i] = phase;
					data[i + 1] = phase;
				}
			}

			FloatVectorOperations::abs(data, b2.getReadPointer(0), size);
			FloatVectorOperations::multiply(data, 1.0f / 95.0f, size);

			auto asComponent = dynamic_cast<Component*>(this);

			int stride = roundToInt((float)size / asComponent->getWidth());
			stride *= 2;

			lPath.clear();

			lPath.startNewSubPath(0.0f, (float)asComponent->getHeight());

			if (sampleRate == 0.0)
				sampleRate = 44100.0;

			int log10Offset = (int)(10.0 / (sampleRate * 0.5) * (double)size + 1.0);

			float lastIndex = 0.0f;
			float value = 0.0f;
			int lastI = 0;
			int sumAmount = 0;

			int lastLineLog = 1;

			Path grid;

			for (int i = log10Offset; i < size; i += 2)
			{
				auto f = (double)i / (double)size * sampleRate / 2.0;

				auto thisLineLog = (int)log10(f);

				if (thisLineLog == 0)
					continue;

				float xPos;

				if (fftProperties.freq2x)
					xPos = (float)fftProperties.freq2x((float)f);
				else
                {
                    auto width = asComponent->getWidth();
                    auto lowFreq = 20;
                    auto highFreq = 20000.0;
                    float freq = f;
                    
                    xPos = (width - 5) * (log (freq / lowFreq) / log (highFreq / lowFreq)) + 2.5f;
                    //xPos = (float)log10((float)i) / maxPos * (float)(asComponent->getWidth() + xLog10Pos) - xLog10Pos;
                }
					

				auto diff = xPos - lastIndex;

				if ((int)thisLineLog != lastLineLog)
				{
                    FloatSanitizers::sanitizeFloatNumber(xPos);
					grid.startNewSubPath(xPos, 0.0f);
					grid.lineTo(xPos, asComponent->getHeight());

					lastLineLog = (int)thisLineLog;
				}

				auto indexDiff = i - lastI;

				float v = fabsf(data[i]);

				if (fftProperties.gain2y)
					v = fftProperties.gain2y(v);
				else
				{



					v = Decibels::gainToDecibels(v);
					v = jlimit<float>(fftProperties.dbRange.getStart(), 0.0f, v);
					v = 1.0f + v / fftProperties.dbRange.getLength();
					v = powf(v, 0.707f);
				}

				value += v;
				sumAmount++;

				if (diff > 1.0f && indexDiff > 4)
				{
					value /= (float)(sumAmount);

					sumAmount = 0;

					lastIndex = xPos;
					lastI = i;

					value = 0.6f * value + 0.4f * lastValues[i];

					if (value > lastValues[i])
						lastValues[i] = value;
					else
						lastValues[i] = jmax<float>(0.0f, lastValues[i] - 0.05f);

					auto yPos = lastValues[i];
					yPos = 1.0f - yPos;

					yPos *= asComponent->getHeight();

					lPath.lineTo(xPos, yPos);

					value = 0.0f;
				}
			}

			lPath.lineTo((float)asComponent->getWidth(), (float)asComponent->getHeight());
			lPath.closeSubPath();

			laf->drawAnalyserGrid(g, *this, grid);
			laf->drawOscilloscopePath(g, *this, lPath);
		}
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

		float x;

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
	auto dataL = b.getReadPointer(0);
	auto dataR = b.getReadPointer(1);
	
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
