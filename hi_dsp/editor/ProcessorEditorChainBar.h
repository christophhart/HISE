
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

#ifndef PROCESSOR_EDITOR_CHAIN_BAR_H_INCLUDED
#define PROCESSOR_EDITOR_CHAIN_BAR_H_INCLUDED

namespace hise { using namespace juce;

class ProcessorEditorChainBar  : public ProcessorEditorChildComponent,
								 public ButtonListener,
								 public SafeChangeListener,
								 public DragAndDropTarget,
								 public Timer
{
public:
    //==============================================================================
    ProcessorEditorChainBar (ProcessorEditor *p);
    ~ProcessorEditorChainBar();

	

	int getActualHeight()
	{ 
#if HISE_IOS
		return chainButtons.size() != 0 ? 35 : 0; 
#else
		return chainButtons.size() != 0 ? 22 : 0; 
#endif
	};

	/** This shortens the ModulatorChain ids ("GainModulation" -> "Gain" etc.) for nicer display. */
	String getShortName(const String identifier) const
	{
		if(identifier == "GainModulation") return "Gain";
		else if (identifier == "PitchModulation") return "Pitch";
		else if (identifier == "Midi Processor") return "MIDI";
		else return identifier;
	}

	/** Checks if the hidden Chain contains childs and paints it red if yes. */
	void checkActiveChilds(int chainToCheck);
	
	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		for(int i = 1; i < chainButtons.size(); i++)
		{
			checkActiveChilds(i-1);
		}
	}

	bool isInterestedInDragSource(const SourceDetails & 	dragSourceDetails) override;

	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
	void itemDragExit(const SourceDetails &dragSourceDetails) override;
	void itemDropped(const SourceDetails &dragSourceDetails) override;
	void itemDragMove(const SourceDetails& dragSourceDetails) override;;

	void refreshPanel();

    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;

	void closeAll();
	
	void paint(Graphics& g) override;

	void paintOverChildren(Graphics &g) override;

	void timerCallback() override;

	Chain *getChainForButton(Component *checkComponent)
	{
		int index = chainButtons.indexOf(dynamic_cast<TextButton*>(checkComponent));

		if (index > 0)
		{
	

			return dynamic_cast<Chain*>(getEditor()->getProcessor()->getChildProcessor(index - 1));
		}
		else return nullptr;
	}
	
	void setMidiIconActive(bool shouldBeActive) noexcept
	{
		if (midiButton != nullptr)
		{
			if (!shouldBeActive && midiActive)
			{
				midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColour, Colour(0xaa000000));
				midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColourOff, Colour(0x99ffffff));
				repaint();
			}
			else if (shouldBeActive && !midiActive)
			{
				midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColour, Colours::black.withAlpha(0.3f));
				midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColourOff, Colours::white);

				

				repaint();
				startTimer(150);
			}

			midiActive = shouldBeActive;
		}
		
	}
	

private:

	NumberTag::LookAndFeelMethods numberRenderer;

	int insertPosition;

	Component::SafePointer<TextButton> midiButton;

	OwnedArray<TextButton> chainButtons;
	OwnedArray<NumberTag> numberTags;

	Array<int> numProcessorList;

	bool itemDragging;
	bool canBeDropped;

	void setDragInsertPosition(int newInsertPosition)
	{
		if (itemDragging)
		{
			insertPosition = newInsertPosition;
			repaint();
		}
		else insertPosition = -1;
	}

	ScopedPointer<ChainBarButtonLookAndFeel> laf;

    bool midiActive = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditorChainBar)
};

} // namespace hise

#endif   
