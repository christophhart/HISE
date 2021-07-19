/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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


namespace snex {
using namespace juce;



namespace ui
{

void OptimizationProperties::resetOptimisations()
{
	getGlobalScope().clearOptimizations();

	for (auto i : items)
	{
		if (i->active)
			getGlobalScope().addOptimization(i->id);
	}

	getWorkbench()->triggerRecompile();
}

void Graph::InternalGraph::paint(Graphics& g)
{
	if (lastBuffer.getNumSamples() == 0)
		return;

	g.fillAll(Colour(0xFF262626));

	g.setFont(GLOBAL_BOLD_FONT());

	if (l.isEmpty() && r.isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.5f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No signal to draw", getLocalBounds().toFloat(), Justification::centred);
	}
	else
	{
		if (getCurrentGraphType() == GraphType::Spectrograph)
		{
			g.drawImageWithin(spectroImage, 0, 0, getWidth(), getHeight(), RectanglePlacement::stretchToFit);
		}
		else
		{
			if (getCurrentGraphType() == GraphType::FFT)
			{
				float freq = 10.0f;
				auto sr = parent.getWorkbench()->getTestData().getPrepareSpecs().sampleRate;

				while (freq < sr)
				{
					auto index = getXPosition(freq / sr * 2.0f);
					freq *= 10.0;

					g.setColour(Colours::white.withAlpha(0.1f));
					g.drawVerticalLine(roundToInt(index * (float)getWidth()), 0.0f, (float)getHeight());
				}


				if (parent.logPeakButton->getToggleState())
				{
					for (int i = 0; i < 10; i++)
					{
						auto db = (float)i / 10.0f;
						auto l = db * (float)getHeight();
						g.drawHorizontalLine(l, 0.0f, (float)getWidth());
					}
				}
			}

			g.setColour(Colours::white.withAlpha(0.8f));

			if (isHiresMode())
				g.strokePath(l, PathStrokeType(2.0f));
			else
				g.fillPath(l);

			if (stereoMode && !r.isEmpty())
			{
				if (isHiresMode())
					g.strokePath(r, PathStrokeType(2.0f));
				else
					g.fillPath(r);
			} 
		}

		
	}

	if (parent.markerButton->getToggleState() && getCurrentGraphType() != GraphType::FFT)
	{
		auto& td = parent.getWorkbench()->getTestData();

		for (int i = 0; i < td.getNumTestEvents(true); i++)
			drawTestEvent(g, true, i);

		for (int i = 0; i < td.getNumTestEvents(false); i++)
			drawTestEvent(g, false, i);
	}

	if (currentPosition > 0 && numSamples > 0)
	{
		auto pos = getPixelForSample(currentPosition);

		g.setColour(Colours::red);
		g.drawVerticalLine((int)pos, 0.0f, (float)getHeight());
	}

	if (pixelsPerSample > 20.0f)
	{
		for (int i = 0; i < lastBuffer.getNumSamples(); i++)
		{
			auto x = getPixelForSample(i);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawVerticalLine(x, 0.0f, (float)getHeight());
		}
	}

	if (!currentPoint.isOrigin())
	{
		float posTotal = (float)currentPoint.getX() / (float)getWidth();
		posTotal = jlimit(0.0f, 1.0f, posTotal);
		int samplePos = jlimit(0, lastBuffer.getNumSamples() - 1, roundToInt(posTotal * (float)lastBuffer.getNumSamples()));

		int x_ = (float)getPixelForSample(samplePos);
		int y_ = (float)getYPixelForSample(samplePos);

		g.fillRect(x_ - 2.0f, y_ - 2.0f, 4.0f, 4.0f);

		g.setColour(Colours::white);
		g.drawVerticalLine(x_, 0.0f, (float)getHeight());

#if 0
		if (!isTimerRunning())
		{
			bool rightChannel = stereoMode && currentPoint.getY() > (getHeight() / 2);

			float value = lastBuffer.getSample(rightChannel ? 1 : 0, samplePos);

			juce::String v;
			v << "data[" << juce::String(samplePos) << "]: " << Types::Helpers::getCppValueString(value);

			auto f = GLOBAL_MONOSPACE_FONT();

			g.setFont(f);

			juce::Rectangle<float> r(0.0f, 0.0f, f.getStringWidthFloat(v) + 10.0f, f.getHeight() + 5.0f);

			auto x = ((float)currentPoint.getX() - r.getWidth()*0.5f);
			x = jlimit(0.0f, (float)getWidth() - r.getWidth(), x);
			r.setX(x);

			auto y = (float)currentPoint.getY() - 20.0f;

			y = jlimit(0.0f, (float)getHeight() - 20.0f, y);

			r.setY(y);

			g.setColour(Colour(0xEEAAAAAA));
			g.fillRoundedRectangle(r, 2.0f);
			g.setColour(Colours::black.withAlpha(0.9f));
			g.drawRoundedRectangle(r, 2.0f, 1.0f);
			g.drawText(v, r, Justification::centred);
		}
#endif


	}
}

void Graph::InternalGraph::drawTestEvent(Graphics& g, bool isParameter, int index)
{
	int timestamp = 0;
	String t;
	Colour c;

	auto& td = parent.getWorkbench()->getTestData();

	if (td.testSourceData.getNumSamples() == 0)
		return;

	if (isParameter)
	{
		auto p = td.getParameterEvent(index);
		timestamp = p.timeStamp;
		t << "P" << String(p.parameterIndex) << ": " << Types::Helpers::getCppValueString(p.valueToUse);
		c = Types::Helpers::getColourForType(Types::ID::Double);
	}
	else
	{
		auto e = td.getTestHiseEvent(index);
		timestamp = e.getTimeStamp();
		t << "E" << String(index) << ": " << e.getTypeAsString();
		c = Types::Helpers::getColourForType(Types::ID::Integer);
	}

	auto ni = (float)timestamp / (float)td.testSourceData.getNumSamples();

	auto xPos = roundToInt(ni * getWidth());

	g.setColour(c.withAlpha(0.8f));
	g.drawVerticalLine(xPos,0.0f, (float)getHeight());

	auto f = GLOBAL_MONOSPACE_FONT();
	g.setFont(f);
	juce::Rectangle<int> ar(xPos, 0, roundToInt(f.getStringWidthFloat(t) + 3.0f), 20);

	g.fillRect(ar);
	g.setColour(Colours::black);
	g.drawText(t, ar.toFloat(), Justification::centred);

}

void Graph::InternalGraph::setBuffer(const AudioSampleBuffer& b)
{
	if (b.getNumSamples() == 0)
		return;

	if (lastBuffer.getNumChannels() == 0 || lastBuffer.getWritePointer(0) != b.getReadPointer(0))
	{
		lastBuffer = AudioSampleBuffer(b.getNumChannels(), b.getNumSamples());

		for (int i = 0; i < lastBuffer.getNumChannels(); i++)
			FloatVectorOperations::copy(lastBuffer.getWritePointer(i), b.getReadPointer(i), b.getNumSamples());
	}

	if (getCurrentGraphType() == GraphType::Spectrograph)
	{
		rebuildSpectrumRectangles();
	}
	else
	{
		calculatePath(l, b, 0);

		stereoMode = b.getNumChannels() == 2;

		if (stereoMode)
			calculatePath(r, b, 1);
		else
			r.clear();

		leftPeaks = b.findMinMax(0, 0, b.getNumSamples());

		if (stereoMode)
			rightPeaks = b.findMinMax(1, 0, b.getNumSamples());

		resizePath();
	}
}

Graph::GraphType Graph::InternalGraph::getCurrentGraphType() const
{
	return findParentComponentOfClass<Graph>()->currentGraphType;
}

void Graph::InternalGraph::calculatePath(Path& p, const AudioSampleBuffer& b, int channel)
{
	numSamples = b.getNumSamples();
	p.clear();

	if (numSamples == 0)
		return;

	if (b.getMagnitude(channel, 0, b.getNumSamples()) > 0.0f)
	{
		auto delta = (float)b.getNumSamples() / jmax(1.0f, (float)getWidth());

		int samplesPerPixel = jmax(1, (int)delta);

		pixelsPerSample = 1.0f / (float)delta;

		p.startNewSubPath(0.0f, getYPosition(0.0f));

		for (int i = 0; i < b.getNumSamples(); i += samplesPerPixel)
		{
			int numToDo = jmin(samplesPerPixel, b.getNumSamples() - i);
			auto range = FloatVectorOperations::findMinAndMax(b.getReadPointer(channel, i), numToDo);


			float s;

			if (range.getEnd() > (-1.0f * range.getStart()))
				s = range.getEnd();
			else
				s = range.getStart();

			auto x = getXPosition((float)i / b.getNumSamples());

			p.lineTo(x, getYPosition(s));
		}

		if (!isHiresMode())
		{
			p.lineTo(1.0f, getYPosition(0.0f));
			p.closeSubPath();
		}
	}
}

void Graph::InternalGraph::resizePath()
{
	auto pb = getLocalBounds();

	if (pb.isEmpty())
		return;

	pb.reduce(2, 2);

	if (stereoMode)
	{
		auto lb = pb.removeFromTop(getHeight() / 2).toFloat();

		auto rb = pb.toFloat();

		lb.reduce(0.0f, 1.0f);
		rb.reduce(0.0f, 1.f);

		l.scaleToFit(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), false);
		r.scaleToFit(rb.getX(), rb.getY(), rb.getWidth(), rb.getHeight(), false);
	}
	else
	{
		auto lb = pb.toFloat();
		l.scaleToFit(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), false);
	}

