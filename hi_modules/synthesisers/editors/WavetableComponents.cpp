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




#if USE_BACKEND

SampleMapToWavetableConverter::Preview::Preview(SampleMapToWavetableConverter& parent_) :
	WavetablePreviewBase(parent_),
	ControlledObject(parent_.chain->getMainController()),
	SimpleTimer(getMainController()->getGlobalUIUpdater())
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
	g.fillAll(Colours::black);

	if (spectrumImage.isValid())
	{
		g.drawImageWithin(spectrumImage, 0, 0, getWidth(), getHeight(), RectanglePlacement::stretchToFit);
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawRect(getLocalBounds().toFloat(), 1.0f);

		if (previewPosition >= 0.0)
		{
			auto x = (float)previewPosition * (float)getWidth();

			g.setColour(Colours::white.withAlpha(0.1f));

			g.fillRect(x - 5.0f, 0.0, 10.0f, (float)getHeight());

			g.setColour(Colours::white.withAlpha(0.8f));
			g.fillRect(x - 1.0f, 0.0, 2.0f, (float)getHeight());
		}

		return;
	}
	else
	{
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawRect(getLocalBounds().toFloat(), 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
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



void SampleMapToWavetableConverter::Preview::timerCallback()
{
	auto numSamples = getMainController()->getPreviewBufferSize();

	if (numSamples != 0)
	{
		auto thisPreviewPosition = (double)getMainController()->getPreviewBufferPosition() / (double)numSamples;
		
		if (previewPosition != thisPreviewPosition)
		{
			previewPosition = thisPreviewPosition;
			repaint();
		}
	}
	else
	{
		if (previewPosition != -1.0)
			repaint();

		previewPosition = -1.0;
	}
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

	if (insertPosition != -1)
	{
		auto c = Colour(SIGNAL_COLOUR);
		g.setColour(c.withAlpha(0.05f));
		g.fillRect(getLocalBounds());

		Rectangle<int> ip(insertPosition * (getWidth() / 128), 0, getWidth() / 128, getHeight());

		g.setColour(c.withAlpha(0.2f));
		g.fillRect(ip.toFloat());
		g.setColour(c.withAlpha(0.6f));
		g.drawRect(ip.toFloat(), 2.0f);

		g.setFont(GLOBAL_BOLD_FONT());

		String message;

		message << "Load sample with root note ";
		message << MidiMessage::getMidiNoteName(insertPosition, true, true, 3);
		message << "(" << roundToInt(48000.0 / MidiMessage::getMidiNoteInHertz(insertPosition)) << " cycle length)";

		g.drawText(message, getLocalBounds().toFloat(), Justification::centred);
	}
	else if (samples.isEmpty())
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Load samplemap or drop audio file", getLocalBounds().toFloat(), Justification::centred);
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

void SampleMapToWavetableConverter::SampleMapPreview::filesDropped(const StringArray& files, int x, int y)
{
	ValueTree v("samplemap");

	double sr, unused;

	File f(files[0]);

	auto fileContent = hlac::CompressionHelpers::loadFile(f, unused, &sr);

	auto numSamples = PitchDetection::getNumSamplesNeeded(sr, 20.0);

	AudioSampleBuffer wb(2, numSamples);

	auto estimatedFreq = MidiMessage::getMidiNoteInHertz(insertPosition);

	auto pitch = PitchDetection::detectPitch(f, wb, sr, estimatedFreq);

	if (pitch == 0.0)
	{
		PresetHandler::showMessageWindow("The root frequency can't be detected.", "The pitch detection failed to use the provided root note. Try another root note", PresetHandler::IconType::Error);
		return;
	}

	auto length = roundToInt(sr / pitch);

	ValueTree sample("sample");

	PoolReference ref(parent.chain->getMainController(), f.getFullPathName(), FileHandlerBase::Samples);

	sample.setProperty(SampleIds::FileName, ref.getReferenceString(), nullptr);
	sample.setProperty(SampleIds::LoKey, 0, nullptr);
	sample.setProperty(SampleIds::HiKey, 127, nullptr);
	sample.setProperty(SampleIds::LoVel, 0, nullptr);
	sample.setProperty(SampleIds::HiVel, 127, nullptr);

	ResynthesisHelpers::writeRootAndPitch(sample, sr, length);

	v.addChild(sample, -1, nullptr);
	v.setProperty(SampleIds::ID, f.getFileNameWithoutExtension(), nullptr);
	v.setProperty("SaveMode", 0, nullptr);

	if (sampleMapLoadFunction)
		sampleMapLoadFunction(v);

	insertPosition = -1;
	repaint();
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
