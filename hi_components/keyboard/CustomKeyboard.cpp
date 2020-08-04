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
	if (FileBrowser *browser = parentComponent->findParentComponentOfClass<FileBrowser>())						return browser;
	if (SamplerBody *samplerBody = parentComponent->findParentComponentOfClass<SamplerBody>())					return samplerBody;
	else if (ScriptingContentOverlay::Dragger* dragger = parentComponent->findParentComponentOfClass<ScriptingContentOverlay::Dragger>()) return dragger;
	else if (MacroParameterTable *table = parentComponent->findParentComponentOfClass<MacroParameterTable>())	return table;

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


CustomKeyboardLookAndFeel::CustomKeyboardLookAndFeel()
{
	topLineColour = Colours::darkred;
	bgColour = Colour(BACKEND_BG_COLOUR_BRIGHT);
	overlayColour = Colours::red.withAlpha(0.12f);

#if HISE_IOS
	useVectorGraphics = true;
#endif

	cachedImage_black_key_off_png = ImageCache::getFromMemory(black_key_off_png, black_key_off_pngSize);
	cachedImage_black_key_on_png = ImageCache::getFromMemory(black_key_on_png, black_key_on_pngSize);
	cachedImage_white_key_off_png = ImageCache::getFromMemory(white_key_off_png, white_key_off_pngSize);
	cachedImage_white_key_on_png = ImageCache::getFromMemory(white_key_on_png, white_key_on_pngSize);
}


void CustomKeyboardLookAndFeel::drawKeyboardBackground(Graphics &g, int width, int height)
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


void CustomKeyboardLookAndFeel::drawWhiteNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &/*lineColour*/, const Colour &/*textColour*/)
{
	if (useVectorGraphics)
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
	else
	{
		const int off = midiNoteNumber % 12;

		g.setOpacity(1.0f);
		if (!isDown)
		{
			g.drawImage(cachedImage_white_key_on_png,
				x, y, w, h,
				0, 0, cachedImage_white_key_on_png.getWidth(), cachedImage_white_key_on_png.getHeight());

			g.setColour(Colours::grey.withAlpha(0.2f));
			g.fillRect(x, y, w, h);

		}
		else
		{
			g.drawImage(cachedImage_white_key_off_png,
				x, y, w, h,
				0, 0, cachedImage_white_key_off_png.getWidth(), cachedImage_white_key_on_png.getHeight());
		}

		if (off == 0)
		{
			g.setColour(Colours::black.withAlpha(0.5f));
			g.setFont(GLOBAL_MONOSPACE_FONT().withHeight(9.0f));
			g.drawText(MidiMessage::getMidiNoteName(midiNoteNumber, false, true, 3), x, y, w, h - 3, Justification::centredBottom, true);
		}

		if (isOver)
		{
			g.setColour(findColour(MidiKeyboardComponent::ColourIds::mouseOverKeyOverlayColourId));
			g.fillRect(x, y, w, h);
		}

		if (state->isColourDefinedForKey(midiNoteNumber))
		{
			g.setColour(state->getColourForSingleKey(midiNoteNumber));
			g.fillRect(x, y, w, h);
		}
	}
}

void CustomKeyboardLookAndFeel::drawBlackNote(CustomKeyboardState* state, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour)
{

	if (useVectorGraphics)
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
	else
	{
		ignoreUnused(noteFillColour);

		const int offset[12] = { 0, 2, 0, 0, 0, 0, 4, 0, 2, 0, -1, 0 };

		const int off = midiNoteNumber % 12;


		g.setOpacity(1.0f);
		if (!isDown)
		{

			g.drawImage(cachedImage_black_key_on_png,
				x + offset[off], y, w, h - 4,
				0, 0, cachedImage_black_key_on_png.getWidth(), cachedImage_black_key_on_png.getHeight());


		}
		else
		{

			g.drawImage(cachedImage_black_key_off_png,
				x + offset[off], y, w, h - 4,
				0, 0, cachedImage_black_key_off_png.getWidth(), cachedImage_black_key_off_png.getHeight());

		}

		if (isOver)
		{
			g.setColour(Colours::red.withAlpha(0.1f));
			g.fillRect(x + offset[off], y, w - 2, h - 4);
		}

		if (state->isColourDefinedForKey(midiNoteNumber))
		{
			g.setColour(state->getColourForSingleKey(midiNoteNumber));
			g.fillRect(x + offset[off], y, w - 2, h - 4);
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

	setLookAndFeel(&laf);

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


void CustomKeyboard::paint(Graphics &g)
{
	if (!useCustomGraphics)
		dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel())->drawKeyboardBackground(g, getWidth(), getHeight());

	MidiKeyboardComponent::paint(g);

	//auto lf_ = dynamic_cast<CustomKeyboardLookAndFeel*>(&getLookAndFeel());
	//lf_->overlayColour = findColour(MidiKeyboardComponent::ColourIds::mouseOverKeyOverlayColourId);

	
}

void CustomKeyboard::mouseDown(const MouseEvent& e)
{
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
			jassertfalse;
			useCustomGraphics = false;
			break;
		}

		PoolReference downRef(mc, wc + "keyboard/down_" + String(i) + ".png", ProjectHandler::SubDirectories::Images);

		downImages.set(i, handler.loadImageReference(downRef, PoolHelpers::LoadAndCacheStrong));

		if (!downImages[i])
		{
			jassertfalse;
			useCustomGraphics = false;
			break;
		}
	}

	repaint();
}