	repaint();
}

String Graph::InternalGraph::getTooltip()
{
	if (currentPoint.isOrigin())
		return {};

	if (lastBuffer.getNumSamples() == 0)
		return {};

	juce::String v;

	auto scaleFreq = parent.logScaleButton->getToggleState();
	auto scalePeak = parent.logPeakButton->getToggleState();

	float xNormalised = (float)currentPoint.getX() / (float)getWidth();
	xNormalised = jlimit(0.0f, 1.0f, xNormalised);
	auto sr = parent.getWorkbench()->getTestData().getPrepareSpecs().sampleRate;

	float yNormalised = (float)currentPoint.getY() / (float)getHeight();
	yNormalised = 1.0f - jlimit(0.0f, 1.0f, yNormalised);

	switch (getCurrentGraphType())
	{
	case GraphType::Signal:
	{
		int samplePos = jlimit(0, lastBuffer.getNumSamples() - 1, roundToInt(xNormalised * (float)lastBuffer.getNumSamples()));

		bool rightChannel = stereoMode && currentPoint.getY() > (getHeight() / 2);
		
		v << "data[" << juce::String(samplePos) << "]: ";
		float value = lastBuffer.getSample(rightChannel ? 1 : 0, samplePos);
		v << Types::Helpers::getCppValueString(value);

		break;
	}
	case GraphType::FFT:
	{
		float hz;

		if (scaleFreq)
		{
			hz = std::exp(std::log(xNormalised) / 0.2f);
			hz *= sr * 0.5;
		}
		else
		{
			hz = xNormalised * sr * 0.5;
		}

		v << String(hz, 1) << " Hz: ";

		float db;

		if (scalePeak)
		{
			db = yNormalised * 100.0 - 100;
		}
		else
		{
			db = Decibels::gainToDecibels(yNormalised);
		}

		v << String(db, 1) << " dB";
		break;
	}
	case GraphType::Spectrograph:
	{
		int samplePos = jlimit(0, lastBuffer.getNumSamples() - 1, roundToInt(xNormalised * (float)lastBuffer.getNumSamples()));

		
		float hz;

		if (scaleFreq)
		{
			hz = getYPosition(1.0f - yNormalised);
		}
		else
		{
			hz = yNormalised;
		}

		float p = 0.0f;

		int c = roundToInt((float)lastBuffer.getNumChannels() * hz);
		c = jlimit(0, lastBuffer.getNumChannels() - 1, c);
		p = lastBuffer.getSample(c, samplePos);
		
		hz *= sr * 0.5;

		v << String(hz, 1) << " Hz: ";
		v << String(Decibels::gainToDecibels(p), 1) << "dB";
	}
    default:
        break;
	}

	DBG(v);

	return v;
}

