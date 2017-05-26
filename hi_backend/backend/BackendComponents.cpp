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

#include "BackendComponents.h"


void MacroComponent::addSynthChainToPopup(ModulatorSynthChain *parent, PopupMenu &p, Array<MacroControlPopupData> &popupData)
{
	for(int i = 0; i < parent->getNumChildProcessors(); i++)
	{
		if(ModulatorSynthChain *msc = dynamic_cast<ModulatorSynthChain*>(parent->getChildProcessor(i)))
		{
			PopupMenu sub;

			for(int j = 0; j < 8; j++)
			{
				if(msc->hasActiveParameters(j))
				{
					MacroControlPopupData pData;

					pData.chain = msc;
					pData.macroIndex = j;
					pData.itemId = popupData.size() + 1;

					popupData.add(pData);

					sub.addItem(pData.itemId, msc->getMacroControlData(j)->getMacroName());
				}
			}

			if(sub.getNumItems() != 0) p.addSubMenu(msc->getId(), sub, true);

			addSynthChainToPopup(msc, p, popupData);
		}
	}
}

void MacroComponent::mouseDown(const MouseEvent &e)
{
	if(e.mods.isRightButtonDown() && dynamic_cast<Slider*>(e.eventComponent) != nullptr)
	{
		const int index = macroKnobs.indexOf(dynamic_cast<Slider*>(e.eventComponent));

		PopupMenu p;

		ScopedPointer<PopupLookAndFeel> laf = new PopupLookAndFeel();

		p.setLookAndFeel(laf);

		p.addSectionHeader("Add Macro Control from subchain");

		Array<MacroControlPopupData> popupData;

		addSynthChainToPopup(synthChain, p, popupData);

		if(p.getNumItems() == 1)
		{
			p.addItem(1, "No subchains with macro controls detected", false, false);
		}
		
		p.addSeparator();

		p.addItem(-1, "Clear MacroControl");

		p.addSeparator();

		p.addItem(-2, "MIDI Learn", synthChain->hasActiveParameters(index));

		const bool midiControlActive = synthChain->getMainController()->getMacroManager().midiControlActiveForMacro(index);

		String removeText;

		if(midiControlActive)
		{
			removeText << "Remove MIDI Control (CC Nr. " << String(synthChain->getMainController()->getMacroManager().getMidiControllerForMacro(index)) << ")";
		}
		else
		{
			removeText << "No MIDI Control";
		}


		p.addItem(-3, removeText, midiControlActive, false);

		const int result = p.show();

		if(result == -1)
		{
			synthChain->getMainController()->getMacroManager().setMacroControlMidiLearnMode(synthChain, index);
			synthChain->getMainController()->getMacroManager().setMidiControllerForMacro(-1);

			synthChain->clearData(index);

			macroNames[index]->setText("Macro " + String(index + 1), dontSendNotification);

		}
		else if(result == -2)
		{
			synthChain->getMainController()->getMacroManager().setMacroControlMidiLearnMode(synthChain, index);
		}
		else if(result == -3)
		{
			synthChain->getMainController()->getMacroManager().setMacroControlMidiLearnMode(synthChain, -1);
			synthChain->getMainController()->getMacroManager().removeMidiController(index);

		}

		for(int i = 0; i < popupData.size(); i++)
		{
			if(popupData[i].itemId == result)
			{
				const String name = popupData[i].chain->getMacroControlData(popupData[i].macroIndex)->getMacroName();

				const int macroIndex = macroKnobs.indexOf(dynamic_cast<Slider*>(e.eventComponent));

				debugToConsole(synthChain, "Adding " + name + " from " + popupData[i].chain->getId() + " to macro slot " + String(macroIndex));

				synthChain->replaceMacroControlData(macroIndex, popupData[i].chain->getMacroControlData(popupData[i].macroIndex), popupData[i].chain);

				macroNames[macroIndex]->setText(name, dontSendNotification);

			}
		}
			
			
	}
}

void MacroComponent::buttonClicked(Button *b)
{
	for(int i = 0; i < macroKnobs.size(); i++)
	{
		macroKnobs[i]->setEnabled(!b->getToggleState());


		editButtons[i]->setColours(Colours::black.withAlpha(0.5f), Colours::black.withAlpha(0.7f), Colour(SIGNAL_COLOUR));

	}

	if(b->getToggleState())
	{
		dynamic_cast<ShapeButton*>(b)->setColours(Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));

		for(int i = 0; i < editButtons.size(); i++)
		{
			editButtons[i]->setToggleState(editButtons[i] == b, dontSendNotification);

			if(editButtons[i] == b)
			{
				checkActiveButtons();

				macroNames[i]->setColour(Label::ColourIds::backgroundColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
					

				processor->getMacroManager().setMacroControlLearnMode(processor->getMainSynthChain(), i);

				if(table.getComponent() != nullptr)
				{
					table->showComponentInDebugArea(true);

					table.getComponent()->setMacroController(processor->getMainSynthChain()->getMacroControlData(i));

				}

				
			}

		}
	}

	else
	{
		
		checkActiveButtons();

		processor->getMacroManager().setMacroControlLearnMode(processor->getMainSynthChain(), -1);
		
		if(table.getComponent() != nullptr)
		{
			
			table->showComponentInDebugArea(false);

			table.getComponent()->setMacroController(nullptr);

		}


	}

	

};


