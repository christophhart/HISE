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

	if (auto l_ = SingleWriteLockfreeMutex::ScopedReadLock(an->getBufferLock()))
	{
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
	}
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



juce::Point<float> Goniometer::Shape::createPointFromSample(float left, float right, float size)
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

Component* AudioAnalyserComponent::Panel::createContentComponent(int index)
{
	Component* c = nullptr;

	switch (index)
	{
	case 0: c = new Goniometer(getProcessor()); break;
	case 1: c = new Oscilloscope(getProcessor()); break;
	case 2:	c = new FFTDisplay(getProcessor()); break;
	default:
		return nullptr;
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
