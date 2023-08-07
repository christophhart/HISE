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

#ifndef SCRIPTCOMPONENTEDITPANEL_H_INCLUDED
#define SCRIPTCOMPONENTEDITPANEL_H_INCLUDED

namespace hise { using namespace juce;

class HiPropertyComponent;

class ScriptComponentEditPanel : public Component,
	public ScriptComponentEditListener,
	public Timer,
	public TextEditor::Listener,
	public Button::Listener,
	public CopyPasteTarget
{
public:
	struct Factory: public PathFactory
	{
		enum ID
		{
			Copy,
			Paste,
			Delete,
			Rebuild,
			Edit,
			Play,
			numIDs
		};

		String getId() const override { return "Property Panel Icons"; }

		Path createPath(const String& id) const override
		{
			Path p;
			auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
			
			LOAD_EPATH_IF_URL("copy", SampleMapIcons::copySamples);
			LOAD_EPATH_IF_URL("paste", SampleMapIcons::pasteSamples);
			
			return p;
		}
	};

public:

	ScriptComponentEditPanel(MainController* mc_, Processor* p);

	~ScriptComponentEditPanel();

	class Panel : public PanelWithProcessorConnection
	{
	public:
		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{
			
		}

		~Panel()
		{
			
		}

		
		Identifier getProcessorTypeId() const override;

		Component* createContentComponent(int index) override;

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}

		SET_PANEL_NAME("InterfacePropertyEditor");
	};

	void scriptComponentSelectionChanged() override;

	void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue) override;

	void timerCallback() override
	{
		fillPanel();
		stopTimer();
	}

	void textEditorReturnKeyPressed(TextEditor&) override;

	void buttonClicked(Button* b) override;

	void paint(Graphics &g) override;

	void resized() override;

	String getObjectTypeName() override { return "Properties"; };
	
	void copyAction() override;;
	virtual void pasteAction();;

	void mouseDown(const MouseEvent& /*event*/) override
	{
		grabCopyAndPasteFocus();
	}

	static void debugProperties(DynamicObject *properties);;

	SelectedItemSet<Component::SafePointer<HiPropertyComponent>>& getPropertySelection()
	{
		return selectedComponents;
	}

	void repaintAllPanels()
	{
		panel->refreshAll();
	}

private:

    ScrollbarFader sf;
    
	ScopedPointer<ShapeButton> copyButton;
	ScopedPointer<ShapeButton> pasteButton;
	ScopedPointer<MarkdownHelpButton> helpButton;

	SelectedItemSet<Component::SafePointer<HiPropertyComponent>> selectedComponents;

	void fillPanel();

	void rebuildScriptedComponents();

	void addSectionToPanel(const Array<Identifier> &idsToAdd, const String &sectionName);

	void addProperty(Array<PropertyComponent*> &arrayToAddTo, const Identifier &id);

	friend class HiColourPropertyComponent;

	HiPropertyPanelLookAndFeel pplaf;

	PopupLookAndFeel plaf;
	AlertWindowLookAndFeel alaf;
	
	ScopedPointer<TextEditor> idEditor;
	

	ScopedPointer<PropertyPanel> panel;
	
	MainController* mc;

	WeakReference<Processor> connectedProcessor;
	void updateIdEditor();
};

} // namespace hise

#endif  // SCRIPTCOMPONENTEDITPANEL_H_INCLUDED