void CustomKeyboard::setUseVectorGraphics(bool shouldUseVectorGraphics, bool useFlatStyle/*=false*/)
{
	laf.useVectorGraphics = shouldUseVectorGraphics;
	laf.useFlatStyle = useFlatStyle;

	if (useFlatStyle)
	{
		setColour(MidiKeyboardComponent::ColourIds::whiteNoteColourId, Colours::transparentBlack);
	}

	setOpaque(!useFlatStyle);
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
		dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel())->drawWhiteNote(state, midiNoteNumber, g, x, y, w, h, isDown, isOver, lineColour, textColour);
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
		dynamic_cast<CustomKeyboardLookAndFeelBase*>(&getLookAndFeel())->drawBlackNote(state, midiNoteNumber, g, x, y, w, h, isDown, isOver, noteFillColour);
	}
}



// JUCER_RESOURCE: black_key_off_png, 1340, "C:/Users/Chrisboy/Documents/black_key_off.png"
static const unsigned char resource_CustomKeyboard_black_key_off_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,19,0,0,0,92,8,6,0,0,0,140,94,142,117,0,0,0,25,116,69,88,116,83,111,102,116,
119,97,114,101,0,65,100,111,98,101,32,73,109,97,103,101,82,101,97,100,121,113,201,101,60,0,0,3,32,105,84,88,116,88,77,76,58,99,111,109,46,97,100,111,98,101,46,120,109,112,0,0,0,0,0,60,63,120,112,97,99,
107,101,116,32,98,101,103,105,110,61,34,239,187,191,34,32,105,100,61,34,87,53,77,48,77,112,67,101,104,105,72,122,114,101,83,122,78,84,99,122,107,99,57,100,34,63,62,32,60,120,58,120,109,112,109,101,116,
97,32,120,109,108,110,115,58,120,61,34,97,100,111,98,101,58,110,115,58,109,101,116,97,47,34,32,120,58,120,109,112,116,107,61,34,65,100,111,98,101,32,88,77,80,32,67,111,114,101,32,53,46,48,45,99,48,54,
48,32,54,49,46,49,51,52,55,55,55,44,32,50,48,49,48,47,48,50,47,49,50,45,49,55,58,51,50,58,48,48,32,32,32,32,32,32,32,32,34,62,32,60,114,100,102,58,82,68,70,32,120,109,108,110,115,58,114,100,102,61,34,
104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,49,57,57,57,47,48,50,47,50,50,45,114,100,102,45,115,121,110,116,97,120,45,110,115,35,34,62,32,60,114,100,102,58,68,101,115,99,114,105,112,
116,105,111,110,32,114,100,102,58,97,98,111,117,116,61,34,34,32,120,109,108,110,115,58,120,109,112,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,
34,32,120,109,108,110,115,58,120,109,112,77,77,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,109,109,47,34,32,120,109,108,110,115,58,115,116,82,
101,102,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,115,84,121,112,101,47,82,101,115,111,117,114,99,101,82,101,102,35,34,32,120,109,112,58,67,
114,101,97,116,111,114,84,111,111,108,61,34,65,100,111,98,101,32,80,104,111,116,111,115,104,111,112,32,67,83,53,32,87,105,110,100,111,119,115,34,32,120,109,112,77,77,58,73,110,115,116,97,110,99,101,73,
68,61,34,120,109,112,46,105,105,100,58,49,57,70,54,54,52,56,55,48,70,70,48,49,49,69,51,66,48,69,65,65,48,65,68,69,57,48,68,52,53,50,66,34,32,120,109,112,77,77,58,68,111,99,117,109,101,110,116,73,68,61,
34,120,109,112,46,100,105,100,58,49,57,70,54,54,52,56,56,48,70,70,48,49,49,69,51,66,48,69,65,65,48,65,68,69,57,48,68,52,53,50,66,34,62,32,60,120,109,112,77,77,58,68,101,114,105,118,101,100,70,114,111,
109,32,115,116,82,101,102,58,105,110,115,116,97,110,99,101,73,68,61,34,120,109,112,46,105,105,100,58,49,57,70,54,54,52,56,53,48,70,70,48,49,49,69,51,66,48,69,65,65,48,65,68,69,57,48,68,52,53,50,66,34,
32,115,116,82,101,102,58,100,111,99,117,109,101,110,116,73,68,61,34,120,109,112,46,100,105,100,58,49,57,70,54,54,52,56,54,48,70,70,48,49,49,69,51,66,48,69,65,65,48,65,68,69,57,48,68,52,53,50,66,34,47,
62,32,60,47,114,100,102,58,68,101,115,99,114,105,112,116,105,111,110,62,32,60,47,114,100,102,58,82,68,70,62,32,60,47,120,58,120,109,112,109,101,116,97,62,32,60,63,120,112,97,99,107,101,116,32,101,110,
100,61,34,114,34,63,62,50,18,92,212,0,0,1,178,73,68,65,84,120,218,236,153,205,74,195,64,16,199,103,55,105,211,75,90,16,164,57,228,162,23,65,240,25,188,250,20,62,128,79,163,39,81,232,201,231,208,94,188,
122,18,4,237,193,75,143,61,88,10,109,104,243,225,108,233,134,77,48,155,9,230,160,101,6,134,108,58,147,31,243,223,217,108,66,42,130,225,240,112,48,24,92,247,251,253,11,207,243,14,132,16,96,51,199,113,160,
211,233,188,225,240,38,142,227,251,167,241,56,143,185,179,217,236,17,1,103,189,94,15,186,221,46,212,193,148,37,73,114,154,166,233,93,20,69,9,158,142,244,239,234,210,76,13,194,48,132,32,8,64,65,109,38,
165,132,205,102,3,139,197,2,16,246,249,49,153,28,231,149,233,193,116,58,221,38,161,220,90,153,235,245,26,150,203,37,160,204,35,51,150,87,70,53,223,247,11,83,241,53,159,231,39,18,90,52,134,49,140,97,12,
99,24,195,24,198,48,134,49,140,97,12,99,24,195,24,182,135,48,23,8,95,88,160,252,241,162,226,26,183,33,10,196,206,127,134,201,102,74,85,190,168,172,172,161,76,149,95,9,51,147,126,13,147,134,76,10,80,218,
100,74,35,64,130,217,42,19,13,43,179,46,141,86,101,234,128,109,98,233,115,182,171,172,85,152,78,108,13,70,169,142,54,103,84,24,101,209,106,80,45,204,113,90,132,53,233,38,5,86,185,57,150,187,89,7,179,222,
232,230,162,165,192,200,50,247,24,86,190,209,255,78,3,90,173,172,233,162,141,227,24,92,215,221,254,201,80,249,116,202,178,12,210,52,173,221,130,162,40,218,74,85,142,192,164,0,83,16,229,26,72,217,207,140,
252,215,66,12,171,121,208,65,83,106,149,27,42,86,120,188,42,55,224,18,143,207,232,231,56,62,33,60,135,87,232,47,8,186,69,127,47,52,135,34,237,255,191,236,125,11,48,0,180,52,119,18,1,213,108,244,0,0,0,
0,73,69,78,68,174,66,96,130,0,0};

