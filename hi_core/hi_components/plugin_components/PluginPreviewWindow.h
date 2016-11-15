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


#ifndef PLUGINPREVIEWWINDOW_H_INCLUDED
#define PLUGINPREVIEWWINDOW_H_INCLUDED

class BackendProcessorEditor;
class ScriptContentContainer;

class PluginPreviewWindow : public DocumentWindow,
							public ModalBaseWindow
{
public:

	PluginPreviewWindow(BackendProcessorEditor *editor);

	void closeButtonPressed() override;;

	class Content : public Component,
		public ComponentWithKeyboard
	{
	public:

		Content(BackendProcessorEditor *editor_);

		~Content();

		void resized() override;

		KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); }

		Component *getKeyboard() const override { return keyboard; };

	private:

		Component::SafePointer<BackendProcessorEditor> editor;
		ScopedPointer<DefaultFrontendBar> frontendBar;
		ScopedPointer<ScriptContentContainer> container;
		ModulatorSynthChain *mainSynthChain;
		ScopedPointer<CustomKeyboard> keyboard;
	};

private:

	

	Component::SafePointer<BackendProcessorEditor> editor;
};




#endif  // PLUGINPREVIEWWINDOW_H_INCLUDED
