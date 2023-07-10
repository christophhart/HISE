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


WaterfallComponent::WaterfallComponent(MainController* mc, ReferenceCountedObjectPtr<WavetableSound> sound_) :
	SimpleTimer(mc->getGlobalUIUpdater()),
	ControlledObject(mc),
	sound(sound_)
{
	start();
	setOpaque(true);
}




void WaterfallComponent::rebuildPaths()
{
	Array<Path> newPaths;

	if (auto first = sound.get())
	{
		auto numTables = first->getWavetableAmount();
		const auto realNumTables = numTables;

		int tableStride = jmax(1, numTables / 64);
		numTables = jmin(numTables, 64);

		auto size = first->getTableSize();

		stereo = first->isStereo();

		if (stereo)
			size *= 2;

		auto b = getLocalBounds().reduced(5).toFloat();

		b.removeFromTop(numTables);
		b.removeFromRight(numTables / 4);

		auto stride = b.getWidth() / (float)size;

		float maxGain = 0.0f;

		for (int i = 0; i < numTables; i++)
		{
			maxGain = jmax(maxGain, first->getUnnormalizedGainValue(i));
		}

		HeapBlock<float> data;
		data.calloc(size);

		auto reversed = first->isReversed();

		if (maxGain != 0.0f)
		{
			for (int pi = 0; pi < realNumTables; pi += tableStride)
			{
				Path p;

				auto thisBounds = b.translated((float)pi*0.25f / (float)tableStride, -pi / (float)tableStride);

				auto tableIndex = reversed ? (realNumTables - pi - 1) : pi;

				auto l = first->getWaveTableData(0, tableIndex);
				FloatVectorOperations::copy(data.get(), l, first->getTableSize());

				if (stereo)
				{
					auto r = first->getWaveTableData(1, tableIndex);
					FloatVectorOperations::copy(data.get() + first->getTableSize(), r, first->getTableSize());
				}

				p.startNewSubPath(thisBounds.getX(), thisBounds.getCentreY());

				auto gain = first->getUnnormalizedGainValue(tableIndex);

				if (gain == 0.0f)
					continue;

				//gain /= maxGain;

				gain = 1.0f / gain;
				//gain = hmath::pow(maxGain, 0.8f);

				for (int i = 0; i < b.getWidth(); i += 2)
				{
					auto uptime = ((float)i / thisBounds.getWidth()) * (float)size;



					int pos = (int)uptime;
					pos = jlimit(0, size - 1, pos);

					int nextPos = jlimit(0, size - 1, pos+1);

					auto alpha = uptime - (float)pos;

					auto value = Interpolator::interpolateLinear(data[pos], data[nextPos], alpha);

					p.lineTo(thisBounds.getX() + (float)i,
						thisBounds.getY() + thisBounds.getHeight() * 0.5f * (1.0f - value * gain));
				}

				p.lineTo(thisBounds.getRight(), thisBounds.getCentreY());

				newPaths.add(p);
			}
		}
	}

	std::swap(paths, newPaths);
	repaint();
}

void WaterfallComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));

	g.setColour(Colours::white.withAlpha(0.05f));
	g.drawRect(getLocalBounds().toFloat(), 1.0f);

	if (paths.isEmpty())
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawText("No preview available", getLocalBounds().toFloat(), Justification::centred);
		return;
	}

	float alpha = 1.0f;

	int idx = 0;

	for (const auto& p : paths)
	{
		float thisAlpha = 1.0f - jlimit(0.0f, 1.0f, (float)hmath::abs(idx - currentIndex) / (float)(paths.size()));

		thisAlpha = jmax(0.08f, hmath::pow(thisAlpha, 8.0f)*0.5f);
		thisAlpha *= alpha;

		if (idx == currentIndex)
		{
			thisAlpha = 1.0f;

			if (stereo)
			{
				g.setColour(Colours::white.withAlpha(0.5f));
				p.getBounds();


				g.setFont(GLOBAL_BOLD_FONT());
				auto pb = p.getBounds();

				g.drawText("L    R", pb, Justification::centredTop);
				g.drawVerticalLine(pb.getCentreX(), pb.getY(), pb.getBottom());
			}

		}

		if (idx != currentIndex && (idx % 2) != 0)
		{
			idx++;
			continue;
		}

		auto c = Colours::white.withAlpha(thisAlpha);

		g.setColour(c);
		alpha *= 0.988f;
		g.strokePath(p, PathStrokeType(idx == currentIndex ? 2.0f : 1.0f));

		idx++;
	}
}

