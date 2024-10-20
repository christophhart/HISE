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


SamplerBasePanel::SamplerBasePanel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
}

void SamplerBasePanel::paint(Graphics& g)
{
    PanelWithProcessorConnection::paint(g);
    auto b = getParentShell()->getContentBounds();
    g.setColour(Colour(0xFF262626));
    g.fillRect(b);

    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), 0.0f, b.getY(), Colours::transparentBlack, 0.0f, b.getY() + 15, false));

    g.fillRect(b.removeFromTop(15));
}

SamplerBasePanel::~SamplerBasePanel()
{
	if (getProcessor())
	{
		//getProcessor()->removeChangeListener(this);
	}
}

void SamplerBasePanel::changeListenerCallback(SafeChangeBroadcaster* /*b*/)
{
	if (auto sampler = dynamic_cast<ModulatorSampler*>(getProcessor()))
	{
		auto se = getContent<SamplerSubEditor>();

		if(sampler->shouldUpdateUI())
			se->updateInterface();
	}
}


Identifier SamplerBasePanel::getProcessorTypeId() const
{
	return ModulatorSampler::getConnectorId();
}

void SamplerBasePanel::contentChanged()
{
	if (getProcessor())
	{
		//getProcessor()->addChangeListener(this);
	}
}



SampleEditorPanel::SampleEditorPanel(FloatingTile* parent) :
	SamplerBasePanel(parent)
{

}

Component* SampleEditorPanel::createContentComponent(int /*index*/)
{
	return new SampleEditor(dynamic_cast<ModulatorSampler*>(getProcessor()), nullptr);
}


SampleMapEditorPanel::SampleMapEditorPanel(FloatingTile* parent) :
	SamplerBasePanel(parent)
{

}

Component* SampleMapEditorPanel::createContentComponent(int /*index*/)
{
	auto sme = new SampleMapEditor(dynamic_cast<ModulatorSampler*>(getProcessor()), nullptr);
	return sme;
}


SamplerTablePanel::SamplerTablePanel(FloatingTile* parent) :
	SamplerBasePanel(parent)
{

}

Component* SamplerTablePanel::createContentComponent(int /*index*/)
{
	auto st = new SamplerTable(dynamic_cast<ModulatorSampler*>(getProcessor()), nullptr);

	

	return st;
}

} // namespace hise