const char* CustomKeyboardLookAndFeel::black_key_off_png = (const char*) resource_CustomKeyboard_black_key_off_png;
const int CustomKeyboardLookAndFeel::black_key_off_pngSize = 1340;

// JUCER_RESOURCE: black_key_on_png, 1422, "C:/Users/Chrisboy/Documents/black_key_on.png"
static const unsigned char resource_CustomKeyboard_black_key_on_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,20,0,0,0,92,8,6,0,0,0,110,130,149,12,0,0,0,25,116,69,88,116,83,111,102,116,
119,97,114,101,0,65,100,111,98,101,32,73,109,97,103,101,82,101,97,100,121,113,201,101,60,0,0,3,32,105,84,88,116,88,77,76,58,99,111,109,46,97,100,111,98,101,46,120,109,112,0,0,0,0,0,60,63,120,112,97,99,
107,101,116,32,98,101,103,105,110,61,34,239,187,191,34,32,105,100,61,34,87,53,77,48,77,112,67,101,104,105,72,122,114,101,83,122,78,84,99,122,107,99,57,100,34,63,62,32,60,120,58,120,109,112,109,101,116,
97,32,120,109,108,110,115,58,120,61,34,97,100,111,98,101,58,110,115,58,109,101,116,97,47,34,32,120,58,120,109,112,116,107,61,34,65,100,111,98,101,32,88,77,80,32,67,111,114,101,32,53,46,48,45,99,48,54,
48,32,54,49,46,49,51,52,55,55,55,44,32,50,48,49,48,47,48,50,47,49,50,45,49,55,58,51,50,58,48,48,32,32,32,32,32,32,32,32,34,62,32,60,114,100,102,58,82,68,70,32,120,109,108,110,115,58,114,100,102,61,34,
104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,49,57,57,57,47,48,50,47,50,50,45,114,100,102,45,115,121,110,116,97,120,45,110,115,35,34,62,32,60,114,100,102,58,68,101,115,99,114,105,112,
116,105,111,110,32,114,100,102,58,97,98,111,117,116,61,34,34,32,120,109,108,110,115,58,120,109,112,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,
34,32,120,109,108,110,115,58,120,109,112,77,77,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,109,109,47,34,32,120,109,108,110,115,58,115,116,82,
101,102,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,115,84,121,112,101,47,82,101,115,111,117,114,99,101,82,101,102,35,34,32,120,109,112,58,67,
114,101,97,116,111,114,84,111,111,108,61,34,65,100,111,98,101,32,80,104,111,116,111,115,104,111,112,32,67,83,53,32,87,105,110,100,111,119,115,34,32,120,109,112,77,77,58,73,110,115,116,97,110,99,101,73,
68,61,34,120,109,112,46,105,105,100,58,70,70,68,65,48,67,69,55,48,70,69,70,49,49,69,51,66,70,67,67,68,54,66,52,69,56,49,56,65,51,54,53,34,32,120,109,112,77,77,58,68,111,99,117,109,101,110,116,73,68,61,
34,120,109,112,46,100,105,100,58,70,70,68,65,48,67,69,56,48,70,69,70,49,49,69,51,66,70,67,67,68,54,66,52,69,56,49,56,65,51,54,53,34,62,32,60,120,109,112,77,77,58,68,101,114,105,118,101,100,70,114,111,
109,32,115,116,82,101,102,58,105,110,115,116,97,110,99,101,73,68,61,34,120,109,112,46,105,105,100,58,70,70,68,65,48,67,69,53,48,70,69,70,49,49,69,51,66,70,67,67,68,54,66,52,69,56,49,56,65,51,54,53,34,
32,115,116,82,101,102,58,100,111,99,117,109,101,110,116,73,68,61,34,120,109,112,46,100,105,100,58,70,70,68,65,48,67,69,54,48,70,69,70,49,49,69,51,66,70,67,67,68,54,66,52,69,56,49,56,65,51,54,53,34,47,
62,32,60,47,114,100,102,58,68,101,115,99,114,105,112,116,105,111,110,62,32,60,47,114,100,102,58,82,68,70,62,32,60,47,120,58,120,109,112,109,101,116,97,62,32,60,63,120,112,97,99,107,101,116,32,101,110,
100,61,34,114,34,63,62,93,22,176,165,0,0,2,4,73,68,65,84,120,218,236,153,65,75,27,65,20,199,223,204,108,178,27,146,8,137,72,18,68,81,60,169,72,232,201,155,218,91,63,67,191,132,103,47,253,12,245,216,126,
137,126,2,65,241,234,181,129,150,32,57,40,9,45,197,168,73,150,133,204,76,223,91,119,202,22,113,87,104,16,173,239,193,159,221,125,243,242,227,255,102,150,28,246,137,118,187,189,160,181,254,136,122,103,
140,169,67,78,88,107,193,26,211,177,0,135,231,221,238,103,151,215,152,167,144,133,66,225,200,83,234,189,148,178,46,32,63,132,16,128,181,27,82,136,79,171,107,107,251,152,242,210,235,210,247,253,45,63,8,
160,88,44,130,84,234,81,64,133,117,84,143,191,61,192,212,98,26,234,97,196,5,232,148,10,96,58,157,102,183,108,76,92,35,164,164,199,6,234,13,106,132,250,21,3,229,221,2,148,74,37,162,195,100,50,201,4,70,
81,4,54,12,211,169,77,212,87,7,148,240,239,81,35,63,127,246,112,6,64,149,230,204,2,248,87,48,144,129,12,100,32,3,25,200,64,6,50,144,129,12,100,32,3,25,200,64,6,190,8,160,103,147,143,96,241,71,178,68,89,
17,215,60,127,135,25,53,236,48,223,161,171,125,222,123,248,164,135,242,10,223,195,153,31,202,127,218,50,159,242,11,251,251,154,253,30,186,27,55,160,113,243,149,135,98,56,28,198,83,159,135,160,94,181,90,
133,114,185,12,141,70,3,130,32,200,157,248,244,122,61,232,247,251,160,181,6,99,204,61,170,215,106,181,160,86,171,65,165,82,137,139,178,128,228,138,234,71,163,17,140,199,99,8,195,112,124,239,148,149,82,
39,228,50,171,141,116,80,23,205,102,147,182,199,252,24,12,190,97,138,28,104,183,46,112,199,118,118,247,246,62,212,231,231,55,43,216,187,54,38,211,97,20,69,230,242,226,226,182,211,233,92,222,222,220,12,
48,125,138,250,162,173,61,119,192,21,188,190,69,109,163,22,80,62,229,115,140,146,35,106,183,139,58,70,157,33,240,218,1,9,176,140,90,71,45,161,230,224,110,206,148,21,17,234,39,234,123,162,43,4,106,7,116,
175,79,53,81,158,67,155,56,164,89,220,117,114,181,110,148,249,91,128,1,0,181,142,155,127,61,200,145,56,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* CustomKeyboardLookAndFeel::black_key_on_png = (const char*) resource_CustomKeyboard_black_key_on_png;
const int CustomKeyboardLookAndFeel::black_key_on_pngSize = 1422;

