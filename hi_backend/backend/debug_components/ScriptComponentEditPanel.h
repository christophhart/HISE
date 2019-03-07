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

		Path createPath(const String& id) const override
		{
			Path path;

			auto url = HtmlGenerator::getSanitizedFilename(id);
			
			if(url == "copy")
			{
				static const unsigned char pathData[] = { 110,109,0,0,240,65,0,0,0,0,108,0,0,32,65,0,0,0,0,98,4,86,14,65,0,0,0,0,0,0,0,65,184,30,101,63,0,0,0,65,0,0,0,64,108,0,0,0,65,0,0,0,65,108,0,0,0,64,0,0,0,65,98,66,96,101,63,0,0,0,65,0,0,0,0,236,81,14,65,0,0,0,0,0,0,32,65,108,0,0,0,0,0,0,240,65,98,0,0,
					0,0,254,212,248,65,66,96,101,63,0,0,0,66,0,0,0,64,0,0,0,66,108,0,0,176,65,0,0,0,66,98,10,215,184,65,0,0,0,66,0,0,192,65,254,212,248,65,0,0,192,65,0,0,240,65,108,0,0,192,65,0,0,192,65,108,0,0,240,65,0,0,192,65,98,254,212,248,65,0,0,192,65,0,0,0,66,254,
					212,184,65,0,0,0,66,0,0,176,65,108,0,0,0,66,0,0,0,64,98,0,0,0,66,184,30,101,63,254,212,248,65,0,0,0,0,0,0,240,65,0,0,0,0,99,109,0,0,168,65,0,0,232,65,108,0,0,64,64,0,0,232,65,108,0,0,64,64,0,0,48,65,108,0,0,0,65,0,0,48,65,108,0,0,0,65,0,0,176,65,98,0,
					0,0,65,10,215,184,65,4,86,14,65,0,0,192,65,0,0,32,65,0,0,192,65,108,0,0,168,65,0,0,192,65,108,0,0,168,65,0,0,232,65,99,109,0,0,232,65,0,0,168,65,108,0,0,48,65,0,0,168,65,108,0,0,48,65,0,0,64,64,108,0,0,232,65,0,0,64,64,108,0,0,232,65,0,0,168,65,99,101,
					0,0 };


				path.loadPathFromData(pathData, sizeof(pathData));

				return path;
			}
			if (url == "paste")
			{
				static const unsigned char pathData[] = { 110,109,55,62,71,67,93,185,77,67,108,55,62,71,67,169,122,74,67,98,55,62,71,67,72,31,74,67,115,243,70,67,133,212,73,67,19,152,70,67,133,212,73,67,108,22,13,66,67,133,212,73,67,108,22,13,66,67,61,136,72,67,98,22,13,66,67,124,209,71,67,142,119,65,67,245,
					59,71,67,206,192,64,67,245,59,71,67,108,61,40,62,67,245,59,71,67,98,118,113,61,67,245,59,71,67,245,219,60,67,124,209,71,67,245,219,60,67,61,136,72,67,108,245,219,60,67,133,212,73,67,108,248,80,56,67,133,212,73,67,98,149,245,55,67,133,212,73,67,212,170,
					55,67,72,31,74,67,212,170,55,67,169,122,74,67,108,212,170,55,67,123,117,87,67,98,212,170,55,67,220,208,87,67,149,245,55,67,159,27,88,67,248,80,56,67,159,27,88,67,108,61,40,62,67,159,27,88,67,108,61,40,62,67,120,0,92,67,108,55,62,71,67,120,0,92,67,108,
					16,35,75,67,159,27,88,67,108,16,35,75,67,93,185,77,67,108,55,62,71,67,93,185,77,67,99,109,61,40,62,67,211,136,72,67,98,101,40,62,67,151,136,72,67,192,40,62,67,80,136,72,67,192,40,62,67,80,136,72,67,108,28,192,64,67,80,136,72,67,98,88,192,64,67,119,136,
					72,67,159,192,64,67,211,136,72,67,159,192,64,67,211,136,72,67,108,159,192,64,67,132,212,73,67,108,14,40,62,67,132,212,73,67,108,14,40,62,67,211,136,72,67,99,109,100,67,58,67,20,109,76,67,108,100,67,58,67,204,32,75,67,108,166,165,68,67,204,32,75,67,108,
					166,165,68,67,20,109,76,67,108,100,67,58,67,20,109,76,67,99,109,55,62,71,67,140,42,90,67,108,55,62,71,67,159,27,88,67,108,36,77,73,67,159,27,88,67,108,55,62,71,67,140,42,90,67,99,109,199,214,73,67,86,207,86,67,108,238,241,69,67,86,207,86,67,108,238,241,
					69,67,47,180,90,67,108,133,116,63,67,47,180,90,67,108,133,116,63,67,164,5,79,67,108,199,214,73,67,164,5,79,67,108,199,214,73,67,86,207,86,67,99,101,0,0 };

				path.loadPathFromData(pathData, sizeof(pathData));
				return path;
			}
			
			return Path();
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
