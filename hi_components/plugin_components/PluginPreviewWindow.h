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


#ifndef PLUGINPREVIEWWINDOW_H_INCLUDED
#define PLUGINPREVIEWWINDOW_H_INCLUDED

namespace hise { using namespace juce;

class BackendProcessorEditor;
class ScriptContentComponent;

class PluginPreviewWindow : public DocumentWindow,
							public ModalBaseWindow
{
public:

	PluginPreviewWindow(BackendProcessorEditor *editor);

	void closeButtonPressed() override;;


	class Content : public Component
	{
	public:

		Content(BackendProcessorEditor *editor_);

		void paint(Graphics& g) override
		{
			g.fillAll(Colours::black);
		}

		~Content();

		void resized() override;

	private:

		class ScriptDeleteListener;

		ScopedPointer<ScriptDeleteListener> deleteListener;

		Component::SafePointer<BackendProcessorEditor> editor;
		
		ScopedPointer<ScriptContentComponent> content;
		JavascriptMidiProcessor* scriptProcessor = nullptr;
		ModulatorSynthChain *mainSynthChain;
	};



	static Content* createContent(BackendProcessorEditor* editor);

private:

	

	Component::SafePointer<BackendProcessorEditor> editor;
};


} // namespace hise

#endif  // PLUGINPREVIEWWINDOW_H_INCLUDED