float Graph::InternalGraph::getXPosition(float normalisedIndex) const
{
	auto scalePeak = parent.logPeakButton->getToggleState();
	auto scaleFreq = parent.logScaleButton->getToggleState();

	switch (parent.currentGraphType)
	{
	case GraphType::Spectrograph:
	{
		if (scalePeak)
		{
			auto l = Decibels::gainToDecibels(normalisedIndex);
			l = (l + 100.0f) / 100.0f;

			return l * l;
		}
		else
			return normalisedIndex;
	}
	case GraphType::Signal: return normalisedIndex;
	case GraphType::FFT:
	{
		if (scaleFreq)
			return std::exp(std::log(normalisedIndex) * 0.2f);
		else
			return normalisedIndex;
	}
    default:
        break;
	}

	return 0.0f;
}

float Graph::InternalGraph::getYPosition(float level) const
{
	switch (parent.currentGraphType)
	{
	case GraphType::Signal: return 1.0f - level;
	case GraphType::FFT:
	{
		auto logPeak = parent.logPeakButton->getToggleState();

		if (logPeak)
		{
			auto l = Decibels::gainToDecibels(level);

			return 1.0f - (l + 100.0f) / 100.0f;
		}
			
		else
			return 1.0f - level;
	}
	case GraphType::Spectrograph:
	{
		auto logFreq = parent.logScaleButton->getToggleState();

		if (logFreq)
			return 1.0f - std::exp(std::log(level) * 0.2f);
		else
			return 1.0f - jlimit(0.0f, 1.0f, level);
	}
    default: break;
	}

	return 0.0f;
}

void Graph::InternalGraph::rebuildSpectrumRectangles()
{
	if (lastBuffer.getNumSamples() == 0)
		return;

	spectroImage = Image(Image::ARGB, lastBuffer.getNumSamples(), lastBuffer.getNumChannels(), true);

	auto Spectrum2DSize = findParentComponentOfClass<Graph>()->Spectrum2DSize;


	auto maxLevel = lastBuffer.getMagnitude(0, lastBuffer.getNumSamples());

	if (maxLevel == 0.0f)
		return;

	for (int y = 0; y < Spectrum2DSize; y++)
	{
		auto skewedProportionY = getYPosition((float)y / (float)Spectrum2DSize);

		auto fftDataIndex = jlimit(0, Spectrum2DSize-1, (int)(skewedProportionY * (int)Spectrum2DSize));

		for (int i = 0; i < lastBuffer.getNumSamples(); i++)
		{
			auto s = lastBuffer.getSample(fftDataIndex, i);

			//auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
			
			s *= (1.0f / maxLevel);

			auto alpha = jlimit(0.0f, 1.0f, s);

			alpha = getXPosition(alpha);

			auto hueOffset = JUCE_LIVE_CONSTANT_OFF(0.4f);
			auto hueGain = JUCE_LIVE_CONSTANT_OFF(0.6f);

			auto hue = jlimit(0.0f, 1.0f, alpha * hueGain + hueOffset);

			spectroImage.setPixelAt(i, y, Colour::fromHSV(hue, 1.0f, 1.0f, alpha));
		}
	}

#if 0
	for (auto& sr : spectrumRectangles)
		sr.areas.consolidate();
#endif

	repaint();
}