// JUCER_RESOURCE: white_key_off_png, 2028, "C:/Users/Chrisboy/Documents/white_key_off.png"
static const unsigned char resource_CustomKeyboard_white_key_off_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,27,0,0,0,150,8,6,0,0,0,47,250,243,83,0,0,0,25,116,69,88,116,83,111,102,116,
119,97,114,101,0,65,100,111,98,101,32,73,109,97,103,101,82,101,97,100,121,113,201,101,60,0,0,3,32,105,84,88,116,88,77,76,58,99,111,109,46,97,100,111,98,101,46,120,109,112,0,0,0,0,0,60,63,120,112,97,99,
107,101,116,32,98,101,103,105,110,61,34,239,187,191,34,32,105,100,61,34,87,53,77,48,77,112,67,101,104,105,72,122,114,101,83,122,78,84,99,122,107,99,57,100,34,63,62,32,60,120,58,120,109,112,109,101,116,
97,32,120,109,108,110,115,58,120,61,34,97,100,111,98,101,58,110,115,58,109,101,116,97,47,34,32,120,58,120,109,112,116,107,61,34,65,100,111,98,101,32,88,77,80,32,67,111,114,101,32,53,46,48,45,99,48,54,
48,32,54,49,46,49,51,52,55,55,55,44,32,50,48,49,48,47,48,50,47,49,50,45,49,55,58,51,50,58,48,48,32,32,32,32,32,32,32,32,34,62,32,60,114,100,102,58,82,68,70,32,120,109,108,110,115,58,114,100,102,61,34,
104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,49,57,57,57,47,48,50,47,50,50,45,114,100,102,45,115,121,110,116,97,120,45,110,115,35,34,62,32,60,114,100,102,58,68,101,115,99,114,105,112,
116,105,111,110,32,114,100,102,58,97,98,111,117,116,61,34,34,32,120,109,108,110,115,58,120,109,112,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,
34,32,120,109,108,110,115,58,120,109,112,77,77,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,109,109,47,34,32,120,109,108,110,115,58,115,116,82,
101,102,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,115,84,121,112,101,47,82,101,115,111,117,114,99,101,82,101,102,35,34,32,120,109,112,58,67,
114,101,97,116,111,114,84,111,111,108,61,34,65,100,111,98,101,32,80,104,111,116,111,115,104,111,112,32,67,83,53,32,87,105,110,100,111,119,115,34,32,120,109,112,77,77,58,73,110,115,116,97,110,99,101,73,
68,61,34,120,109,112,46,105,105,100,58,52,53,54,53,70,52,65,56,48,70,70,48,49,49,69,51,56,54,54,65,68,52,49,54,53,54,55,51,51,48,68,49,34,32,120,109,112,77,77,58,68,111,99,117,109,101,110,116,73,68,61,
34,120,109,112,46,100,105,100,58,52,53,54,53,70,52,65,57,48,70,70,48,49,49,69,51,56,54,54,65,68,52,49,54,53,54,55,51,51,48,68,49,34,62,32,60,120,109,112,77,77,58,68,101,114,105,118,101,100,70,114,111,
109,32,115,116,82,101,102,58,105,110,115,116,97,110,99,101,73,68,61,34,120,109,112,46,105,105,100,58,52,53,54,53,70,52,65,54,48,70,70,48,49,49,69,51,56,54,54,65,68,52,49,54,53,54,55,51,51,48,68,49,34,
32,115,116,82,101,102,58,100,111,99,117,109,101,110,116,73,68,61,34,120,109,112,46,100,105,100,58,52,53,54,53,70,52,65,55,48,70,70,48,49,49,69,51,56,54,54,65,68,52,49,54,53,54,55,51,51,48,68,49,34,47,
62,32,60,47,114,100,102,58,68,101,115,99,114,105,112,116,105,111,110,62,32,60,47,114,100,102,58,82,68,70,62,32,60,47,120,58,120,109,112,109,101,116,97,62,32,60,63,120,112,97,99,107,101,116,32,101,110,
100,61,34,114,34,63,62,89,113,113,231,0,0,4,98,73,68,65,84,120,218,228,91,205,78,27,49,16,254,178,113,8,73,128,34,129,68,203,161,247,222,242,0,168,135,246,216,71,40,239,211,135,160,143,208,75,95,2,30,
128,75,79,180,220,8,160,36,64,40,40,36,76,109,107,67,189,142,247,39,107,123,151,77,44,141,162,108,178,30,207,248,155,241,204,216,70,0,208,199,131,3,250,125,118,70,211,233,148,92,183,199,199,71,250,126,
116,68,156,207,55,6,222,62,125,254,140,63,231,231,120,179,189,141,233,100,130,90,16,32,8,169,86,171,33,224,4,65,11,54,241,238,223,251,123,116,187,93,124,61,60,252,192,214,215,215,209,235,245,112,114,124,
140,235,171,43,60,60,60,160,221,110,99,115,115,83,82,171,213,194,90,179,9,198,152,124,121,209,54,30,143,113,122,122,138,6,99,187,172,209,104,160,223,239,99,107,107,11,183,183,183,178,243,38,239,188,211,
233,96,99,99,3,109,254,41,6,36,254,151,167,13,135,67,240,233,193,100,50,169,51,174,214,151,31,132,218,132,4,107,107,107,146,65,139,75,216,209,152,45,42,157,144,76,244,23,212,235,96,154,146,229,195,58,
103,40,152,10,6,226,143,130,242,74,198,213,39,251,147,130,168,147,57,35,241,67,93,48,213,40,79,19,131,151,96,19,253,26,81,164,48,207,163,186,88,198,38,184,38,125,183,105,82,141,2,36,18,40,33,145,242,76,
165,60,141,148,62,3,66,113,141,105,195,248,79,250,247,156,146,169,239,70,152,145,70,48,124,95,152,151,242,110,80,160,22,139,101,198,76,98,195,128,68,178,152,179,25,186,151,92,141,164,25,246,139,234,52,
227,182,54,234,226,1,162,142,58,206,160,201,194,215,132,239,39,26,53,57,54,106,102,114,43,70,169,108,220,149,206,140,12,35,161,42,187,171,136,100,38,248,195,17,244,87,192,17,171,86,142,24,239,97,163,198,
25,49,21,137,70,70,234,92,230,101,182,34,139,103,168,34,74,8,227,172,189,126,209,139,39,139,140,64,241,139,46,37,163,50,140,218,232,174,34,161,184,11,201,94,133,7,33,131,148,75,128,70,117,158,76,243,88,
73,52,34,46,118,116,132,198,114,230,140,212,17,120,242,141,229,122,125,31,9,60,74,69,35,169,235,89,2,34,115,207,89,136,5,102,10,147,157,134,223,166,196,130,82,66,111,183,137,133,207,58,72,248,201,146,
70,227,82,50,50,166,76,174,147,65,189,194,67,25,41,175,100,115,70,77,41,217,76,133,151,152,20,131,174,102,192,19,25,129,42,157,203,162,203,139,157,233,112,247,13,253,44,181,16,119,70,173,75,232,219,93,
21,86,225,33,165,176,137,165,137,65,146,36,82,159,219,186,43,102,244,248,174,161,175,3,132,60,3,132,98,139,155,113,146,186,54,106,56,46,1,198,122,253,84,178,145,172,200,232,106,206,93,145,150,100,56,89,
169,21,42,113,47,198,227,206,32,34,235,89,28,32,170,22,202,25,195,111,213,243,47,87,192,19,241,240,62,42,60,133,111,29,39,21,161,93,134,223,209,136,88,47,83,84,191,232,162,129,194,71,192,83,174,111,76,
146,170,218,37,64,0,126,160,31,137,174,210,98,124,223,94,63,46,92,176,79,224,179,72,229,50,224,201,146,167,193,66,58,74,12,229,226,138,49,121,195,111,36,236,83,251,95,169,83,234,196,185,1,82,154,187,82,
215,177,229,115,87,115,5,23,111,238,42,75,193,197,135,81,251,56,162,129,196,204,179,168,114,18,138,50,106,175,213,111,32,189,156,4,167,153,103,214,26,136,175,197,211,117,78,141,212,57,51,205,157,173,157,
81,198,133,211,207,33,47,31,118,150,5,141,128,175,218,149,111,52,170,7,134,200,179,111,76,71,163,143,210,68,154,174,253,149,218,125,250,198,90,6,251,90,130,35,135,112,127,210,5,153,11,101,85,221,248,41,
49,205,245,165,70,165,127,134,12,25,167,43,163,126,5,199,177,125,162,49,14,61,126,34,226,172,107,154,181,100,225,141,158,194,11,210,169,217,140,141,215,79,90,169,151,196,131,20,233,136,179,214,66,108,
161,191,42,215,139,124,222,177,48,29,133,162,132,249,243,99,212,113,82,186,66,99,17,155,226,171,128,198,34,60,136,8,62,159,159,159,163,133,77,204,239,89,231,157,179,41,239,91,144,224,17,60,61,61,97,52,
26,97,48,24,200,43,206,179,74,154,122,6,132,44,34,171,155,225,80,94,115,30,221,221,77,197,202,73,111,247,246,228,173,98,113,53,246,221,254,190,188,71,221,12,111,27,51,254,156,213,235,242,246,120,109,65,
137,6,253,62,122,151,151,242,123,239,226,226,167,156,179,27,46,209,238,206,206,236,33,174,181,219,198,234,165,224,60,77,104,142,51,253,37,152,213,196,85,116,222,186,156,190,112,149,189,87,147,9,161,107,
11,102,99,78,39,237,118,251,7,239,103,252,79,128,1,0,187,116,90,111,116,168,228,14,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* CustomKeyboardLookAndFeel::white_key_off_png = (const char*) resource_CustomKeyboard_white_key_off_png;
const int CustomKeyboardLookAndFeel::white_key_off_pngSize = 2028;

