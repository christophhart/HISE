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
	public AsyncUpdater,
	public OverlayMessageBroadcaster::Listener
{
public:

	DeactiveOverlay(MainController* mc);;
	~DeactiveOverlay();

	void buttonClicked(Button *b);

	using State = OverlayMessageBroadcaster::State;

	void paint(Graphics &g) override;
	void timerCallback() override;
	void fadeout();
	void handleAsyncUpdate() override;
	void rebuildImage();
	void overlayMessageSent(int state, const String& message) override;

	void checkVisibility()
	{
		setVisible(currentState != 0);
	}
	
    void clearAllFlags()
    {
        currentState = 0;
    }
    
	bool check(State s, const String &value = String());

#if !USE_SCRIPT_COPY_PROTECTION
	State checkLicense(const String &keyContent = String());
#endif

	void refreshLabel();
	String getTextForError(State s) const;
	void resized();
	void clearState(State s);

private:

	void setStateInternal(State s, bool value);


	String customMessage;

	int numFramesLeft = 0;
	

	PostGraphicsRenderer::DataStack stack;

	Image originalImage;
	Image img;

	ScopedPointer<LookAndFeel> alaf;
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