namespace GraphIcons
{
	static const unsigned char loopIcon[] = { 110,109,8,172,30,65,4,86,134,64,108,195,245,14,65,4,86,134,64,98,14,45,12,65,219,249,202,64,72,225,222,64,201,118,0,65,254,212,152,64,201,118,0,65,98,229,208,106,64,201,118,0,65,23,217,38,64,23,217,242,64,213,120,233,63,193,202,217,64,108,244,253,52,
64,45,178,185,64,98,25,4,86,64,63,53,202,64,164,112,129,64,31,133,211,64,254,212,152,64,31,133,211,64,98,193,202,197,64,31,133,211,64,72,225,234,64,242,210,177,64,186,73,240,64,4,86,134,64,108,16,88,209,64,4,86,134,64,108,252,169,3,65,195,245,16,64,108,
8,172,30,65,4,86,134,64,99,109,209,34,123,63,27,47,117,64,98,207,247,147,63,217,206,215,63,145,237,60,64,0,0,0,0,18,131,164,64,0,0,0,0,98,158,239,199,64,0,0,0,0,133,235,233,64,174,71,225,62,213,120,1,65,127,106,156,63,108,23,217,226,64,201,118,14,64,
98,211,77,210,64,131,192,218,63,109,231,187,64,203,161,181,63,18,131,164,64,203,161,181,63,98,160,26,111,64,203,161,181,63,145,237,36,64,221,36,30,64,172,28,26,64,27,47,117,64,108,0,0,88,64,27,47,117,64,108,158,239,215,63,176,114,184,64,108,0,0,0,0,27,
47,117,64,108,209,34,123,63,27,47,117,64,99,101,0,0 };

	static const unsigned char bypassIcon[] = { 110,109,238,124,179,64,14,45,141,65,98,104,145,245,64,141,151,242,64,94,186,132,65,0,0,0,0,172,28,218,65,0,0,0,0,98,180,72,22,66,0,0,0,0,195,245,56,66,113,61,226,64,94,186,66,66,207,247,132,65,108,119,190,82,66,207,247,132,65,108,119,190,82,66,215,163,
228,64,108,43,135,134,66,180,200,173,65,108,119,190,82,66,57,52,17,66,108,119,190,82,66,154,153,214,65,108,72,97,36,66,154,153,214,65,108,72,97,36,66,170,241,152,65,98,84,227,30,66,59,223,75,65,33,48,8,66,240,167,254,64,172,28,218,65,240,167,254,64,98,
213,120,159,65,240,167,254,64,53,94,94,65,82,184,88,65,244,253,82,65,18,131,165,65,108,244,253,82,65,137,65,208,65,108,0,0,0,0,137,65,208,65,108,0,0,0,0,14,45,141,65,108,238,124,179,64,14,45,141,65,99,101,0,0 };

	static const unsigned char processIcon[] = { 110,109,8,44,34,66,164,112,157,65,98,113,61,26,66,231,251,197,65,0,128,6,66,94,186,226,65,47,221,222,65,94,186,226,65,98,94,186,176,65,94,186,226,65,137,65,137,65,231,251,197,65,156,196,114,65,164,112,157,65,108,0,0,0,0,164,112,157,65,108,0,0,0,0,178,
157,23,65,108,33,176,116,65,178,157,23,65,98,205,204,138,65,74,12,146,64,203,161,177,65,123,20,142,63,47,221,222,65,123,20,142,63,98,74,12,6,66,123,20,142,63,201,118,25,66,74,12,146,64,39,177,33,66,178,157,23,65,108,184,158,83,66,178,157,23,65,108,184,
158,83,66,0,0,0,0,108,76,247,134,66,125,63,105,65,108,184,158,83,66,125,63,233,65,108,184,158,83,66,164,112,157,65,108,8,44,34,66,164,112,157,65,99,101,0,0 };

	static const unsigned char markerIcons[] = { 110,109,25,4,12,65,168,134,160,66,108,0,0,0,0,168,134,160,66,108,0,0,0,0,111,18,3,59,108,55,137,141,64,111,18,3,59,98,182,243,141,64,111,18,131,58,102,102,142,64,111,18,131,58,23,217,142,64,0,0,0,0,108,117,147,96,66,0,0,0,0,98,170,241,96,66,166,155,68,
59,223,79,97,66,66,96,229,59,20,174,97,66,10,215,35,60,98,92,15,101,66,135,22,217,61,141,151,101,66,94,186,73,62,250,254,102,66,90,100,187,62,98,244,253,107,66,217,206,119,63,246,40,111,66,231,251,17,64,59,95,111,66,106,188,108,64,108,59,95,111,66,233,
38,108,66,98,119,62,111,66,74,140,111,66,127,234,110,66,166,27,112,66,23,89,110,66,86,142,113,66,98,45,50,108,66,49,8,119,66,178,157,102,66,82,184,122,66,117,147,96,66,176,242,122,66,108,25,4,12,65,176,242,122,66,108,25,4,12,65,168,134,160,66,99,109,
174,199,81,66,106,188,236,64,108,25,4,12,65,106,188,236,64,108,25,4,12,65,35,91,93,66,108,174,199,81,66,35,91,93,66,108,174,199,81,66,106,188,236,64,99,109,217,206,46,66,156,196,153,65,108,166,155,205,65,156,196,153,65,108,166,155,205,65,141,151,227,
65,108,119,62,43,66,141,151,227,65,108,119,62,43,66,193,202,6,66,108,166,155,205,65,193,202,6,66,108,166,155,205,65,41,220,49,66,108,133,107,49,66,41,220,49,66,108,29,90,48,66,188,244,70,66,108,63,53,141,65,188,244,70,66,108,63,53,141,65,80,141,95,65,
108,178,29,48,66,80,141,95,65,108,217,206,46,66,156,196,153,65,99,101,0,0 };