// JUCER_RESOURCE: white_key_on_png, 1611, "C:/Users/Chrisboy/Documents/white_key_on.png"
static const unsigned char resource_CustomKeyboard_white_key_on_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,27,0,0,0,151,8,6,0,0,0,228,166,32,246,0,0,0,25,116,69,88,116,83,111,102,116,
119,97,114,101,0,65,100,111,98,101,32,73,109,97,103,101,82,101,97,100,121,113,201,101,60,0,0,3,32,105,84,88,116,88,77,76,58,99,111,109,46,97,100,111,98,101,46,120,109,112,0,0,0,0,0,60,63,120,112,97,99,
107,101,116,32,98,101,103,105,110,61,34,239,187,191,34,32,105,100,61,34,87,53,77,48,77,112,67,101,104,105,72,122,114,101,83,122,78,84,99,122,107,99,57,100,34,63,62,32,60,120,58,120,109,112,109,101,116,
97,32,120,109,108,110,115,58,120,61,34,97,100,111,98,101,58,110,115,58,109,101,116,97,47,34,32,120,58,120,109,112,116,107,61,34,65,100,111,98,101,32,88,77,80,32,67,111,114,101,32,53,46,48,45,99,48,54,
48,32,54,49,46,49,51,52,55,55,55,44,32,50,48,49,48,47,48,50,47,49,50,45,49,55,58,51,50,58,48,48,32,32,32,32,32,32,32,32,34,62,32,60,114,100,102,58,82,68,70,32,120,109,108,110,115,58,114,100,102,61,34,
104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,49,57,57,57,47,48,50,47,50,50,45,114,100,102,45,115,121,110,116,97,120,45,110,115,35,34,62,32,60,114,100,102,58,68,101,115,99,114,105,112,
116,105,111,110,32,114,100,102,58,97,98,111,117,116,61,34,34,32,120,109,108,110,115,58,120,109,112,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,
34,32,120,109,108,110,115,58,120,109,112,77,77,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,109,109,47,34,32,120,109,108,110,115,58,115,116,82,
101,102,61,34,104,116,116,112,58,47,47,110,115,46,97,100,111,98,101,46,99,111,109,47,120,97,112,47,49,46,48,47,115,84,121,112,101,47,82,101,115,111,117,114,99,101,82,101,102,35,34,32,120,109,112,58,67,
114,101,97,116,111,114,84,111,111,108,61,34,65,100,111,98,101,32,80,104,111,116,111,115,104,111,112,32,67,83,53,32,87,105,110,100,111,119,115,34,32,120,109,112,77,77,58,73,110,115,116,97,110,99,101,73,
68,61,34,120,109,112,46,105,105,100,58,51,54,67,50,67,66,48,65,48,70,70,48,49,49,69,51,56,55,66,57,56,69,68,69,56,67,48,54,66,70,50,56,34,32,120,109,112,77,77,58,68,111,99,117,109,101,110,116,73,68,61,
34,120,109,112,46,100,105,100,58,51,54,67,50,67,66,48,66,48,70,70,48,49,49,69,51,56,55,66,57,56,69,68,69,56,67,48,54,66,70,50,56,34,62,32,60,120,109,112,77,77,58,68,101,114,105,118,101,100,70,114,111,
109,32,115,116,82,101,102,58,105,110,115,116,97,110,99,101,73,68,61,34,120,109,112,46,105,105,100,58,51,54,67,50,67,66,48,56,48,70,70,48,49,49,69,51,56,55,66,57,56,69,68,69,56,67,48,54,66,70,50,56,34,
32,115,116,82,101,102,58,100,111,99,117,109,101,110,116,73,68,61,34,120,109,112,46,100,105,100,58,51,54,67,50,67,66,48,57,48,70,70,48,49,49,69,51,56,55,66,57,56,69,68,69,56,67,48,54,66,70,50,56,34,47,
62,32,60,47,114,100,102,58,68,101,115,99,114,105,112,116,105,111,110,62,32,60,47,114,100,102,58,82,68,70,62,32,60,47,120,58,120,109,112,109,101,116,97,62,32,60,63,120,112,97,99,107,101,116,32,101,110,
100,61,34,114,34,63,62,123,17,9,24,0,0,2,193,73,68,65,84,120,218,236,91,187,110,26,65,20,61,3,179,44,143,165,216,34,18,84,212,40,77,148,38,80,58,181,211,165,140,75,164,248,7,226,252,65,252,41,142,112,
58,187,192,249,9,132,34,129,28,217,18,152,32,167,137,64,222,136,21,34,59,153,217,44,129,88,150,229,98,102,172,37,119,164,17,104,139,61,123,239,61,247,236,153,129,97,25,224,153,235,186,141,15,135,135,207,
125,223,47,64,227,16,66,160,211,233,92,126,60,58,250,22,95,144,96,239,63,29,31,139,229,114,41,116,143,40,138,68,16,4,226,213,238,174,144,56,130,191,221,223,127,218,104,54,17,134,33,114,185,92,252,52,186,
6,99,12,139,197,2,239,14,14,112,122,114,2,62,15,195,39,159,207,206,240,162,209,128,231,121,58,179,8,206,57,198,227,49,46,47,46,80,40,22,145,249,25,4,144,161,34,156,207,193,212,211,104,156,191,150,75,124,
191,190,198,100,50,129,227,56,224,73,188,235,169,121,136,132,40,9,63,236,13,2,35,176,251,251,14,183,122,67,247,88,221,87,172,192,54,244,197,0,218,186,127,137,32,4,118,63,245,153,5,234,175,251,108,243,
245,98,130,250,68,144,244,11,241,93,204,209,45,196,255,130,153,98,35,9,49,217,130,135,131,153,214,70,98,227,22,104,163,44,32,75,166,126,126,176,191,44,167,154,165,217,202,153,36,8,41,72,234,86,158,100,
191,83,222,212,198,85,63,81,126,170,89,90,125,163,233,85,76,242,73,4,209,72,16,11,77,77,66,172,89,136,173,189,169,55,27,218,212,155,122,235,9,98,109,95,127,93,179,219,162,105,66,136,169,169,181,190,60,
77,10,49,72,136,205,236,131,220,33,154,233,20,98,106,106,218,7,161,154,113,102,88,136,217,127,97,120,30,71,136,237,185,43,18,226,244,185,43,131,66,76,238,42,157,75,38,114,87,219,176,15,66,238,138,192,
30,99,77,205,168,102,91,160,32,27,214,128,20,132,192,172,251,70,208,111,158,4,246,112,71,188,98,204,86,253,239,42,195,152,21,160,172,227,72,176,108,214,10,88,78,129,101,45,129,229,243,121,100,184,69,48,
174,206,136,193,176,187,82,247,45,20,10,50,141,156,91,137,44,62,236,198,45,129,149,20,152,99,9,172,104,51,50,175,84,146,145,73,254,91,1,43,151,45,214,76,70,198,85,100,166,247,245,99,48,207,179,152,70,
85,51,91,105,44,171,154,217,138,172,20,71,102,11,44,174,153,173,62,179,217,212,69,149,70,107,66,44,85,159,171,52,170,211,193,166,250,44,18,2,81,20,253,209,198,121,24,98,58,157,98,54,155,105,7,26,141,70,
248,122,126,142,171,171,171,24,140,7,55,55,63,218,237,54,212,172,213,106,168,84,42,208,97,21,212,25,237,94,175,23,127,175,86,171,240,125,31,252,205,222,222,151,193,96,128,110,183,139,225,112,24,79,189,
246,45,131,86,171,5,215,117,193,212,193,234,151,59,59,57,121,253,181,156,77,101,132,116,102,82,206,211,122,189,222,237,247,251,248,45,192,0,66,57,226,65,2,160,50,223,0,0,0,0,73,69,78,68,174,66,96,130,
0,0};

const char* CustomKeyboardLookAndFeel::white_key_on_png = (const char*) resource_CustomKeyboard_white_key_on_png;
const int CustomKeyboardLookAndFeel::white_key_on_pngSize = 1611;



} // namespace hise