void MacroComponent::changeListenerCallback(SafeChangeBroadcaster *)
{

	if(table.getComponent() != nullptr)
	{
		table.getComponent()->updateContent();
	}

	for(int i = 0; i < macroKnobs.size(); i++)
	{
		macroKnobs[i]->setValue(synthChain->getMacroControlData(i)->getCurrentValue(), dontSendNotification);
	}

	checkActiveButtons();
}


CachedViewport::CachedViewport()
{
	addAndMakeVisible(viewport = new InternalViewport());

	setOpaque(true);

	setWantsKeyboardFocus(false);

}

bool CachedViewport::isInterestedInDragSource(const SourceDetails & dragSourceDetails)
{
    return File::isAbsolutePath(dragSourceDetails.description.toString()) && File(dragSourceDetails.description).getFileExtension() == ".hip";
}

void CachedViewport::itemDragEnter(const SourceDetails &dragSourceDetails)
{
	dragNew = isInterestedInDragSource(dragSourceDetails);

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);

	repaint();
}

void CachedViewport::itemDragExit(const SourceDetails &/*dragSourceDetails*/)
{
	dragNew = false;

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);
	repaint();
}

void CachedViewport::itemDropped(const SourceDetails &dragSourceDetails)
{

	if (PresetHandler::showYesNoWindow("Replace Root Container",
		"Do you want to replace the root container with the preset?"))
	{
		findParentComponentOfClass<BackendProcessorEditor>()->loadNewContainer(File(dragSourceDetails.description.toString()));


	}
	dragNew = false;

	viewport->setColour(backgroundColourId, dragNew ? Colours::green.withAlpha(0.1f) :
		Colours::lightgrey);
	repaint();
}

void CachedViewport::resized()
{
	viewport->setBounds(0, 0, getWidth(), getHeight());

}

CachedViewport::InternalViewport::InternalViewport() :
isCurrentlyScrolling(false)
{
	setOpaque(true);

	setColour(backgroundColourId, Colours::lightgrey);

	//getVerticalScrollBar()->addMouseListener(this, true);
	//getVerticalScrollBar()->setWantsKeyboardFocus(false);
	getVerticalScrollBar()->setMouseClickGrabsKeyboardFocus(false);

	setWantsKeyboardFocus(false);

}



void CachedViewport::InternalViewport::paint(Graphics &g)
{
	Colour c15 = HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright);

	g.setColour(c15);
	g.fillAll();

	Rectangle<int> area = getLocalBounds();
	area.removeFromRight(getScrollBarThickness());
	area.setHeight(area.getHeight() + 5);

	Colour c2 = Colour(0xff242424);

	g.setColour(c2);
	g.fillRect(FLOAT_RECTANGLE(area));

	if (isCurrentlyScrolling)
	{

		const float scalingFactor = (float)Desktop::getInstance().getDisplays().getMainDisplay().scale;

		Rectangle<int> position = getViewArea();

		Rectangle<int> scaledPosition = Rectangle<int>((int)((float)position.getX() * scalingFactor),
			(int)((float)position.getY() * scalingFactor),
			(int)((float)position.getWidth() * scalingFactor),
			(int)((float)position.getHeight()*scalingFactor));

		Image clippedImage = cachedImage.getClippedImage(scaledPosition);

		AffineTransform backScaler = AffineTransform::scale(1.0f / scalingFactor);

		g.drawImageTransformed(clippedImage, backScaler);
	}
}

void BreadcrumbComponent::refreshBreadcrumbs()
{
	BackendProcessorEditor *bpe = findParentComponentOfClass<BackendProcessorEditor>();

	jassert(bpe != nullptr);

	if (bpe != nullptr)
	{
		breadcrumbs.clear();

		const Processor *mainSynthChain = bpe->getMainSynthChain();

		const Processor *currentRoot = bpe->getMainSynthChain()->getRootProcessor();

		while (currentRoot != mainSynthChain)
		{
			Breadcrumb *b = new Breadcrumb(currentRoot);
			breadcrumbs.add(b);
			addAndMakeVisible(b);

            currentRoot = ProcessorHelpers::findParentProcessor(currentRoot, false);
		}

		Breadcrumb *chain = new Breadcrumb(mainSynthChain);
		breadcrumbs.add(chain);
		addAndMakeVisible(chain);
	}

	resized();
}

void BreadcrumbComponent::resized()
{
    int x = 5;
    for(int i = breadcrumbs.size()-1; i  >= 0; i--)
    {
        breadcrumbs[i]->setBounds(x, 0, breadcrumbs[i]->getPreferredWidth(), getHeight()-8);
        x = breadcrumbs[i]->getRight() + 20;
    }

	repaint();
}

void BreadcrumbComponent::Breadcrumb::mouseDown(const MouseEvent& )
{
	findParentComponentOfClass<BackendProcessorEditor>()->setRootProcessorWithUndo(processor);
}