	static const unsigned char logIcon[] = { 110,109,68,11,139,67,172,156,120,67,108,201,118,28,65,172,156,120,67,98,29,90,12,65,182,115,120,67,150,67,247,64,176,114,120,67,88,57,216,64,12,34,120,67,98,72,225,114,64,199,43,119,67,244,253,180,63,166,187,116,67,125,63,245,62,225,218,113,67,98,121,
233,38,62,143,226,112,67,10,215,35,62,10,215,111,67,0,0,0,0,63,213,110,67,108,0,0,0,0,201,118,28,65,98,29,90,164,62,98,16,248,64,86,14,45,63,213,120,181,64,160,26,239,63,244,253,128,64,98,27,47,69,64,61,10,23,64,66,96,153,64,219,249,142,63,88,57,216,
64,125,63,245,62,98,150,67,247,64,121,233,38,62,29,90,12,65,10,215,35,62,201,118,28,65,0,0,0,0,108,68,11,139,67,0,0,0,0,98,74,140,139,67,10,215,35,62,47,13,140,67,248,83,163,62,20,142,140,67,125,63,245,62,98,111,2,141,67,233,38,113,63,104,129,141,67,
211,77,162,63,2,235,141,67,160,26,239,63,98,229,192,142,67,27,47,69,64,0,96,143,67,66,96,153,64,203,177,143,67,88,57,216,64,98,29,218,143,67,150,67,247,64,160,218,143,67,29,90,12,65,250,238,143,67,201,118,28,65,108,250,238,143,67,63,213,110,67,98,227,
197,143,67,231,219,112,67,115,152,143,67,164,240,114,67,0,0,143,67,188,148,116,67,98,156,100,142,67,66,64,118,67,121,137,141,67,184,126,119,67,20,142,140,67,12,34,120,67,98,236,17,140,67,176,114,120,67,74,140,139,67,182,115,120,67,68,11,139,67,172,156,
120,67,99,109,141,39,134,67,201,118,156,65,108,201,118,156,65,201,118,156,65,108,201,118,156,65,211,13,101,67,108,141,39,134,67,211,13,101,67,108,141,39,134,67,201,118,156,65,99,109,102,230,232,66,231,59,88,67,108,39,177,4,66,231,59,88,67,108,39,177,
4,66,88,185,3,66,108,102,230,232,66,88,185,3,66,108,102,230,232,66,231,59,88,67,99,109,238,252,89,67,231,59,88,67,108,236,17,65,67,231,59,88,67,108,236,17,65,67,88,185,3,66,108,238,252,89,67,88,185,3,66,108,238,252,89,67,231,59,88,67,99,109,86,142,47,
67,231,59,88,67,108,201,246,5,67,231,59,88,67,108,201,246,5,67,88,185,3,66,108,86,142,47,67,88,185,3,66,108,86,142,47,67,231,59,88,67,99,109,8,204,124,67,31,133,87,67,108,66,128,107,67,31,133,87,67,108,66,128,107,67,53,222,0,66,108,8,204,124,67,53,222,
0,66,108,8,204,124,67,31,133,87,67,99,101,0,0 };

	static const unsigned char compareIcon[] = { 110,109,207,247,149,67,53,46,190,67,108,51,115,79,67,53,46,190,67,108,111,146,102,67,156,68,234,66,108,49,104,138,67,156,68,234,66,108,207,247,149,67,53,46,190,67,99,109,98,240,76,67,100,187,135,67,98,33,240,76,67,4,6,164,67,236,17,31,67,31,245,186,67,
219,249,204,66,31,245,186,67,98,29,218,56,66,31,245,186,67,195,245,8,63,129,69,164,67,158,239,39,61,72,49,136,67,108,0,0,0,0,238,44,136,67,108,10,215,35,61,70,38,136,67,98,227,165,27,61,176,2,136,67,80,141,23,61,27,223,135,67,80,141,23,61,100,187,135,
67,108,78,98,48,63,100,187,135,67,98,154,153,25,65,250,254,129,67,133,235,203,66,96,37,13,67,47,29,206,66,219,249,13,67,98,162,197,207,66,94,154,14,67,4,22,42,67,135,22,99,67,182,147,76,67,141,87,135,67,108,195,181,75,67,100,187,135,67,108,98,240,76,
67,100,187,135,67,99,109,199,171,253,67,188,244,64,67,98,166,171,253,67,252,137,121,67,106,188,230,67,25,180,147,67,12,114,202,67,25,180,147,67,98,217,78,174,67,25,180,147,67,16,120,151,67,246,8,122,67,213,56,151,67,131,224,65,67,108,150,51,151,67,207,
215,65,67,108,180,56,151,67,127,202,65,67,98,82,56,151,67,84,131,65,67,49,56,151,67,231,59,65,67,49,56,151,67,188,244,64,67,108,199,139,151,67,188,244,64,67,98,98,0,156,67,231,123,53,67,119,46,202,67,80,141,122,66,193,186,202,67,53,222,125,66,98,221,
36,203,67,164,48,128,66,152,62,236,67,57,148,20,67,80,125,253,67,14,45,64,67,108,86,14,253,67,188,244,64,67,108,199,171,253,67,188,244,64,67,99,109,213,120,221,65,100,187,135,67,108,219,249,204,66,100,187,135,67,108,211,109,50,67,100,187,135,67,108,47,
29,206,66,76,23,68,67,108,213,120,221,65,100,187,135,67,99,109,35,11,165,67,188,244,64,67,108,12,114,202,67,188,244,64,67,108,127,106,240,67,188,244,64,67,108,193,186,202,67,127,42,235,66,108,35,11,165,67,188,244,64,67,99,109,43,135,81,67,35,91,209,66,
108,254,148,175,66,74,76,9,67,108,190,223,149,66,227,165,178,66,108,221,228,68,67,123,148,98,66,98,2,235,66,67,37,198,129,66,115,232,66,67,45,114,147,66,6,65,69,67,188,244,164,66,98,86,142,71,67,78,34,182,66,231,219,75,67,20,46,197,66,43,135,81,67,35,
91,209,66,99,109,102,166,110,67,47,93,6,66,98,82,72,129,67,61,10,226,65,115,136,139,67,47,93,32,66,117,51,142,67,37,6,112,66,98,119,222,144,67,10,215,159,66,117,243,138,67,141,215,200,66,86,254,128,67,150,131,211,66,98,111,18,110,67,158,47,222,66,111,
146,89,67,150,131,198,66,41,60,84,67,27,175,158,66,98,37,230,78,67,70,182,109,66,41,188,90,67,63,181,27,66,102,166,110,67,47,93,6,66,99,109,84,147,210,67,96,229,63,66,108,254,180,149,67,33,48,161,66,98,215,163,150,67,213,248,144,66,92,159,150,67,174,
71,127,66,213,120,149,67,133,235,92,66,98,139,76,148,67,102,230,57,66,4,22,146,67,223,79,27,66,133,43,143,67,70,182,2,66,108,4,38,204,67,0,0,0,0,108,84,147,210,67,96,229,63,66,99,101,0,0 };

