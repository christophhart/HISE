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


namespace hise {
using namespace juce;

ProcessorEditorBody * AnalyserEffect::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new AnalyserEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif

}

void Goniometer::paint(Graphics& g)
{
	

	auto an = getAnalyser();

	ScopedReadLock sl(an->getBufferLock());

	auto size = jmin<int>(getWidth(), getHeight());

	Rectangle<int> area = { (getWidth() - size) / 2, (getHeight() - size) / 2, size, size };

	g.setColour(getColourForAnalyser(AudioAnalyserComponent::bgColour));
	g.fillRect(area);

	g.setColour(getColourForAnalyser(AudioAnalyserComponent::lineColour));

	g.drawLine((float)area.getX(), (float)area.getY(), (float)area.getRight(), (float)area.getBottom(), 1.0f);
	g.drawLine((float)area.getX(), (float)area.getBottom(), (float)area.getRight(), (float)area.getY(), 1.0f);

	auto buffer = an->getAnalyseBuffer();

	shapeIndex = (shapeIndex + 1) % 6;
	shapes[shapeIndex] = Shape(buffer, area);
	
	Colour c = getColourForAnalyser(AudioAnalyserComponent::fillColour);
	
    shapes[shapeIndex].draw(g, c.withAlpha(1.0f));
    shapes[(shapeIndex + 1) % 6].draw(g, c.withAlpha(0.5f));
	shapes[(shapeIndex + 2) % 6].draw(g, c.withAlpha(0.3f));
	shapes[(shapeIndex + 3) % 6].draw(g, c.withAlpha(0.2f));
	shapes[(shapeIndex + 4) % 6].draw(g, c.withAlpha(0.1f));
	shapes[(shapeIndex + 5) % 6].draw(g, c.withAlpha(0.05f));

	//drawOscilloscope(g, buffer);



}

Goniometer::Shape::Shape(const AudioSampleBuffer& buffer, Rectangle<int> area)
{
	const int stepSize = buffer.getNumSamples() / 128;

	for (int i = 0; i < 128; i++)
	{
		auto p = createPointFromSample(buffer.getSample(0, i * stepSize), buffer.getSample(1, i*stepSize), (float)area.getWidth());

		points.addWithoutMerging({ p.x + area.getX(), p.y + area.getY(), 2.0f, 2.0f });
	}
}



Point<float> Goniometer::Shape::createPointFromSample(float left, float right, float size)
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


void Goniometer::Shape::draw(Graphics& g, Colour c)
{
	g.setColour(c);

	g.fillRectList(points);

}

void Oscilloscope::paint(Graphics& g)
{
	auto an = getAnalyser();

	ScopedReadLock sl(an->getBufferLock());

	g.fillAll(getColourForAnalyser(AudioAnalyserComponent::bgColour));

	auto buffer = an->getAnalyseBuffer();

	g.setColour(getColourForAnalyser(AudioAnalyserComponent::fillColour));

	drawOscilloscope(g, buffer);

}

    
void drawPath(const float* l_, int numSamples, int width, Path& p)
    {
        int stride = roundFloatToInt((float)numSamples / width);
        stride = jmax<int>(1, stride * 2);
        
        if (numSamples != 0)
        {
            p.clear();
            p.startNewSubPath(0.0f, 0.0f);
			p.lineTo(0.0f, 1.0f);
			p.lineTo(0.0f, -1.0f);
            
            for (int i = stride; i < numSamples; i += stride)
            {
                const int numToCheck = jmin<int>(stride, numSamples - i);
                
                auto value = jmax<float>(0.0f, FloatVectorOperations::findMaximum(l_ + i, numToCheck));
                
                p.lineTo((float)i, -1.0f * value);
                
            };
            
            for (int i = numSamples - 1; i > 0; i -= stride)
            {
                const int numToCheck = jmin<int>(stride, numSamples - i);
                
                auto value = jmin<float>(0.0f, FloatVectorOperations::findMinimum(l_ + i, numToCheck));
                
                p.lineTo((float)i, -1.0f * value);
            };
            
            p.closeSubPath();
        }
        else
        {
            p.clear();
        }
    }

