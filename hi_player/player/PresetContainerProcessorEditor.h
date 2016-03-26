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

#ifndef PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED
#define PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED




class PresetContainerProcessorEditor : public AudioProcessorEditor,
	public FileDragAndDropTarget,
	public Button::Listener
{
public:

	PresetContainerProcessorEditor(PresetContainerProcessor *ppc_);;

	void buttonClicked(Button *b) override;

	virtual bool 	isInterestedInFileDrag(const StringArray &files) override;

	void filesDropped(const StringArray &files, int, int);
	void refreshPresetList(bool forceRefresh=false);

	void paint(Graphics &g);

	void resized();
	void removePresetProcessor(PresetProcessor * pp, VerticalListComponent::ChildComponent *presetEditor);
	void toggleBrowserWindow();

	void mouseDown(const MouseEvent &e) override;

	void addNewPreset(const File &f);

private:

	PresetContainerProcessor *ppc;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<Viewport> viewport;

	ScopedPointer<TooltipBar> tooltipBar;

	LookAndFeel_V3 laf;

	ScopedPointer<VerticalListComponent> verticalList;

	ScopedPointer<VerticalListComponent> browserList;

	ScopedPointer<ResizableBorderComponent> borderDragger;

	ScopedPointer<AnimatedPanelViewport> presetBrowser;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	ScopedPointer<ShapeButton> browserButton;

#if JUCE_DEBUG && INCLUDE_COMPONENT_DEBUGGER

	ScopedPointer<jcf::ComponentDebugger> debugger;

#endif

};



#endif  // PRESETCONTAINERPROCESSOREDITOR_H_INCLUDED