	static const unsigned char testIcon[] = { 110,109,2,75,14,67,109,199,3,67,108,90,164,230,66,66,32,170,66,108,160,58,44,67,33,240,144,66,108,221,4,72,67,188,52,241,66,108,199,43,15,67,217,46,5,67,108,6,33,42,67,66,224,51,67,108,51,115,253,66,244,189,87,67,108,193,10,26,67,76,135,131,67,108,135,
54,3,67,92,175,139,67,108,45,146,94,67,178,205,218,67,108,143,162,52,67,246,232,230,67,108,0,0,0,0,117,243,20,67,108,125,191,39,66,94,122,249,66,108,131,64,64,66,219,89,7,67,108,86,206,141,66,223,15,238,66,108,209,98,197,66,252,41,39,67,108,188,180,151,
66,41,124,55,67,108,147,216,206,66,106,60,103,67,108,246,104,252,66,63,245,86,67,108,174,199,197,66,96,165,39,67,108,2,75,14,67,109,199,3,67,99,109,242,178,99,67,199,139,40,67,108,47,125,127,67,20,174,88,67,108,188,148,70,67,227,69,101,67,108,127,202,
42,67,150,35,53,67,108,242,178,99,67,199,139,40,67,99,109,88,121,157,67,109,39,192,66,108,231,43,138,67,184,94,12,67,108,6,17,152,67,197,128,60,67,108,119,94,171,67,195,53,16,67,108,88,121,157,67,109,39,192,66,99,109,133,171,119,67,176,50,182,66,108,
55,137,71,67,43,199,237,66,108,117,83,99,67,227,5,39,67,108,225,186,137,67,166,59,11,67,108,133,171,119,67,176,50,182,66,99,109,80,189,129,67,0,0,0,0,108,125,223,92,67,8,44,49,66,108,186,169,120,67,160,218,184,66,108,111,162,143,67,55,137,64,66,108,80,
189,129,67,0,0,0,0,99,101,0,0 };

}

juce::Path Graph::Icons::createPath(const String& t) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(t);

	Path p;

	LOAD_PATH_IF_URL("new-file", SampleMapIcons::newSampleMap);
	LOAD_PATH_IF_URL("open-file", SampleMapIcons::loadSampleMap);
	LOAD_PATH_IF_URL("save-file", SampleMapIcons::saveSampleMap);
	LOAD_PATH_IF_URL("switch-domains", SampleMapIcons::monolith);
	LOAD_PATH_IF_URL("processing-setup", SampleMapIcons::deleteSamples);
	LOAD_PATH_IF_URL("loop", GraphIcons::loopIcon);
	LOAD_PATH_IF_URL("delete", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_PATH_IF_URL("bypass", GraphIcons::bypassIcon);
	LOAD_PATH_IF_URL("process", GraphIcons::processIcon);
	LOAD_PATH_IF_URL("log-scale", GraphIcons::logIcon);
	LOAD_PATH_IF_URL("markers", GraphIcons::markerIcons);
	LOAD_PATH_IF_URL("compare", GraphIcons::compareIcon);


	LOAD_PATH_IF_URL("test", SnexIcons::bugIcon);
	return p;
}




void Graph::comboBoxChanged(ComboBox* cb)
{
	if (cb == graphType)
	{
		currentGraphType = (GraphType)cb->getSelectedItemIndex();
		updateFFTComponents();
		refreshDisplayedBuffer();
	}
	if (cb == windowType)
	{
		currentWindowType = (Helpers::WindowType)cb->getSelectedItemIndex();
		refreshDisplayedBuffer();
	}
	
}

void Graph::buttonClicked(Button* b)
{
	if (b == markerButton)
	{
		internalGraph.repaint();
	}
	else
		refreshDisplayedBuffer();
}

