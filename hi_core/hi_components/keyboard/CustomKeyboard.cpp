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

CustomKeyboardLookAndFeelBase::CustomKeyboardLookAndFeelBase()
{
	topLineColour = Colours::darkred;
	bgColour = Colour(BACKEND_BG_COLOUR_BRIGHT);
	overlayColour = Colours::red.withAlpha(0.12f);
}

void CustomKeyboardLookAndFeelBase::drawKeyboardBackground(Graphics &g, Component* c, int width, int height)
{
	if (!useFlatStyle)
	{
		g.setGradientFill(ColourGradient(Colour(0x7d000000),
			0.0f, 80.0f,
			Colour(0x00008000),
			5.0f, 80.0f,
			false));
		g.fillRect(0, 0, 16, height);

		g.setGradientFill(ColourGradient(Colour(0x7d000000),
			(float)width, 80.0f,
			Colour(0x00008000),
			(float)width - 5.0f, 80.0f,
			false));
		g.fillRect(width - 16, 0, 16, height);
	}
}


void CustomKeyboardLookAndFeelBase::drawWhiteNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &/*lineColour*/, const Colour &/*textColour*/)
{
	if (useFlatStyle)
	{
		Rectangle<int> r(x, y, w, h);

		float cornerSize = (float)roundToInt((float)w * 0.05f);

		r.reduce(2, 1);
		r.removeFromTop(4);

		g.setColour(bgColour);
		g.fillRoundedRectangle(r.toFloat(), cornerSize);

		g.setColour(overlayColour);
		g.drawRoundedRectangle(r.toFloat(), cornerSize, 1.0f);

		if (isDown)
		{
			g.setColour(activityColour);
			g.fillRoundedRectangle(r.toFloat(), cornerSize);
		}

	}
	else
	{
		float cornerSize = (float)w * 0.1f;
		g.setColour(Colours::black);
		//g.fillRect(x, y, w, h);

		if (!isDown)
			h -= (h / 20);

		Colour bc = isDown ? JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)) : Colour(0xFFCCCCCC);

		g.setGradientFill(ColourGradient(Colour(0xFFEEEEEE), 0.0f, 0.0f,
			bc, 0.0f, (float)(y + h), false));

		g.fillRoundedRectangle((float)x + 1.f, (float)y - cornerSize, (float)w - 2.f, (float)h + cornerSize, cornerSize);

		if (isOver)
		{
			g.setColour(overlayColour);
			g.fillRoundedRectangle((float)x + 1.f, (float)y - cornerSize, (float)w - 2.f, (float)h + cornerSize, cornerSize);
		}

		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, 0.0f,
			Colours::transparentBlack, 0.0f, 8.0f, false));

		g.fillRect(x, y, w, 8);

		g.setColour(Colour(BACKEND_BG_COLOUR_BRIGHT));
		g.drawLine((float)x, (float)y, (float)(x + w), (float)y, 2.0f);

		if (state->isColourDefinedForKey(midiNoteNumber))
		{
			g.setColour(state->getColourForSingleKey(midiNoteNumber));
			g.fillRoundedRectangle((float)x + 1.f, (float)y - cornerSize, (float)w - 2.f, (float)h + cornerSize, cornerSize);
		}
	}
}

void CustomKeyboardLookAndFeelBase::drawBlackNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour)
{
	if (useFlatStyle)
	{

		Rectangle<int> r(x, y, w, h);

		float cornerSize = (float)roundToInt((float)w * 0.09f);

		r.reduce(1, 1);

		g.setColour(topLineColour);
		g.fillRoundedRectangle(r.toFloat(), cornerSize);

		g.setColour(overlayColour);
		g.drawRoundedRectangle(r.toFloat(), cornerSize, 1.0f);

		if (isDown)
		{
			g.setColour(activityColour);
			g.fillRoundedRectangle(r.toFloat(), cornerSize);
		}

	}
	else
	{
		float cornerSize = (float)w * 0.1f;
		Rectangle<float> keyArea((float)x, (float)y - cornerSize, (float)w, (float)(h - cornerSize)*0.9f);
		float xOffset = JUCE_LIVE_CONSTANT_OFF(0.22f) * (float)w;
		float shadowHeight = isDown ? 0.05f : 0.18f * (float)h;

		Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333));

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));
		g.fillRoundedRectangle(keyArea, cornerSize);

		Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xff505050));

		g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f,
			isDown ? c1 : c2, 0.0f, (float)h, false));

		g.fillRect(keyArea.reduced(xOffset, shadowHeight));

		if (isOver)
		{
			g.setColour(overlayColour);
			g.fillRoundedRectangle(keyArea, cornerSize);
		}

		Path p;

		p.startNewSubPath(keyArea.getBottomLeft());
		p.lineTo((float)x + xOffset, keyArea.getBottom() - shadowHeight);
		p.lineTo(keyArea.getRight() - xOffset, keyArea.getBottom() - shadowHeight);
		p.lineTo(keyArea.getBottomRight());
		p.closeSubPath();

		g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0x36ffffff)), 0.0f, p.getBounds().getY(),
			Colours::transparentWhite, 0.0f, keyArea.getBottom(), false));

		g.fillPath(p);

		g.setColour(Colour(BACKEND_BG_COLOUR_BRIGHT));
		//g.drawRoundedRectangle(keyArea, cornerSize, 1.0f);

		if (state->isColourDefinedForKey(midiNoteNumber))
		{
			g.setColour(state->getColourForSingleKey(midiNoteNumber));
			g.fillRoundedRectangle(keyArea, cornerSize);
		}
	}
}

//==============================================================================
CustomKeyboard::CustomKeyboard(MainController* mc_) :
	MidiKeyboardComponent(mc_->getKeyboardState(), Orientation::horizontalKeyboard),
	state(&mc_->getKeyboardState()),
	mc(mc_),
    narrowKeys(true),
    lowKey(12)
{
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
		g.setFont(GLOBAL_BOLD_FONT().withHeight((float)w / 3.0f));
		
        g.setColour(Colours::grey);
        
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
