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



Component * MidiKeyboardFocusTraverser::getDefaultComponent(Component *parentComponent)
{
#if USE_BACKEND

	if (auto p = parentComponent->findParentComponentOfClass<ParentWithKeyboardFocus>())						
		return dynamic_cast<Component*>(p);

	if (dynamic_cast<CopyPasteTarget*>(parentComponent))
	{
		dynamic_cast<CopyPasteTarget*>(parentComponent)->grabCopyAndPasteFocus();
	}
	else if (parentComponent->findParentComponentOfClass<CopyPasteTarget>())
	{
		parentComponent->findParentComponentOfClass<CopyPasteTarget>()->grabCopyAndPasteFocus();
	}
	else
	{
		BackendRootWindow *editor = parentComponent->findParentComponentOfClass<BackendRootWindow>();

		if (editor != nullptr) editor->setCopyPasteTarget(nullptr);
	}
#endif

	ComponentWithKeyboard *componentWithKeyboard = parentComponent->findParentComponentOfClass<ComponentWithKeyboard>();

	return componentWithKeyboard != nullptr ? componentWithKeyboard->getKeyboard() : nullptr;
}



//==============================================================================
CustomKeyboard::CustomKeyboard(MainController* mc_) :
	MidiKeyboardComponent(mc_->getKeyboardState(), Orientation::horizontalKeyboard),
	state(&mc_->getKeyboardState()),
	mc(mc_),
    narrowKeys(true),
  lowKey(12),
	currentKeyboardOctave(5)
{
	setKeyPressBaseOctave(currentKeyboardOctave);

	state->addChangeListener(this);
   
	setColour(whiteNoteColourId, Colours::black);

	ownedLaf = PresetHandler::createAlertWindowLookAndFeel();

	if (dynamic_cast<CustomKeyboardLookAndFeelBase*>(ownedLaf.get()) == nullptr)
		ownedLaf = new CustomKeyboardLookAndFeel();

	setLookAndFeel(ownedLaf);

    setOpaque(true);

#if HISE_IOS

	setKeyWidth(75.0f);
	setScrollButtonsVisible(false);

	setAvailableRange(36, 36 + 21);
    
#else

	

    setKeyWidth(narrowKeys ? 14.0f : 18.0f);
	setScrollButtonsVisible(false);
	
	setAvailableRange(9, 127);

#endif

	
}



CustomKeyboard::~CustomKeyboard()
{
	setLookAndFeel(nullptr);
	state->removeChangeListener(this);
}

void CustomKeyboard::buttonClicked(Button* b)
{
	if (b->getName() == "OctaveUp")
	{
		lowKey += 12;
	}
            
	else
	{
		lowKey -= 12;
	}
		
	setAvailableRange(lowKey, lowKey + 19);
}


void CustomKeyboard::paint(Graphics &g)
{
	if (!useCustomGraphics)
    {
        if(auto laf = dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel()))
            laf->drawKeyboardBackground(g, this, getWidth(), getHeight());
    }
		
	MidiKeyboardComponent::paint(g);
}

void CustomKeyboard::changeListenerCallback(SafeChangeBroadcaster*)
{
		
	repaint();
}

void CustomKeyboard::mouseDown(const MouseEvent& e)
{
	if (ccc)
	{
		if (ccc(e, true))
			return;
	}

	if (toggleMode)
	{
		auto number = getNoteAtPosition(e.getMouseDownPosition().toFloat());

		if (state->isNoteOnForChannels(getMidiChannelsToDisplay(), number))
		{
			state->noteOff(getMidiChannel(), number, 1.0f);
		}
		else
		{
			state->noteOn(getMidiChannel(), number, 1.0f);
		}
	}
	else
	{
		MidiKeyboardComponent::mouseDown(e);
	}
	
}


void CustomKeyboard::mouseUp(const MouseEvent& e)
{
	if (ccc)
	{
		if (ccc(e, false))
			return;
	}

	if (!toggleMode)
		MidiKeyboardComponent::mouseUp(e);
}


void CustomKeyboard::mouseDrag(const MouseEvent& e)
{
	if (!toggleMode)
		MidiKeyboardComponent::mouseDrag(e);
}

bool CustomKeyboard::keyPressed(const KeyPress& key)
{
	// Handle Z key - decrease octave
	if (key.getKeyCode() == 'z' || key.getKeyCode() == 'Z')
	{
		if (currentKeyboardOctave > 0)
		{
			currentKeyboardOctave--;
			setKeyPressBaseOctave(currentKeyboardOctave);
		}
		return true;
	}
	// Handle X key - increase octave
	else if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X')
	{
		if (currentKeyboardOctave < 10)
		{
			currentKeyboardOctave++;
			setKeyPressBaseOctave(currentKeyboardOctave);
		}
		return true;
	}

	// Let the parent class handle other keys (including the awsedftgyhujkolp; note keys)
	return MidiKeyboardComponent::keyPressed(key);
}

