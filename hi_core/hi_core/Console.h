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

#ifndef __CONSOLE_H_8FBBF4D7__
#define __CONSOLE_H_8FBBF4D7__

namespace hise { using namespace juce;

class Processor;
class BaseDebugArea;

class BackendRootWindow;
class BackendProcessorEditor;


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
			   public CodeDocument::Listener
{
public:

	Console(MainController* mc);

	~Console();
    
    void mouseDown(const MouseEvent &e) override;
    void mouseMove(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent& event) override;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

	void resized() override;

	void codeDocumentTextInserted(const String &/*newText*/, int /*insertIndex*/) override;

	void codeDocumentTextDeleted(int /*startIndex*/, int /*endIndex*/) override;

	void clear();

	/** Adds a new line to the console.
    *
	*   This can be called in the audio thread. It stores all text in an internal String buffer and writes it periodically
	*   on the timer callback.
	*/

	void setTokeniser(CodeTokeniser* newTokeniser);

	static void updateFontSize(Console& c, float newSize);

private:

	class ConsoleTokeniser : public CodeTokeniser
	{
	public:

		ConsoleTokeniser();

		int readNextToken(CodeDocument::Iterator& source);

		CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

	private:

		int state = 0;

		CodeEditorComponent::ColourScheme s;

	};


	class ConsoleEditorComponent : public CodeEditorComponent
	{
	public:

		ConsoleEditorComponent(CodeDocument &doc, CodeTokeniser *tok);

		void addPopupMenuItems(PopupMenu &/*menuToAddTo*/, const MouseEvent *) override;;
        
        ScrollbarFader fader;
	};

	ScopedPointer<ConsoleEditorComponent> newTextConsole;
	ScopedPointer<CodeTokeniser> tokeniser;

	MainController* mc;
	JUCE_DECLARE_WEAK_REFERENCEABLE(Console);
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

} // namespace hise

#endif  // __CONSOLE_H_8FBBF4D7__
