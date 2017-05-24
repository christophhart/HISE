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

#ifndef DEBUGAREA_H_INCLUDED
#define DEBUGAREA_H_INCLUDED

class BackendProcessorEditor;
class MacroParameterTable;
class ScriptWatchTable;




class BaseDebugArea: public Component,
					 public Timer,
					 public RestorableObject
{
public:

	BaseDebugArea(BackendProcessorEditor *mainEditor_);

	virtual ~BaseDebugArea()
	{
		panel = nullptr;
	}

	class Toolbar : public Component,
					public ButtonListener
	{
	public:

		Toolbar() {};

		virtual ~Toolbar() {};

		int getIndexForButton(Button *b)
		{
			for (int i = 0; i < toolbarButtons.size(); i++)
			{
				if (b == toolbarButtons[i]) return i;
			}

			return -1;
		}

		void buttonClicked(Button *b) override
		{
			const int index = getIndexForButton(b);

			toggleVisibility(index);
		}

		void showComponent(int index, bool show, bool updateButton = false)
		{
			ShapeButton *b = toolbarButtons[index];

			BaseDebugArea *area = (BaseDebugArea*)getParentComponent();

			Component *co = area->getComponentForIndex(index);

			bool isVisible = !show; //area->isInPanel(co);

			if (isVisible) area->panel->removePanel(co);
			else
			{
				int panelIndex = area->getPanelIndexFor(co);

				area->panel->addPanel(panelIndex, co, false);

			}

			if (updateButton)
			{
				b->setToggleState(show, dontSendNotification);
				Colour c = b->getToggleState() ? Colours::white.withAlpha(0.8f) : Colour(BACKEND_BG_COLOUR_BRIGHT);

				b->setColours(c.withAlpha(0.8f), c, c);
			}

			area->resized();
		}

		void toggleVisibility(int buttonIndex)
		{
			ShapeButton *b = toolbarButtons[buttonIndex];

			b->setToggleState(!b->getToggleState(), dontSendNotification);

			Colour c = b->getToggleState() ? Colour(BACKEND_ICON_COLOUR_ON) : Colour(BACKEND_ICON_COLOUR_OFF);

			b->setColours(c.withAlpha(0.8f), c, c);

			showComponent(buttonIndex, b->getToggleState());

		}

		void resized() override
		{
			for (int i = 0; i < toolbarButtons.size(); i++)
			{
				toolbarButtons[i]->setBounds(i * (getHeight() + 5), 0, getHeight(), getHeight());
			}
		}

	private:

		friend class BaseDebugArea;

		OwnedArray<ShapeButton> toolbarButtons;

	};

	class DebugPopout : public DocumentWindow
	{
	public:

		DebugPopout(Component* c, BaseDebugArea* parent, Rectangle<int> area);

		void closeButtonPressed() override;

		void resized() override
		{
			if (getContentComponent() != nullptr)
			{
                auto b = getScreenBounds();
                
				area->setFloatingArea(getContentComponent(), b);
			}
		}

		BackendProcessorEditor* editor;
		BaseDebugArea* area;
	};

	void timerCallback() override
	{
		panel->repaint();
	}

	bool isInPanel(Component *c) const
	{
		for (int i = 0; i < panel->getNumPanels(); i++)
		{
			if (panel->getPanel(i) == c)
			{
				return true;
			}
		}

		return false;
	}

	bool isFloating(Component* c) const
	{
		for (int i = 0; i < floatingWindows.size(); i++)
		{
			if (floatingWindows[i]->getContentComponent() == c)
				return true;
		}

		return false;
	}

	virtual Identifier getIdForArea() const = 0;

	virtual Identifier getIdForComponent(int i) const = 0;

	int getPanelIndexFor(Component *c) const
	{
		const int idealIndex = getIndexForComponent(c);

		for (int i = 0; i < panel->getNumPanels(); i++)
		{
			if (getIndexForComponent(panel->getPanel(i)) > idealIndex) return i;
		}

		return -1;
	}

	ValueTree exportAsValueTree() const override
	{
		ValueTree v(getIdForArea());

		static const Identifier floating("floating");
		static const Identifier ip("inPanel");
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier w("w");
		static const Identifier h("h");

		for (int i = 0; i < getNumComponents(); i++)
		{
			ValueTree child(getIdForComponent(i));

			Component *co = getComponentForIndex(i);

			child.setProperty(ip, isInPanel(co), nullptr);
			
			if (isFloating(co))
			{
				child.setProperty(floating, true, nullptr);

				child.setProperty(x, floatingAreas[i].getX(), nullptr);
				child.setProperty(y, floatingAreas[i].getY(), nullptr);
				child.setProperty(w, floatingAreas[i].getWidth(), nullptr);
				child.setProperty(h, floatingAreas[i].getHeight(), nullptr);
			}
			else
			{
				child.setProperty(floating, false, nullptr);
				child.setProperty(y, co->getHeight(), nullptr);
			}

			v.addChild(child, -1, nullptr);
		}

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		if (v.getNumChildren() == 0)
			return;

		jassert(v.getType() == getIdForArea());
		jassert(getNumComponents() == v.getNumChildren());

		static const Identifier floating("floating");
		static const Identifier ip("inPanel");
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier w("w");
		static const Identifier h("h");

		// Delete them all
		for (int i = 0; i < getNumComponents(); i++)
		{
			panel->removePanel(getComponentForIndex(i));
		}
		
		// Add them all
		for (int i = 0; i < getNumComponents(); i++)
		{
			ValueTree child = v.getChild(i);

			const bool ip_ = (bool)child.getProperty(ip, false);

			if (ip_)
			{
				showComponent(i, true);
				continue;
			}

			const bool floating_ = child.getProperty(floating, false);
			
			if (floating_)
			{
				floatingAreas[i].setBounds(child.getProperty(x, 0), child.getProperty(y, 0), child.getProperty(w, 0), child.getProperty(h, 0));
				openInFloatingWindow(i, true);
			}
		}

		resized();
	}

	virtual Component *getComponentForIndex(int i) const = 0;

	virtual int getNumComponents() const = 0;

	int getIndexForComponent(Component *c) const
	{
		for (int i = 0; i <= getNumComponents(); i++)
		{
			if (c == getComponentForIndex(i)) return i;
		}
		jassertfalse;
		return -1;
	}

    void paint(Graphics &g) override;

	void mouseDown(const MouseEvent& e) override;

	void resized() override
	{
		panel->setBounds(1, 28, getWidth()-2, getHeight() - 28);
		debugToolbar->setBounds(0, 0, getWidth(), 20);

		const int h = getHeight() - 20 - 16;

        if(h < 0) return;
        
        
		const int numVisiblePanels = (panel->getNumPanels() - 1);

        if(numVisiblePanels == 0)
        {
            return;
        }
        
		const int heightPerPanel = (numVisiblePanels != 0) ? (h / numVisiblePanels - 20) : 0;

		for (int i = numVisiblePanels - 1; i >= 0; i--) // go backwards or it doesn't work
		{
            Component *cp = panel->getPanel(i);
            
            if(cp != nullptr)
            {
                panel->setPanelSize(cp, heightPerPanel, false);
            }
        }

		panel->setPanelSize(panel->getPanel(panel->getNumPanels() - 1), 0, false);


	}

	bool componentIsInPanel(int index)
	{
		Component *c = getComponentForIndex(index);

		for (int i = 0; i < panel->getNumPanels(); i++)
		{
			if (panel->getPanel(i) == c) return true;
		}

		return false;
	}
	void showComponent(int index, bool shouldBeShown);

	void openInFloatingWindow(int index, bool shouldBeShown);

	void setFloatingArea(Component* c, Rectangle<int>& newBounds)
	{
		int index = getIndexForComponent(c);

		if (index >= 0 && index < 16)
		{
			floatingAreas[index] = newBounds;
		}
	}

	/** Overwrite this if you want to limit a components size. */
	virtual int getMaximumSizeForComponent(int /*componentIndex*/) { return -1; }

	BackendProcessorEditor* getBackendProcessorEditor() { return mainEditor; }

protected:

	void addButtonToToolbar(ShapeButton *newButton);

	BackendProcessorEditor* mainEditor;
	ScopedPointer<Component> emptyComponent;

private:

    ConcertinaPanelHeaderLookAndFeel laf;
    
	ScopedPointer<ConcertinaPanel> panel;
	
	ScopedPointer<Toolbar> debugToolbar;
	
	OwnedArray<DebugPopout> floatingWindows;
	
	Rectangle<int> floatingAreas[16];

};