void Oscilloscope::drawOscilloscope(Graphics &g, const AudioSampleBuffer &b)
{
	AudioSampleBuffer b2(2, b.getNumSamples());

	auto dataL = b2.getWritePointer(0);
    auto dataR = b2.getWritePointer(1);
	int size = b.getNumSamples();

	auto readIndex = getAnalyser()->getCurrentReadIndex();

	int numBeforeWrap = size - readIndex;
	int numAfterWrap = size - numBeforeWrap;

	FloatVectorOperations::copy(dataL, b.getReadPointer(0, readIndex), numBeforeWrap);
	FloatVectorOperations::copy(dataL + numBeforeWrap, b.getReadPointer(0, 0), numAfterWrap);

    FloatVectorOperations::copy(dataR, b.getReadPointer(1, readIndex), numBeforeWrap);
    FloatVectorOperations::copy(dataR + numBeforeWrap, b.getReadPointer(1, 0), numAfterWrap);

    drawPath(dataL, b.getNumSamples(), getWidth(), lPath);
    drawPath(dataR, b.getNumSamples(), getWidth(), rPath);
    
	lPath.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)(getHeight()/2), false);
	rPath.scaleToFit(0.0f, (float)(getHeight()/2), (float)getWidth(), (float)(getHeight()/2), false);

	g.fillPath(lPath);
	g.fillPath(rPath);
}


Colour AudioAnalyserComponent::getColourForAnalyser(ColourId id)
{
	if (auto panel = findParentComponentOfClass<Panel>())
	{
		switch (id)
		{
		case hise::AudioAnalyserComponent::bgColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::bgColour);
		case hise::AudioAnalyserComponent::fillColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
		case hise::AudioAnalyserComponent::lineColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::itemColour2);
            default: break;
		}
	}
	else
	{
		switch (id)
		{
		case hise::AudioAnalyserComponent::bgColour:   return findColour(AudioAnalyserComponent::ColourId::bgColour);
		case hise::AudioAnalyserComponent::fillColour: return Colour(0xFF555555);
		case hise::AudioAnalyserComponent::lineColour: return Colour(0xFF555555);
            default: break;
		}
	}

	jassertfalse;
	return Colours::transparentBlack;
}

AnalyserEffect * AudioAnalyserComponent::getAnalyser() 
{
	return dynamic_cast<AnalyserEffect*>(processor.get());
}

const AnalyserEffect* AudioAnalyserComponent::getAnalyser() const
{
	return dynamic_cast<AnalyserEffect*>(processor.get());
}