void WaterfallComponent::timerCallback()
{
	if (!displayDataFunction)
		jassertfalse;

	auto df = displayDataFunction();

	float modValue = df.modValue;

	auto thisIndex = roundToInt(modValue * (paths.size() - 1));

	if (sound != df.sound)
	{
		sound = df.sound;
		rebuildPaths();
	}

	if (currentIndex != thisIndex)
	{
		currentIndex = thisIndex;
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
	repaint();
}

void SampleMapToWavetableConverter::Preview::mouseEnter(const MouseEvent& /*event*/)
{
}

void SampleMapToWavetableConverter::Preview::mouseExit(const MouseEvent& /*event*/)
{
	repaint();
}

void SampleMapToWavetableConverter::Preview::mouseDown(const MouseEvent& event)
{
}

void SampleMapToWavetableConverter::Preview::updateGraphics()
{
	repaint();
}





void SampleMapToWavetableConverter::Preview::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));

	if (spectrumImage.isValid())
	{
		g.drawImageWithin(spectrumImage, 0, 0, getWidth(), getHeight(), RectanglePlacement::stretchToFit);
		return;
	}
	else
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawText("No preview available", getLocalBounds().toFloat(), Justification::centred);
		return;
	}

#if 0
    float rectWidth = (float)getWidth() / (float)parent.numParts;

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
#endif
}

#if 0
void SampleMapToWavetableConverter::Preview::rebuildMap()
{

	if (const auto currentMap = parent.getCurrentMap())
	{
		int numHarmonics = currentMap->harmonicGains.getNumSamples();

		float rectWidth = (float)getWidth() / (float)currentMap->harmonicGains.getNumChannels();
		float rectHeight = 0.0f;

		if (numHarmonics != 0)
		{
			rectHeight = (float)getHeight() / (float)numHarmonics;
		}

		float x = 0.0f;

		harmonicList.clear();


		for (int j = 0; j < currentMap->harmonicGains.getNumChannels(); j++)
		{
			float y = 0.0f;

			for (int i = 0; i < currentMap->harmonicGains.getNumSamples(); i++)
			{
				float gainL = currentMap->harmonicGains.getSample(j, i);
				float gainR = currentMap->harmonicGainsRight.getNumSamples() > 0 ? currentMap->harmonicGainsRight.getSample(j, i) : gainL;

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

		repaint();
	}	

}
#endif

void SampleMapToWavetableConverter::SampleMapPreview::updateGraphics()
{
	samples.clear();

	for (const auto& s : parent.sampleMap)
		samples.add({ s, getLocalBounds() });

	for (auto& s : samples)
	{
		s.active = parent.currentIndex == s.index;

		if(isPositiveAndBelow(s.index, parent.harmonicMaps.size()))
			s.analysed = parent.harmonicMaps[s.index]->analysed;
	}

	repaint();
}

void SampleMapToWavetableConverter::SampleMapPreview::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
	g.setColour(Colours::white.withAlpha(0.05f));

	g.drawRect(getLocalBounds(), 1.0f);

	int widthPerNote = getWidth() / 128;
	int x = 0;

	for (int i = 0; i < 128; i++)
	{
		g.drawVerticalLine(x, 0.0f, (float)getWidth());
		x += widthPerNote;
	}

	if (samples.isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("No samplemap loaded", getLocalBounds().toFloat(), Justification::centred);
	}

	for (auto& s : samples)
	{
		Colour c = s.index == parent.getCurrentMap()->index.sampleIndex ? Colour(SIGNAL_COLOUR) : Colours::grey;

		g.setColour(c.withAlpha(s.index == hoverIndex ? 0.6f : 0.5f));

		if (s.area.getHeight() > 0)
		{
			g.drawRect(s.area, 1);
			g.fillRect(s.area);
		}

		auto l = s.keyRange.getLength();

		if (l > parent.mipmapSize)
		{
			bool bright = true;
			for (int i = s.keyRange.getStart(); i < s.keyRange.getEnd(); i += parent.mipmapSize)
			{
				bright = !bright;

				auto w = jmin(parent.mipmapSize, s.keyRange.getEnd() - i);

				g.setColour((bright ? Colours::white : Colours::black).withAlpha(0.03f));
				
				g.fillRect(i * widthPerNote, s.area.getY(), w * widthPerNote, s.area.getHeight());
			}
		}
	}

	
}

void SampleMapToWavetableConverter::SampleMapPreview::mouseDown(const MouseEvent& e)
{
	for (auto& s : samples)
	{
		if (s.area.contains(e.getPosition()))
		{
			indexBroadcaster.sendMessage(sendNotificationSync, s.index);
			repaint();
			break;
		}
	}
}

void SampleMapToWavetableConverter::SampleMapPreview::mouseMove(const MouseEvent& e)
{
	for (auto& s : samples)
	{
		if (s.area.contains(e.getPosition()))
		{
			hoverIndex = s.index;
			repaint();
			break;
		}
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
	index = data.getParent().indexOf(data);

	keyRange = { d.lowKey, d.highKey };
	rootNote = d.rootNote;
}

#endif






}
