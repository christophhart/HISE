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



WavetableDisplayComponent::WavetableDisplayComponent(ModulatorSynth *synth_) :
	synth(synth_),
	tableLength(0),
	tableValues(nullptr)
{
	setColour(bgColour, Colours::transparentBlack);
	setColour(lineColour, Colours::white);
	setColour(fillColour, Colours::white.withAlpha(0.5f));

	if (isDisplayForWavetableSynth()) startTimer(50);
	else setBufferedToImage(true);
}

bool WavetableDisplayComponent::isDisplayForWavetableSynth() const
{
	return dynamic_cast<WavetableSynth*>(synth) != nullptr;
}

void WavetableDisplayComponent::paint(Graphics &g)
{
	Path p;

	float w = (float)getWidth();
	float h = (float)getHeight();

	p.startNewSubPath(0.0, h / 2.0f);

	const float cycle = tableLength / w;

	if (tableValues != nullptr)
	{

		for (int i = 0; i < getWidth(); i++)
		{
			const float tableIndex = ((float)i * cycle);

			const int x1 = (int)tableIndex;
			const int x2 = (x1 + 1) % tableLength;
			const float alpha = tableIndex - (float)x1;

			const float value = Interpolator::interpolateLinear(tableValues[x1], tableValues[x2], alpha);

			jassert(tableIndex < tableLength);

			p.lineTo((float)i, value * -(h - 2) / 2 + h / 2);
		}
	}

	p.lineTo(w, h / 2.0f);

	p.closeSubPath();

	if (useFlatDesign)
	{
		g.setColour(findColour(bgColour));
		g.fillAll();

		g.setColour(findColour(fillColour));
		g.fillPath(p);

		g.setColour(findColour(lineColour));
		g.strokePath(p, PathStrokeType(2.0f));
	}
	else
	{
		KnobLookAndFeel::fillPathHiStyle(g, p, (int)w, (int)h);
	}
}

juce::Identifier WavetableDisplayComponent::Panel::getProcessorTypeId() const
{
	return WavetableSynth::getClassType();
}

Component* WavetableDisplayComponent::Panel::createContentComponent(int /*index*/)
{
	auto c = new WavetableDisplayComponent(dynamic_cast<WavetableSynth*>(getProcessor()));

	c->setUseFlatDesign(true);
	c->setColour(bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	c->setColour(fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
	c->setColour(lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));

	return c;
}

void WavetableDisplayComponent::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<WavetableSynth>(moduleList);
}


void WavetableDisplayComponent::timerCallback()
{
	ModulatorSynthVoice *voice = dynamic_cast<ModulatorSynthVoice*>(synth->getLastStartedVoice());

	if (voice == nullptr) return;

	ModulatorSynthSound *sound = dynamic_cast<ModulatorSynthSound*>(voice->getCurrentlyPlayingSound().get());

	if (dynamic_cast<WavetableSound*>(sound) != nullptr)
	{
		WavetableSound *wavetableSound = dynamic_cast<WavetableSound*>(sound);
		WavetableSynthVoice *wavetableVoice = dynamic_cast<WavetableSynthVoice*>(voice);

		const int tableIndex = wavetableVoice->getCurrentTableIndex();
		tableValues = wavetableSound->getWaveTableData(tableIndex);
		tableLength = wavetableSound->getTableSize();



		normalizeValue = 1.0f / wavetableSound->getMaxLevel();

		repaint();
	}
	else if (!isDisplayForWavetableSynth())
	{
		tableValues = dynamic_cast<SineSynth*>(synth)->getSaturatedTableValues();
		tableLength = 128;
		normalizeValue = 1.0f;

		repaint();
	}
	else
	{
		tableValues = nullptr;
		repaint();
	}



}



#if USE_BACKEND

SampleMapToWavetableConverter::Preview::Preview(SampleMapToWavetableConverter& parent_) :
	WavetablePreviewBase(parent_)
{
	

	setName("Harmonic Map Preview");
}

SampleMapToWavetableConverter::Preview::~Preview()
{
	
}

void SampleMapToWavetableConverter::Preview::mouseMove(const MouseEvent& event)
{
	hoverIndex = getHoverIndex(event.getMouseDownX());
	repaint();
}

void SampleMapToWavetableConverter::Preview::mouseEnter(const MouseEvent& /*event*/)
{
	setMouseCursor(MouseCursor::CrosshairCursor);
}

void SampleMapToWavetableConverter::Preview::mouseExit(const MouseEvent& /*event*/)
{
	setMouseCursor(MouseCursor::NormalCursor);

	hoverIndex = -1;
	repaint();
}

void SampleMapToWavetableConverter::Preview::mouseDown(const MouseEvent& event)
{
	int index = getHoverIndex(event.getMouseDownX());

	parent.replacePartOfCurrentMap(index);

	rebuildMap(); 
}

void SampleMapToWavetableConverter::Preview::updateGraphics()
{
	rebuildMap();
}

