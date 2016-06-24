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


AudioProcessorEditorWrapper::AudioProcessorEditorWrapper(ProcessorEditor *p) :
ProcessorEditorBody(p)
{
	addAndMakeVisible(content = new Content(dynamic_cast<AudioProcessorWrapper*>(p->getProcessor())));
}


void AudioProcessorEditorWrapper::Content::comboBoxChanged(ComboBox *c)
{
	const Identifier id = Identifier(c->getItemText(c->getSelectedItemIndex()));
	
	if (getWrapper() != nullptr)
	{
		getWrapper()->setAudioProcessor(id);
	}
}

AudioProcessorEditorWrapper::~AudioProcessorEditorWrapper()
{
	content = nullptr;
	
}

AudioProcessorEditorWrapper::Content::Content(AudioProcessorWrapper *wrapper_):
  wrapper(wrapper_)
{
	setLookAndFeel(&laf);


	addAndMakeVisible(registeredProcessorList = new ComboBox());

	registeredProcessorList->addItemList(AudioProcessorWrapper::getRegisteredProcessorList(), 1);

	registeredProcessorList->addListener(this);

	registeredProcessorList->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colour(0x66333333));
	registeredProcessorList->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colour(0xfb111111));
	registeredProcessorList->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::white.withAlpha(0.3f));

	if (getWrapper() != nullptr)
	{
		unconnected = false;

		getWrapper()->addEditor(this);

		if (getWrapper()->getWrappedAudioProcessor() != nullptr)
		{
			setAudioProcessor(getWrapper()->getWrappedAudioProcessor());
		}
	}
	else
	{
		unconnected = true;
	}
}

void AudioProcessorEditorWrapper::Content::setAudioProcessor(AudioProcessor *newProcessor)
{
	if (getWrapper() != nullptr && newProcessor != nullptr)
	{
		addAndMakeVisible(wrappedEditor = newProcessor->createEditor());
		registeredProcessorList->setVisible(false);
	}
	else
	{
		registeredProcessorList->setVisible(true);
		wrappedEditor = nullptr;
	}

	if (AudioProcessorEditorWrapper *body = dynamic_cast<AudioProcessorEditorWrapper*>(getParentComponent()))
	{
		body->refreshBodySize();
	}
}
