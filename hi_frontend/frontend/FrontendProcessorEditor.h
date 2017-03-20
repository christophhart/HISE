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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef FRONTENDPROCESSOREDITOR_H_INCLUDED
#define FRONTENDPROCESSOREDITOR_H_INCLUDED

#define INCLUDE_BAR 1


class ScriptContentContainer;

class FrontendEditorHolder: public Component
{
    
};

class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer,
							   public ComponentWithKeyboard,
							   public ModalBaseWindow,
							   public OverlayMessageBroadcaster::Listener


{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	~FrontendProcessorEditor();

	void overlayMessageSent(int state, const String& message) override
	{
		if (deactiveOverlay != nullptr)
		{
			deactiveOverlay->setCustomMessage(message);
			deactiveOverlay->setState((DeactiveOverlay::State)state, true);
		}
	}

	void timerCallback()
	{
#if USE_COPY_PROTECTION
		if (!dynamic_cast<FrontendProcessor*>(getAudioProcessor())->unlocker.isUnlocked())
		{
			getAudioProcessor()->suspendProcessing(true);
		}
#endif
	}

	KeyboardFocusTraverser *createFocusTraverser() override { return (keyboard != nullptr && keyboard->isVisible()) ? new MidiKeyboardFocusTraverser() : new KeyboardFocusTraverser(); };

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
	};

#if CRASH_ON_GLITCH
	void paintOverChildren(Graphics& g) override
	{
		
		
		Font f = GLOBAL_BOLD_FONT().withHeight(32.0f);
		const String s = "Debug Version. Don't use in production!";

		g.setFont(f);
		

		int w = f.getStringWidth(s) + 30;
		int h = (int)(f.getHeight() + 10.0f);

		Rectangle<int> a(0, 0, w, h);

		a.setCentre(getLocalBounds().getCentre());

		g.setColour(Colour(0x88FFFFFF));

		g.fillRect(a);

		g.setColour(Colour(0x99000000));

		g.drawText(s, a, Justification::centred);
	}
#endif

    void setGlobalScaleFactor(float newScaleFactor);

	void resized() override;

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	Component *getKeyboard() const override { return keyboard; }

private:

    ScopedPointer<FrontendEditorHolder> container;
    
	friend class BaseFrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;
	ScopedPointer<BaseFrontendBar> mainBar;
	ScopedPointer<CustomKeyboard> keyboard;
	ScopedPointer<AboutPage> aboutPage;
	ScopedPointer<DeactiveOverlay> deactiveOverlay;
	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> loaderOverlay;
    
	float scaleFactor = 1.0f;

    int originalSizeX = 0;
    int originalSizeY = 0;

    bool overlayToolbar;
};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