void CustomKeyboard::setUseCustomGraphics(bool shouldUseCustomGraphics)
{
	useCustomGraphics = shouldUseCustomGraphics;

	if (!useCustomGraphics)
		return;

	auto& handler = mc->getExpansionHandler();

	String wc = "{PROJECT_FOLDER}";

	if (FullInstrumentExpansion::isEnabled(mc) && handler.getCurrentExpansion() != nullptr)
		wc = handler.getCurrentExpansion()->getWildcard();

	for (int i = 0; i < 12; i++)
	{
		PoolReference upRef(mc, wc + "keyboard/up_" + String(i) + ".png", ProjectHandler::SubDirectories::Images);

		upImages.set(i, handler.loadImageReference(upRef, PoolHelpers::LoadAndCacheStrong));

		if (!upImages[i])
		{
			useCustomGraphics = false;
			break;
		}

		PoolReference downRef(mc, wc + "keyboard/down_" + String(i) + ".png", ProjectHandler::SubDirectories::Images);

		downImages.set(i, handler.loadImageReference(downRef, PoolHelpers::LoadAndCacheStrong));

		if (!downImages[i])
		{
			useCustomGraphics = false;
			break;
		}
	}

	repaint();
}

void CustomKeyboard::setMidiChannelBase(int newChannel)
{ 
	setMidiChannel(newChannel); 
	//BigInteger mask = 0;
	//mask.setBit(newChannel-1, true);
	//setMidiChannelsToDisplay(mask.toInteger());
}

void CustomKeyboard::setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle/*=false*/)
{
	if(auto laf = dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel()))
		laf->useFlatStyle = useFlatStyle;

	if (useFlatStyle)
	{
		setColour(MidiKeyboardComponent::ColourIds::whiteNoteColourId, Colours::transparentBlack);
	}

	setOpaque(!useFlatStyle);
}

void CustomKeyboard::setCustomClickCallback(const CustomClickCallback& f)
{
	ccc = f;
}

void CustomKeyboard::drawWhiteNote(int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour)
{
	auto areaInt = area.toNearestInt();

	int x = areaInt.getX();
	int y = areaInt.getY();
	int w = areaInt.getWidth();
	int h = areaInt.getHeight();

	if (useCustomGraphics)
	{
        SET_IMAGE_RESAMPLING_QUALITY();
        
		g.setOpacity(1.0f);
        
		const int index = midiNoteNumber % 12;

		if (auto keyImage = isDown ? downImages[index].getData() : upImages[index].getData())
		{
			g.drawImage(*keyImage,
				x, y, w, h,
				0, 0, keyImage->getWidth(), keyImage->getHeight());
		}


		

	}
	else
	{
        if(auto laf = dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel()))
            laf->drawWhiteNote(state, this, midiNoteNumber, g, x, y, w, h, isDown, isOver, lineColour, textColour);
	}

	if (displayOctaveNumber && midiNoteNumber % 12 == 0)
	{
		g.setFont(GLOBAL_BOLD_FONT().withHeight((float)w / 1.5f));
        g.setColour(Colours::darkgrey);
		g.drawText(MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 3), x, (h*3)/4, w, h / 4, Justification::centred);
	}
	
}

void CustomKeyboard::drawBlackNote(int midiNoteNumber, Graphics &g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour)
{
	auto areaInt = area.toNearestInt();

	int x = areaInt.getX();
	int y = areaInt.getY();
	int w = areaInt.getWidth();
	int h = areaInt.getHeight();

	if (useCustomGraphics)
	{
		g.setOpacity(1.0f);
        
		const int index = midiNoteNumber % 12;

		if (auto keyImage = isDown ? downImages[index].getData() : upImages[index].getData())
		{
			g.drawImage(*keyImage,
				x, y, w, h,
				0, 0, keyImage->getWidth(), keyImage->getHeight());
		}
	}
	else
	{
		if(auto laf = dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel()))
            laf->drawBlackNote(state, this, midiNoteNumber, g, x, y, w, h, isDown, isOver, noteFillColour);
	}
}

bool CustomKeyboard::isUsingFlatStyle() const
{
	if (auto laf = dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel()))
		return laf->useFlatStyle;

	return false;
}

void CustomKeyboard::setRange(int lowKey_, int hiKey_)
{
	lowKey = jlimit<int>(0, 100, lowKey_);
	hiKey = jlimit<int>(10, 128, hiKey_);

	setAvailableRange(lowKey, hiKey);
}
} // namespace hise
