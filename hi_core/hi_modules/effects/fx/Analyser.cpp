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






Colour AudioAnalyserComponent::getColourForAnalyser(RingBufferComponentBase::ColourId id)
{
	if (auto panel = findParentComponentOfClass<Panel>())
	{
		switch (id)
		{
		case RingBufferComponentBase::bgColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::bgColour);
		case RingBufferComponentBase::fillColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
		case RingBufferComponentBase::lineColour: return panel->findPanelColour(FloatingTileContent::PanelColourId::itemColour2);
            default: break;
		}
	}
	else
	{
		switch (id)
		{
		case RingBufferComponentBase::bgColour:   return findColour(RingBufferComponentBase::ColourId::bgColour);
		case RingBufferComponentBase::fillColour: return Colour(0xFF555555);
		case RingBufferComponentBase::lineColour: return Colour(0xFF555555);
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

	if(dynamic_cast<hise::AnalyserEffect*>(getProcessor()) == nullptr)
	{
		if(auto ed = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
		{
			if(isPositiveAndBelow(index, ed->getNumDataObjects(ExternalData::DataType::DisplayBuffer)))
			{
				auto rb = ed->getDisplayBuffer(index);
				jassert(rb != nullptr);

				auto obj = rb->getPropertyObject();
				auto editor = obj->createComponent();

				editor->setComplexDataUIBase(rb);

				

				c = dynamic_cast<Component*>(editor);

				c->setColour(0, findPanelColour(PanelColourId::bgColour));
				c->setColour(1, findPanelColour(PanelColourId::itemColour1));
				c->setColour(2, findPanelColour(PanelColourId::itemColour2));
			}
		}
	}
	else
	{
		switch (index)
		{
		case 0: c = new Goniometer(getProcessor()); break;
		case 1: c = new Oscilloscope(getProcessor()); break;
		case 2:	c = new FFTDisplay(getProcessor()); break;
		default:
			return nullptr;
		}
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