void FFTDisplay::paint(Graphics& g)
{
	g.fillAll(getColourForAnalyser(AudioAnalyserComponent::bgColour));

#if USE_IPP
    
	auto an = getAnalyser();

	ScopedReadLock sl(an->getBufferLock());

	auto b = an->getAnalyseBuffer();

	int size = b.getNumSamples();

	

	if (windowBuffer.getNumSamples() == 0 || windowBuffer.getNumSamples() != size)
	{
		windowBuffer = AudioSampleBuffer(1, size);
		fftBuffer = AudioSampleBuffer(1, size);

		fftBuffer.clear();

		icstdsp::VectorFunctions::blackman(windowBuffer.getWritePointer(0), size);
	}

	AudioSampleBuffer b2(2, b.getNumSamples());

	auto data = b2.getWritePointer(0);
	auto lastValues = fftBuffer.getWritePointer(0);

	auto readIndex = an->getCurrentReadIndex();

	int numBeforeWrap = size - readIndex;
	int numAfterWrap = size - numBeforeWrap;

	FloatVectorOperations::copy(data, b.getReadPointer(0, readIndex), numBeforeWrap);
	FloatVectorOperations::copy(data + numBeforeWrap, b.getReadPointer(0, 0), numAfterWrap);

	FloatVectorOperations::add(data, b.getReadPointer(1, readIndex), numBeforeWrap);
	FloatVectorOperations::add(data + numBeforeWrap, b.getReadPointer(1, 0), numAfterWrap);

	FloatVectorOperations::multiply(data, 0.5f, size);
	FloatVectorOperations::multiply(data, windowBuffer.getReadPointer(0), size);
	
	auto sampleRate = getAnalyser()->getSampleRate();

	fftObject.realFFTInplace(data, size);

	for (int i = 2; i < size; i+= 2)
	{
		data[i] = sqrtf(data[i] * data[i] + data[i + 1] * data[i + 1]);
		data[i + 1] = data[i];
	}

	//fftObject.realFFT(b.getReadPointer(1), b2.getWritePointer(1), b2.getNumSamples());

	FloatVectorOperations::abs(data, b2.getReadPointer(0), size);
	FloatVectorOperations::multiply(data, 1.0f / 95.0f, size);
	
	int stride = roundFloatToInt((float)size / getWidth());
	stride *= 2;
	
	lPath.clear();

	lPath.startNewSubPath(0.0f, (float)getHeight());
	//lPath.lineTo(0.0f, -1.0f);
	

	int log10Offset = (int)(10.0 / (sampleRate * 0.5) * (double)size + 1.0);

	auto maxPos = log10f((float)(size));

	float lastIndex = 0.0f;
	float value = 0.0f;
	int lastI = 0;
	int sumAmount = 0;

	int lastLineLog = 1;
	
	g.setColour(getColourForAnalyser(AudioAnalyserComponent::lineColour));

	int xLog10Pos = roundToInt(log10((float)log10Offset) / maxPos * (float)getWidth());

	for (int i = log10Offset; i < size; i+= 2)
	{
		auto f = (double)i / (double)size * sampleRate/2.0;

		auto thisLineLog = (int)log10(f);

		if (thisLineLog == 0)
			continue;

		auto xPos = log10((float)i) / maxPos * (float)(getWidth()+xLog10Pos) - xLog10Pos;
		auto diff = xPos - lastIndex;

		if ((int)thisLineLog != lastLineLog)
		{
			g.drawVerticalLine((int)xPos, 0.0f, (float)getHeight());

			lastLineLog = (int)thisLineLog;
		}

		auto indexDiff = i - lastI;

		float v = fabsf(data[i]);

		v = Decibels::gainToDecibels(v);
		v = jlimit<float>(-70.0f, 0.0f, v);
		v = 1.0f + v / 70.0f;
		v = powf(v, 0.707f);

		value += v;
		sumAmount++;

		if(diff > 1.0f && indexDiff > 4)
		{
			value /= (float)(sumAmount);

			sumAmount = 0;

			lastIndex = xPos;
			lastI = i;

			value = 0.6f * value + 0.4f * lastValues[i];
	
			if (value > lastValues[i])
				lastValues[i] = value;
			else
				lastValues[i] = jmax<float>(0.0f, lastValues[i] - 0.02f);

			auto yPos = lastValues[i];
			yPos = 1.0f - yPos;

			yPos *= getHeight();

			lPath.lineTo(xPos, yPos);

			value = 0.0f;
		}
	}

	

	lPath.lineTo((float)getWidth(), (float)getHeight());
	
	lPath.closeSubPath();
	
	g.setColour(getColourForAnalyser(AudioAnalyserComponent::fillColour));
	g.fillPath(lPath);
    
#else
    
    g.setColour(getColourForAnalyser(AudioAnalyserComponent::fillColour));
    g.setFont(GLOBAL_BOLD_FONT());
    g.drawText("You need IPP for the FFT Analyser", 0.0f, 0.0f, (float)getWidth(), (float)getHeight(), Justification::centred, false);
    
#endif
}

Component* AudioAnalyserComponent::Panel::createContentComponent(int index)
{
	Component* c = nullptr;

	switch (index)
	{
	case 0: c = new Goniometer(getProcessor()); break;
	case 1: c = new Oscilloscope(getProcessor()); break;
	case 2:	c = new FFTDisplay(getProcessor()); break;
	}

	if (findPanelColour(FloatingTileContent::PanelColourId::bgColour).isOpaque())
		c->setOpaque(true);

	return c;
}

void AudioAnalyserComponent::Panel::fillIndexList(StringArray& indexList)
{
	indexList.add("Goniometer");
	indexList.add("Oscilloscope");
	indexList.add("Spectral Analyser");
}

}
