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

#ifndef __CONSOLE_H_8FBBF4D7__
#define __CONSOLE_H_8FBBF4D7__

class Processor;
class BaseDebugArea;

/** A textbox that acts as console (read only / autoscrolling)
*	@ingroup debugComponents
*    
*   The console has a clear button that deletes the content.*
*	Use the (addLine) method to write something to the console.
*    
*	For Modulators there is the macro function debugMod(String &t)
*/
class Console: public Component,
			   public ButtonListener,
			   public AsyncUpdater,
			   public Timer,
			   public AutoPopupDebugComponent
{
public:

	enum WarningLevel
	{
		Message = 0,
		Error = 1
	};

	Console(BaseDebugArea *area);

	void timerCallback()
	{
		cpuSlider->setValue(usage, dontSendNotification);

		voiceLabel->setText("Voices: " + String(voiceAmount) + ", Tempo: " + String(hostTempo) + "BPM", dontSendNotification);

		if(usage != 0)
		{
			ScopedLock sl(lock);
			usage = 0;
		}

		unprintedMessages.ensureStorageAllocated(100);

	};

	~Console();

	void sendChangeMessage();
    
    void mouseDown(const MouseEvent &e) override;

    void mouseMove(const MouseEvent &e) override
    {
        if(e.mods.isAltDown())
        {
            setMouseCursor(MouseCursor::PointingHandCursor);
        }
    }
    
	void resized() override;

	void buttonClicked (Button* buttonThatWasClicked) override;

    void clear()
    {
        ScopedLock sl(lock);
        {
            tempString.clear();
            line = 0;
            processorLines.clear();
        }
        
        textConsole->clear();
        
    }

	void handleAsyncUpdate();

	/** Adds a new line to the console.
    *
	*   This can be called in the audio thread. It stores all text in an internal String buffer and writes it periodically
	*   on the timer callback.
	*/
	void logMessage(const String &t, WarningLevel warningLevel, const Processor *p, Colour c);

private:

	struct ConsoleMessage
	{
		ConsoleMessage(const String &m, WarningLevel w, const Processor *p, Colour c=Colours::black):
			message(m),
			warningLevel(w),
			processor(p),
			colour(c)
		{ };

		ConsoleMessage():
			message(String::empty),
			warningLevel(Error),
		    processor(nullptr)
		{
            jassertfalse;
        };

		ConsoleMessage &operator=( const ConsoleMessage&)
		{
            jassertfalse;
            
			return *this;
		};

		const String message;
		const Processor * const processor;
		const WarningLevel warningLevel;
		Colour colour;
        
    };

	friend class WeakReference<Console>;
	WeakReference<Console>::Master masterReference;
	
	CriticalSection lock;

    ScopedPointer<TextEditor> textConsole;
    ScopedPointer<TextButton> clearButton;
	ScopedPointer<Label> voiceLabel;

	bool popupMode;
	bool overflowProtection;

	int voiceAmount;

	int hostTempo;

	ScopedPointer<Slider> cpuSlider;

	String tempString;

	int line;
	int usage;

	Array<bool> errors;

	Array<WeakReference<Processor>> processorLines;


	Array<ConsoleMessage> unprintedMessages;

};

#endif  // __CONSOLE_H_8FBBF4D7__
