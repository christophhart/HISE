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

class BackendRootWindow;
class BackendProcessorEditor;

/** If the component somehow needs access to the main panel, subclass it from this interface and use getMainPanel(). */
class ComponentWithAccessToMainPanel
{
public:

	virtual ~ComponentWithAccessToMainPanel() {};

protected:

	BackendProcessorEditor* getMainPanel();
	const BackendProcessorEditor* getMainPanel() const;
};


#if USE_BACKEND

/** A textbox that acts as console (read only / autoscrolling)
*	@ingroup debugComponents
*    
*   The console has a clear button that deletes the content.*
*	Use the (addLine) method to write something to the console.
*    
*	For Modulators there is the macro function debugMod(String &t)
*/
class Console: public Component,
			   public AsyncUpdater,
			   public ComponentWithAccessToMainPanel
{
public:

	
	
	enum WarningLevel
	{
		Message = 0,
		Error = 1
	};

	Console(MainController* mc);

	~Console();

	void sendChangeMessage();
    
    void mouseDown(const MouseEvent &e) override;
    void mouseMove(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent& event) override;

	void resized() override;

    void clear();

	void handleAsyncUpdate();

	/** Adds a new line to the console.
    *
	*   This can be called in the audio thread. It stores all text in an internal String buffer and writes it periodically
	*   on the timer callback.
	*/
	void logMessage(const String &t, WarningLevel warningLevel, const Processor *p, Colour c);

private:

	class ConsoleTokeniser : public CodeTokeniser
	{
	public:

		ConsoleTokeniser();

		int readNextToken(CodeDocument::Iterator& source);

		CodeEditorComponent::ColourScheme getDefaultColourScheme() override { return s; }


	private:

		CodeEditorComponent::ColourScheme s;

	};


	class ConsoleEditorComponent : public CodeEditorComponent
	{
	public:

		ConsoleEditorComponent(CodeDocument &doc, CodeTokeniser *tok);

		void addPopupMenuItems(PopupMenu &/*menuToAddTo*/, const MouseEvent *) override {};

		

	};


	enum class ConsoleMessageItems
	{
		WarningLevel = 0,
		Processor,
		Message
	};

	using ConsoleMessage = std::tuple < WarningLevel, const WeakReference<Processor>, String > ;

	const CriticalSection &getLock() const { return lock; }


	std::vector<ConsoleMessage> unprintedMessages;

	friend class WeakReference<Console>;
	WeakReference<Console>::Master masterReference;
	
	CriticalSection lock;

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<ConsoleEditorComponent> newTextConsole;
	ScopedPointer<CodeTokeniser> tokeniser;

	bool overflowProtection;

	bool clearFlag;

};

#else

class Console
{

	virtual ~Console() { masterReference.clear(); }

private:

	friend class WeakReference<Console>;
	WeakReference<Console>::Master masterReference;
};

#endif

#endif  // __CONSOLE_H_8FBBF4D7__
