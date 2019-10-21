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

#ifndef __FRONTENDBAR_H_INCLUDED
#define __FRONTENDBAR_H_INCLUDED

namespace hise { using namespace juce;



class SampleDataImporter;

/** A overlay that can show critical error messages on the compiled plugin.
	@ingroup hise_ui

	If your plugin's state encounters a error that either needs the full attention of the user
	or even completely deactivates the UI functionality, this class will appear and show the 
	error message that was received.
*/
class DeactiveOverlay : public Component,
	public ButtonListener,
	public ControlledObject,
	public Timer,
	public AsyncUpdater
{
public:

	DeactiveOverlay(MainController* mc);;

	void buttonClicked(Button *b);

	enum State
	{
		AppDataDirectoryNotFound,
		LicenseNotFound,
		ProductNotMatching,
		UserNameNotMatching,
		EmailNotMatching,
		MachineNumbersNotMatching,
		LicenseExpired,
		LicenseInvalid,
		CriticalCustomErrorMessage,
		SamplesNotInstalled,
		SamplesNotFound,
		CustomErrorMessage,
		CustomInformation,
		numReasons
	};

	void paint(Graphics &g)
	{
		g.fillAll();
		g.drawImageAt(img, 0, 0);
		g.setColour(Colours::black.withAlpha(0.66f * (float)(numFramesLeft) / 10.0f));
		g.fillAll();
	}

	void timerCallback() override
	{
		numFramesLeft -= 1;

		if (numFramesLeft <= 0)
		{
			stopTimer();
			setVisible(false);
			originalImage = Image();
			return;
		}

		triggerAsyncUpdate();
	}

	void fadeout()
	{
		numFramesLeft = 10;
		startTimer(30);
	}

	void handleAsyncUpdate()
	{
		rebuildImage();
		repaint();
	}

	void rebuildImage();

	void setState(State s, bool value)
	{
		
		bool wasVisible = currentState != 0;

#if !HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON && !HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		if (s == SamplesNotInstalled && value)
			return;
#endif

		currentState.setBit(s, value);

		if (!wasVisible && currentState != 0)
		{
			numFramesLeft = 10;
			setVisible(true);
			refreshLabel();
			resized();
		}

		if (wasVisible && currentState == 0)
		{
			refreshLabel();
			fadeout();
			resized();
		}

		if (wasVisible && currentState != 0)
		{
			refreshLabel();
			resized();
		}

		if (!wasVisible && currentState == 0)
		{
			setVisible(false);
			refreshLabel();
			resized();
		}
	}

    void clearAllFlags()
    {
        currentState = 0;
    }
    
	void setCustomMessage(const String newCustomMessage)
	{
		customMessage = newCustomMessage;
	}

	bool check(State s, const String &value = String());

	State checkLicense(const String &keyContent = String());

	void refreshLabel()
	{
		if (currentState == 0)
		{
			descriptionLabel->setText("", dontSendNotification);
		}

		for (int i = 0; i < numReasons; i++)
		{
			if (currentState[i])
			{
				descriptionLabel->setText(getTextForError((State)i), dontSendNotification);
				return;
			}
		}

		resized();
	}

	String getTextForError(State s) const;

	void resized();

private:

	int numFramesLeft = 0;
	

	PostGraphicsRenderer::DataStack stack;

	Image originalImage;
	Image img;

	ScopedPointer<LookAndFeel> alaf;
	String customMessage;

	ScopedPointer<Label> descriptionLabel;

	ScopedPointer<TextButton> resolveLicenseButton;
	ScopedPointer<TextButton> installSampleButton;
	ScopedPointer<TextButton> resolveSamplesButton;
	ScopedPointer<TextButton> registerProductButton;
	ScopedPointer<TextButton> ignoreButton;

	BigInteger currentState;

};

} // namespace hise

#endif  

