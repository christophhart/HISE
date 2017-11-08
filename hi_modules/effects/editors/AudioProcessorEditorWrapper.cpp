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

#if USE_BACKEND
AudioProcessorEditorWrapper::AudioProcessorEditorWrapper(ProcessorEditor *p) :
ProcessorEditorBody(p)
{
	addAndMakeVisible(content = new WrappedAudioProcessorEditorContent(dynamic_cast<AudioProcessorWrapper*>(p->getProcessor())));
}


AudioProcessorEditorWrapper::~AudioProcessorEditorWrapper()
{
	content = nullptr;

}

#endif


void WrappedAudioProcessorEditorContent::comboBoxChanged(ComboBox *c)
{
	const Identifier id = Identifier(c->getItemText(c->getSelectedItemIndex()));
	
	if (getWrapper() != nullptr)
	{
		getWrapper()->setAudioProcessor(id);
	}
}


WrappedAudioProcessorEditorContent::WrappedAudioProcessorEditorContent(AudioProcessorWrapper *wrapper_) :
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

void WrappedAudioProcessorEditorContent::setAudioProcessor(AudioProcessor *newProcessor)
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

#if USE_BACKEND
	if (AudioProcessorEditorWrapper *body = dynamic_cast<AudioProcessorEditorWrapper*>(getParentComponent()))
	{
		body->refreshBodySize();
	}
#endif
}

} // namespace hise