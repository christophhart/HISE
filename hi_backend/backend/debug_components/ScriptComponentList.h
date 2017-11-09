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


#ifndef SCRIPTCOMPONENTLIST_H_INCLUDED
#define SCRIPTCOMPONENTLIST_H_INCLUDED

namespace hise { using namespace juce;

class ScriptComponentList : public SearchableListComponent,
							public ScriptComponentEditListener,
							public ScriptingApi::Content::RebuildListener,
							public GlobalScriptCompileListener,
							public ButtonListener,
							public DragAndDropContainer
{
public:

	ScriptComponentList(BackendRootWindow* window, JavascriptProcessor* p);

	~ScriptComponentList();

	void buttonClicked(Button* b) override;

	class ScriptComponentItem : public SearchableListComponent::Item,
								public SafeChangeListener,
								public DragAndDropTarget
								
	{
	public:

		ScriptComponentItem(ScriptComponent* c_);;

		~ScriptComponentItem();

		void changeListenerCallback(SafeChangeBroadcaster* b) override;

		void mouseEnter(const MouseEvent&) override;
		void mouseExit(const MouseEvent&) override;
		void mouseDoubleClick(const MouseEvent&) override;

		void paint(Graphics& g) override;

		void mouseUp(const MouseEvent& event) override;

		void mouseDrag(const MouseEvent& event) override;

		bool keyPressed(const KeyPress& key) override;

		bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
		
		void itemDragEnter(const SourceDetails& dragSourceDetails) override;

		void itemDragExit(const SourceDetails& dragSourceDetails) override;

		void itemDragMove(const SourceDetails& dragSourceDetails) override;

		void itemDropped(const SourceDetails& dragSourceDetails) override;


	private:

		bool insertDragAfterComponent = false;
		bool insertDragAsParentComponent = false;

		int childDepth = 0;

		AttributedString s;

		String id;
		String typeName;
		float typeOffset;

		ScriptComponent::Ptr c;
	};

	
	void scriptComponentSelectionChanged() override
	{
		getCollection(0)->repaintAllItems();
	}

	
	void scriptComponentPropertyChanged(ScriptComponent* /*sc*/, Identifier idThatWasChanged, const var& /*newValue*/)
	{
		static const Identifier e("visible");

		if (e == idThatWasChanged && showOnlyVisibleItems)
		{
			rebuildModuleList(true);
		}
	}

	void scriptWasCompiled(JavascriptProcessor* p)
	{
		if (jp == p)
		{
			rebuildModuleList(true);
		}
	}

	void contentWasRebuilt() override
	{
		rebuildModuleList(true);
	}

	class AllCollection : public SearchableListComponent::Collection,
						  public DragAndDropTarget
	{
	public:

		AllCollection(JavascriptProcessor* p, bool showOnlyVisibleItems);

		void paint(Graphics& g) override;

		bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;

		void itemDragEnter(const SourceDetails& dragSourceDetails) override;

		void itemDragExit(const SourceDetails& dragSourceDetails) override;

		void itemDragMove(const SourceDetails& dragSourceDetails) override;

		void itemDropped(const SourceDetails& dragSourceDetails) override;

	private:

		bool isDropTarget = false;
	};

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{};

		SET_PANEL_NAME("ScriptComponentList");

		Identifier getProcessorTypeId() const override { return JavascriptProcessor::getConnectorId(); }

		Component* createContentComponent(int /*index*/) override;

		

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}
	};

	int getNumCollectionsToCreate() const override { return 1; }

	Collection *createCollection(int /*index*/) override
	{
		return new AllCollection(jp, showOnlyVisibleItems);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));
		SearchableListComponent::paint(g);
	}

private:

	bool showOnlyVisibleItems = false;

	ScopedPointer<ShapeButton> hideButton;

	ScriptComponent::Ptr lastClickedComponent;

	JavascriptProcessor* jp;
};

} // namespace hise

#endif  // SCRIPTCOMPONENTLIST_H_INCLUDED