class CombinedDebugArea: public BaseDebugArea
				 
{
public:

	enum AreaIds
	{
		ProcessorCollection,
		ApiCollectionEnum,
		FileBrowserShown,
		SamplePool,
		ExternalFiles,
		ImageTable,
		numAreas
	};

	CombinedDebugArea(BackendProcessorEditor *mainEditor_);

	class CombinedDebugToolbar: public BaseDebugArea::Toolbar
	{
	public:

		CombinedDebugToolbar();

	protected:

		OwnedArray<ShapeButton> toolbarButtons;

	};

	Identifier getIdForArea() const override { return "References"; }

	Identifier getIdForComponent(int i) const override;

	Component *getComponentForIndex(int i) const override;

	int getNumComponents() const override { return numAreas; };

	~CombinedDebugArea()
	{
		poolTable = nullptr;
		externalFileTable = nullptr;
	}

	
private:

	ScopedPointer<SamplePoolTable> poolTable;
	ScopedPointer<ExternalFileTable<AudioSampleBuffer>> externalFileTable;
	ScopedPointer<ExternalFileTable<Image>> imageTable;
	ScopedPointer<ModuleBrowser> processorDragList;
	ScopedPointer<ApiCollection> apiCollection;
	
	ScopedPointer<FileBrowser> fileBrowser;
};


class PropertyDebugArea : public BaseDebugArea
{
public:

	enum AreaIds
	{
		MacroArea = 0,
		ScriptTable,
		ScriptComponentPanel,
		ModulationPlotter,
		DebugConsole,
		ModuleBrowser,
		numAreas
	};

	PropertyDebugArea(BackendProcessorEditor *editor);

	~PropertyDebugArea()
	{

		console = nullptr;
		plotter = nullptr;
		scriptComponentPanel = nullptr;
		scriptWatchTable = nullptr;
		macroTable = nullptr;
		moduleList = nullptr;
	}

	Identifier getIdForArea() const override { return "Properties"; }

	class PropertyDebugToolbar : public BaseDebugArea::Toolbar
	{
	public:

		PropertyDebugToolbar();

	protected:

		OwnedArray<ShapeButton> toolbarButtons;

	};

	Identifier getIdForComponent(int i) const override;

	Component *getComponentForIndex(int i) const override;

	int getNumComponents() const override { return numAreas; };


	MacroParameterTable *getMacroTable()
	{
		return macroTable;
	}

private:

	ScopedPointer<Console> console;
	ScopedPointer<Plotter> plotter;
	ScopedPointer<ScriptWatchTable> scriptWatchTable;
	ScopedPointer<MacroParameterTable> macroTable;
	ScopedPointer<ScriptComponentEditPanel> scriptComponentPanel;
	ScopedPointer<PatchBrowser> moduleList;
};


#endif  // DEBUGAREA_H_INCLUDED