void Graph::postPostCompile(WorkbenchData::Ptr wb)
{
	auto r = wb->getLastResult();

	if (r.compiledOk())
	{
		cpuUsage = wb->getTestData().cpuUsage;
		
		refreshDisplayedBuffer();
		repaint();
	}
}

void Graph::paint(Graphics& g)
{
	auto total = getLocalBounds();

	auto buttonRow = total.removeFromTop(24);

	GlobalHiseLookAndFeel::drawFake3D(g, buttonRow);

	auto b = total.removeFromRight(30);

	b = b.removeFromTop(viewport.getMaximumVisibleHeight());

	g.setFont(GLOBAL_BOLD_FONT());

	g.setColour(Colours::white);

	if (internalGraph.stereoMode)
	{
		auto left = b.removeFromTop(b.getHeight() / 2).toFloat();
		auto right = b.toFloat();

		auto lMax = left.removeFromTop(18);
		auto lMin = left.removeFromBottom(18);

		g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
		g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);

		auto rMax = right.removeFromTop(18);
		auto rMin = right.removeFromBottom(18);

		g.drawText(juce::String(internalGraph.rightPeaks.getStart(), 1), rMin, Justification::left);
		g.drawText(juce::String(internalGraph.rightPeaks.getEnd(), 1), rMax, Justification::left);
	}
	else
	{
		auto left = b.removeFromTop(b.getHeight()).toFloat();

		auto lMax = left.removeFromTop(18);
		auto lMin = left.removeFromBottom(18);

		g.drawText(juce::String(internalGraph.leftPeaks.getStart(), 1), lMin, Justification::left);
		g.drawText(juce::String(internalGraph.leftPeaks.getEnd(), 1), lMax, Justification::left);
	}

	String cu;

	cu << "CPU: " << String(cpuUsage * 100.0, 2) << "%";

	g.setColour(Colours::white);
	g.drawText(cu, getLocalBounds().toFloat().reduced(30.f, 3.f), Justification::topRight);
}

void Graph::refreshDisplayedBuffer()
{
	resized();

	auto& td = getWorkbench()->getTestData();

	auto shouldProcess = processButton->getToggleState();
	const auto& bToUse = shouldProcess ? td.testOutputData : td.testSourceData;

	if (currentGraphType == GraphType::Signal)
	{
		internalGraph.setBuffer(bToUse);
	}
	else
	{
		processFFT(bToUse);
	}
}

void Graph::processFFT(const AudioSampleBuffer& originalSource)
{
#if USE_IPP
	auto& td = getWorkbench()->getTestData();

	if (currentGraphType == GraphType::Spectrograph)
	{

		auto order = jlimit(8, 13, (int)log2(nextPowerOfTwo(hmath::sqrt((double)originalSource.getNumSamples()))));

		if (logScaleButton->getToggleState())
			order += 2;

		Spectrum2DSize = roundToInt(hmath::pow(2.0, (double)order));

		auto numSamplesToFill = jmax(0, originalSource.getNumSamples() / Spectrum2DSize * 2 - 1);

		if (numSamplesToFill == 0)
			return;

		AudioSampleBuffer b(Spectrum2DSize, numSamplesToFill);

		hise::IppFFT fft(hise::IppFFT::DataType::RealFloat, order + 2);

		for (int i = 0; i < numSamplesToFill; i++)
		{
			auto offset = i * Spectrum2DSize / 2;
			AudioSampleBuffer sb(1, Spectrum2DSize * 2);
			sb.clear();

			auto numToCopy = jmin(Spectrum2DSize, originalSource.getNumSamples() - offset);

			FloatVectorOperations::copy(sb.getWritePointer(0), originalSource.getReadPointer(0, offset), numToCopy);

			Helpers::applyWindow(currentWindowType, sb);

			fft.realFFTInplace(sb.getWritePointer(0), Spectrum2DSize * 2);

			AudioSampleBuffer out(1, Spectrum2DSize);

			IppFFT::Helpers::toFreqSpectrum(sb, out);
			IppFFT::Helpers::scaleFrequencyOutput(out, false);

			for (int c = 0; c < Spectrum2DSize; c++)
			{
				b.setSample(c, i, out.getSample(0, c));
			}
		}

		internalGraph.setBuffer(b);
	}
	else
	{
		auto numToFFT = originalSource.getNumSamples();

		numToFFT = td.testSignalLength;

		if (originalSource.getNumSamples() < numToFFT)
			return;

		auto order = (int)log2(numToFFT);

		auto numOriginalSamples = numToFFT;

		if (numOriginalSamples == 0)
			return;

		AudioSampleBuffer temp;

		temp.setSize(1, numOriginalSamples * 2);
		temp.clear();

		FloatVectorOperations::copy(temp.getWritePointer(0), originalSource.getReadPointer(0), numOriginalSamples);

		Helpers::applyWindow(currentWindowType, temp);

		temp.setSize(temp.getNumChannels(), numOriginalSamples * 2, true, true);
		fftSource.setSize(1, numOriginalSamples);
		hise::IppFFT fft(hise::IppFFT::DataType::RealFloat, order + 2);
		fft.realFFTInplace(temp.getWritePointer(0), numOriginalSamples * 2);
		hise::IppFFT::Helpers::toFreqSpectrum(temp, fftSource);
		hise::IppFFT::Helpers::scaleFrequencyOutput(fftSource, false);
		internalGraph.setBuffer(fftSource);
	}
#endif
}