int SampleMapToWavetableConverter::Preview::getHoverIndex(int xPos) const
{
	int index = (int)((float)xPos / (float)getWidth() * (float)parent.numParts);
	index = jlimit<int>(0, parent.numParts, index);

	return index;
}



void SampleMapToWavetableConverter::Preview::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
	float rectWidth = (float)getWidth() / 64.0f;

	for (auto& r : harmonicList)
	{
		float alpha = jmax(r.lGain, r.rGain);
		alpha = jlimit(0.0f, 1.0f, alpha);

		uint8 redPart = (uint8)roundToInt(r.lGain * 255.0f);
		uint8 greenPart = (uint8)roundToInt(r.rGain * 255.0f);
		uint8 bluePart = (uint8)roundToInt(alpha * 255.0f);

		uint32 c = 0;
		c |= ((uint32)redPart << 16);
		c |= ((uint32)greenPart) << 8;
		c |= ((uint32)bluePart);

		Colour co(c);
		co = co.withAlpha(alpha);

		g.setColour(co);
		g.fillRect(r.area);
	}

	if (hoverIndex != -1)
	{
		auto area = Rectangle<float>((float)(hoverIndex * rectWidth), 0, rectWidth, (float)getHeight());
		g.setColour(Colours::red.withAlpha(0.1f));
		g.fillRect(area);
	}

	g.setColour(Colour(0xff7559a4).withAlpha(0.4f));
	g.strokePath(p, PathStrokeType(2.0f));
}

void SampleMapToWavetableConverter::Preview::rebuildMap()
{
	float rectWidth = (float)getWidth() / 64.0f;
	float rectHeight = 0.0f;

	const auto& currentMap = parent.harmonicMaps.getReference(parent.currentIndex);

	int numHarmonics = currentMap.harmonicGains.getNumSamples();

	if (numHarmonics != 0)
	{
		rectHeight = (float)getHeight() / (float)numHarmonics;
	}

	float x = 0.0f;



	harmonicList.clear();
	

	for (int j = 0; j < currentMap.harmonicGains.getNumChannels(); j++)
	{
		float y = 0.0f;

		for (int i = 0; i < currentMap.harmonicGains.getNumSamples(); i++)
		{
			float gainL = currentMap.harmonicGains.getSample(j, i);
			float gainR = currentMap.harmonicGainsRight.getSample(j, i);

			gainL = powf(gainL, 0.25f);
			gainR = powf(gainR, 0.25f);

			auto area = Rectangle<float>(x, (float)getHeight() - y, rectWidth, rectHeight);

			Harmonic l;
			
			l.area = area;
			l.lGain = gainL;
			l.rGain = gainR;

			harmonicList.add(l);
			
			y += rectHeight;
		}

		x += rectWidth;
	}

	p.clear();

	p.startNewSubPath(0.0f, (float)getHeight() / 2.0f);

	for (int i = 0; i < parent.numParts; i++)
	{
		auto deviation = jlimit<float>(-100.0f, 100.0f, (float)currentMap.pitchDeviations[i]);

        deviation = FloatSanitizers::sanitizeFloatNumber(deviation);
        
		auto scaled = deviation / 100.0 * (float)getHeight() / 2.0f + getHeight() / 2.0f;

		p.lineTo((float)i*rectWidth, (float)scaled);
	}

	p.lineTo((float)getWidth(), (float)getHeight() / 2.0f);

	repaint();
}

void SampleMapToWavetableConverter::SampleMapPreview::updateGraphics()
{
	samples.clear();

	for (const auto& s : parent.sampleMap)
		samples.add({ s, getLocalBounds() });

	for (auto& s : samples)
	{
		s.active = parent.currentIndex == s.index;
		s.analysed = parent.harmonicMaps[s.index].analysed;
	}

	repaint();
}

void SampleMapToWavetableConverter::SampleMapPreview::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.1f));

	int widthPerNote = getWidth() / 128;
	int x = 0;

	for (int i = 0; i < 128; i++)
	{
		g.drawVerticalLine(x, 0.0f, (float)getWidth());
		x += widthPerNote;
	}

	for (auto& s : samples)
	{
		Colour c = s.analysed ? Colours::green : Colours::grey;
		if (s.active)
			c = Colours::white;

		g.setColour(c.withAlpha(0.6f));
		g.drawRect(s.area, 1);
		g.fillRect(s.area);
	}
}

SampleMapToWavetableConverter::SampleMapPreview::Sample::Sample(const ValueTree& data, Rectangle<int> totalArea)
{
	auto d = StreamingHelpers::getBasicMappingDataFromSample(data);

	int x = jmap((int)d.lowKey, 0, 128, 0, totalArea.getWidth());
	int w = jmap((int)(1 + d.highKey - d.lowKey), 0, 128, 0, totalArea.getWidth());
	int y = jmap((int)d.highVelocity, 128, 0, 0, totalArea.getHeight());
	int h = jmap((int)(1 + d.highVelocity - d.lowVelocity), 0, 128, 0, totalArea.getHeight()-1);

	area = { x, y, w, h };
	index = data.getProperty("ID");
}

#endif

}