void ParameterList::rebuild()
{
	lastResult = getWorkbench()->getLastResult();

	if (lastResult.compiledOk())
	{
		functions.clear();
		sliders.clear();

		int numRows = 0;

		for (auto p : lastResult.parameters)
		{
			auto s = new juce::Slider(p.info.getId());
			s->setLookAndFeel(&laf);

			auto r = p.toRange();

			s->setRange(r.getRange(), r.interval);
			s->setSkewFactor(r.skew);
			s->setValue(p.info.defaultValue, dontSendNotification);

			s->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
			s->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
			s->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
			s->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
			s->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
			s->addListener(this);

			addAndMakeVisible(s);
			s->setSize(128, 48);
			sliders.add(s);
		}

		auto numColumns = jmax(1, getWidth() / 150);
		numRows = sliders.size() / numColumns + 1;

		resized();
	}

}

void ParameterList::sliderValueChanged(Slider* slider)
{
	auto index = sliders.indexOf(slider);

	if (isPositiveAndBelow(index, lastResult.parameters.size()))
	{
		auto value = (double)slider->getValue();
		lastResult.parameters.getReference(index).callback.call(value);
		getWorkbench()->triggerPostCompileActions();

#if 0
		// Move this to jit compiled thingie...
		f.callVoid(value);

		getWorkbench()->triggerPostCompileActions();
#endif

	}
}

void Graph::Helpers::applyWindow(WindowType t, AudioSampleBuffer& b)
{
	auto s = b.getNumSamples() / 2;
	auto data = b.getWritePointer(0);

	using DspWindowType = juce::dsp::WindowingFunction<float>;

	switch (t)
	{
	case Rectangle: 
		break;
	case BlackmanHarris:
		DspWindowType(s, DspWindowType::blackmanHarris, true).multiplyWithWindowingTable(data, s);
		break;
	case Hamming:
		DspWindowType(s, DspWindowType::hamming, true).multiplyWithWindowingTable(data, s);
		break;
	case Hann:
		DspWindowType(s, DspWindowType::hamming, true).multiplyWithWindowingTable(data, s);
		break;
	case Triangle:
		DspWindowType(s, DspWindowType::triangular, true).multiplyWithWindowingTable(data, s);
		break;
	case FlatTop:
		DspWindowType(s, DspWindowType::flatTop, true).multiplyWithWindowingTable(data, s);
		break;
	default:
		jassertfalse;
		FloatVectorOperations::clear(data, s);
		break;
	}
}

TestDataComponent::Item::Item(WorkbenchData::TestData& d, int i, bool isParameter_) :
	data(d),
	index(i),
	isParameter(isParameter_),
	deleteButton("Delete", this, f)
{
	addAndMakeVisible(jsonEditor);
	addAndMakeVisible(deleteButton);

	jsonEditor.addListener(this);

	jsonEditor.setFont(GLOBAL_MONOSPACE_FONT());

	var obj;

	if (isParameter)
	{
		auto p = d.getParameterEvent(index);
		obj = p.toJson();
	}
	else
	{
		auto e = d.getTestHiseEvent(index);
		obj = JitFileTestCase::getJSONData(e);
	}

	jsonEditor.setText(JSON::toString(obj, true), false);
}

void TestDataComponent::Item::textEditorReturnKeyPressed(TextEditor&)
{
	auto v = JSON::parse(jsonEditor.getText());

	if (!v.isObject())
		return;

	data.removeTestEvent(index, isParameter, dontSendNotification);

	if (isParameter)
	{
		data.addTestEvent(WorkbenchData::TestData::ParameterEvent(v));
	}
	else
	{
		auto e = JitFileTestCase::parseHiseEvent(v);
		data.addTestEvent(e);
	}
}


void TestDataComponent::buttonClicked(Button* b)
{
	auto& td = getWorkbench()->getTestData();

	if (b == &addParameter)
		td.addTestEvent(WorkbenchData::TestData::ParameterEvent());
	else if (b == &addEvent)
		td.addTestEvent(HiseEvent(HiseEvent::Type::NoteOn, 64, 127, 1));
	else if (b == testIcon)
	{
		getWorkbench()->setDebugMode(b->getToggleState(), sendNotificationAsync);

		return;
	}
	else if (b == compareButton)
	{
		td.saveCurrentTestOutput();

		

		return;
	}


	FileChooser fc("Choose Test file", td.getTestRootDirectory(), "*.json", true);

	if (b->getName() == "new-file")
	{
		td.clear(sendNotification);
	}

	if (b->getName() == "open-file")
	{
		if (fc.browseForFileToOpen())
		{
			td.loadFromFile(fc.getResult());
		}
	}
	if (b->getName() == "save-file")
	{
		if (fc.browseForFileToSave(true))
		{
			auto jsonData = JSON::toString(td.toJSON());
			fc.getResult().replaceWithText(jsonData);
			td.loadFromFile(fc.getResult());
		}
	}

	if (b->getName() == "compare")
	{

	}
}

void TestDataComponent::comboBoxChanged(ComboBox* cb)
{
	auto& td = getWorkbench()->getTestData();

	if (cb == signalLength)
		td.testSignalLength = cb->getText().getIntValue();
	if (cb == signalType)
		td.currentTestSignalType = (WorkbenchData::TestData::TestSignalMode)cb->getSelectedItemIndex();

	setTestBuffer(sendNotification);
}

}



}
