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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#define OLD_COLOUR 0

#ifndef FLOATINGTILECONTENT_H_INCLUDED
#define FLOATINGTILECONTENT_H_INCLUDED

class FloatingTile;
class FloatingTileContainer;

#define RETURN_DEFAULT_PROPERTY_ID(idToCheck, name, id) if(idToCheck == (int)name) { static const Identifier i(id); return i;}
#define RETURN_DEFAULT_PROPERTY(idToCheck, name, value) if(idToCheck == (int)name) { return value; }

/** A base class for objects that can be stored and restored using DynamicObject. */
class ObjectWithDefaultProperties
{
public:

	virtual ~ObjectWithDefaultProperties() {};

	/** Writes the state of this object into a dynamic object. */
	virtual var toDynamicObject() const = 0;

	/** Restores the state of this object from a dynamic object. */
	virtual void fromDynamicObject(const var& objectData) = 0;

	/** Overwrite this and get the default property. */
	virtual var getDefaultProperty(int id) const = 0;
	
	/** Overwrite this and return the number of properties that are defaultable. */
	virtual int getNumDefaultableProperties() const = 0;
	
	/** Overwrite this and return the Identifier for the given property index. 
	*
	*	You might want to use the macro RETURN_DEFAULT_PROPERTY_ID(idToCheck, name) with a named enum for this. 
	*
	*/
	virtual Identifier getDefaultablePropertyId(int i) const = 0;

	/** Clears the given object and sets all defaultable properties to their initial values. */
	void resetObject(DynamicObject* objectToClear)
	{
		jassert(objectToClear != nullptr);

		objectToClear->clear();

		for (int i = 0; i < getNumDefaultableProperties(); i++)
		{
			objectToClear->setProperty(getDefaultablePropertyId(i), getDefaultProperty(i));
		}
	}

	void storePropertyInObject(var obj, int id, var value, var defaultValue = var()) const
	{
		jassert(obj.isObject());

		if ((defaultValue.isUndefined() || defaultValue.isVoid()) || value != defaultValue)
		{
			obj.getDynamicObject()->setProperty(getDefaultablePropertyId(id), value);
		}
	}

	var getPropertyWithDefault(var obj, int id) const
	{
		if (auto object = obj.getDynamicObject())
		{
			auto prop = getDefaultablePropertyId(id);

			if (object->hasProperty(prop))
				return object->getProperty(prop);
			else
				return getDefaultProperty(id);
		}

		return {};
	}
};


class JSONEditor : public Component
{
public:

	JSONEditor(ObjectWithDefaultProperties* editedObject):
		editedComponent(dynamic_cast<Component*>(editedObject))
	{
		setName("JSON Editor");

		tokeniser = new JavascriptTokeniser();
		doc = new CodeDocument();
		doc->replaceAllContent(JSON::toString(editedObject->toDynamicObject()));
		doc->setSavePoint();
		doc->clearUndoHistory();

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
	}

	JSONEditor(var object)
	{
		auto s = JSON::toString(object, false);

		tokeniser = new JavascriptTokeniser();
		doc = new CodeDocument();
		doc->replaceAllContent(s);
		doc->setSavePoint();
		doc->clearUndoHistory();

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
		editor->setReadOnly(true);
		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(16.0f));
		editor->setReadOnly(true);

		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

	}

	JSONEditor(const File& f)
	{
		setName("External Script Preview");

		tokeniser = new JavascriptTokeniser();
		doc = new CodeDocument();
		doc->replaceAllContent(f.loadFileAsString());
		doc->setSavePoint();
		doc->clearUndoHistory();

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
		editor->setReadOnly(true);
		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(16.0f));

		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
	}

	~JSONEditor()
	{
		
	}

	bool keyPressed(const KeyPress& key)
	{
		if (editedComponent == nullptr)
			return false;

		if (key == KeyPress::F5Key)
		{
			replace();
			return true;
		}

		return false;
	}

	void replace();
	

	void resized() override
	{
		editor->setBounds(getLocalBounds());
		resizer->setBounds(getWidth() - 12, getHeight() - 12, 12, 12);
	}


private:

	Component::SafePointer<Component> editedComponent;

	ScopedPointer<ResizableCornerComponent> resizer;

	ComponentBoundsConstrainer constrainer;

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer<CodeEditorComponent> editor;
};



/** The base class for all components that can be put into a floating panel. 
*
*	In order to use it:
*
*	- Subclass this interface
*	- Use the macro SET_PANEL_NAME() to set it's unique ID
*	- register the class to the FloatingPanel::Factory
*	- add it to the popup menu
*
*	You can use the ValueTree methods to save / restore the state.
*/
class FloatingTileContent : public ObjectWithDefaultProperties
{
protected:

	/** Creates a new floating panel. 
	*
	*	You must supply a FloatingShellComponent at initialisation to prevent the requirement for lazy initialisation.
	*/
	FloatingTileContent(FloatingTile* parent_) :
		parent(parent_)
	{
		styleData = getDefaultProperty((int)PanelPropertyId::StyleData);

	}

public:

	enum class PanelPropertyId
	{
		Type = 0, ///< the ID of the panel that determines the actual type.
		Title, ///< can be used to show a better title in the popup
		StyleData,
		ColourData, ///< an object that contains 5 basic colour IDs that can be used to customize the panel: `bgColour`, `textColour`, `itemColour1`, `itemColour2` and `itemColour3`
		LayoutData, ///< this object contains information about the layout within its parent container. You usually don't need to change this via scripting.
		Font, ///< if the panel is rendering text, this can be used to change the font. For changing the font-style, append ` Bold` or ` Italic` at the font name.
		FontSize, ///< if the panel is rendering text, this can be used to change the font size.
		numPropertyIds
	};

	

	virtual ~FloatingTileContent()
	{
		parent = nullptr;
	}

	/** Returns the parent shell. 
	*
	*	Unlike getParentComponent(), this always returns non nullptr, so you can use it in the constructor.
	*/
	FloatingTile* getParentShell()
	{
		jassert(parent != nullptr);

		return parent; 
	}

	


	int getNumDefaultableProperties() const override { return (int)PanelPropertyId::numPropertyIds; };
	Identifier getDefaultablePropertyId(int i) const override;
	var getDefaultProperty(int id) const override;

	MainController* getMainController();

	const MainController* getMainController() const;

	BackendRootWindow* getRootWindow();

	const BackendRootWindow* getRootWindow() const;

	const FloatingTile* getParentShell() const 
	{
		jassert(parent != nullptr);

		return parent; 
	}

	virtual String getTitle() const { return getIdentifierForBaseClass().toString(); };
	virtual Identifier getIdentifierForBaseClass() const = 0;

	var toDynamicObject() const override;

	void fromDynamicObject(const var& object) override;

	static FloatingTileContent* createPanel(const var& data, FloatingTile* parent);

	static FloatingTileContent* createNewPanel(const Identifier& id, FloatingTile* parent);

	/** Set a custom title to the panel that will be displayed in tabs, etc. */
	void setCustomTitle(String newCustomTitle)
	{
		customTitle = newCustomTitle;
	}

	/** If you set a custom title, this will return it. */
	String getCustomTitle() const
	{
		return customTitle;
	}

	bool hasCustomTitle() const { return customTitle.isNotEmpty(); }

	bool hasDynamicTitle() const { return dynamicTitle.isNotEmpty(); }

	/** Sets a title that is supposed to change during runtime. It will not be stored in the JSON data. */
	void setDynamicTitle(const String& newDynamicTitle);

	String getDynamicTitle() const { return dynamicTitle; }

	Font getFont() const
	{
		if (fontName.isEmpty())
			return GLOBAL_BOLD_FONT();

		return getMainController()->getFontFromString(fontName, fontSize);
	}

	/** This returns the title that is supposed to be displayed. */
	String getBestTitle() const
	{
		if (hasDynamicTitle())
			return getDynamicTitle();

		if (hasCustomTitle())
			return getCustomTitle();

		return getTitle();
	}

	BackendProcessorEditor* getMainPanel();

	const BackendProcessorEditor* getMainPanel() const;

	class Factory
	{
	public:

		enum class PopupMenuOptions
		{
			Cancel = 0,
			Empty,
			Spacer,
			VisibilityToggleBar,
			BigResizer,
			HorizontalTile,
			VerticalTile,
			Tabs,
			Note,
			MacroControls,
			MacroTable,
			MidiKeyboard,
			PresetBrowser,
			SampleConnector,
			SampleEditor,
			SampleMapEditor,
			SamplerTable,
			ScriptConnectorPanel,
			ScriptEditor,
			ScriptContent,
			InterfaceContent,
			TablePanel,
			SliderPackPanel,
			Plotter,
			Console,
			ApiCollection,
			ScriptWatchTable,
			ScriptComponentEditPanel,
			ModuleBrowser,
			PatchBrowser,
			FileBrowser,
			ImageTable,
			AudioFileTable,
			SamplePoolTable,
			PopoutButton,
			PerformanceStatistics,
			ActivityLed,
			ActivationPanel,
			TuningWindow,
			PluginSettings,
			TooltipPanel,
			MidiSourceList,
			MidiChannelList,
			SampleMapBrowser,
			Matrix2x2,
			ThreeColumns,
			ThreeRows,
			toggleLayoutMode,
			toggleGlobalLayoutMode,
			exportAsJSON,
			loadFromJSON,
			MenuCommandOffset = 10000,

			numOptions
		};

		/** Register a subclass to this factory. The subclass must have a static method 'Identifier getName()'. */
		template <typename DerivedClass> void registerType(PopupMenuOptions index)
		{
			if (std::is_base_of<FloatingTileContent, DerivedClass>::value)
			{
				ids.add(DerivedClass::getPanelId());
				idIndexes.add(index);
				functions.add(&createFunc<DerivedClass>);
			}
		}

		/** Creates a subclass instance with the registered Identifier and returns a base class pointer to this. You need to take care of the ownership of course. */
		FloatingTileContent* createFromId(const Identifier &id, FloatingTile* parent) const
		{
			const int index = ids.indexOf(id);

			if (index != -1) return functions[index](parent);
			else
			{
				jassertfalse;
				return functions[0](parent);
			}
		}

		PopupMenuOptions getOption(const FloatingTile* t) const;

		/** Returns the list of all registered items. */
		const Array<Identifier> &getIdList() const { return ids; }

		void handlePopupMenu(PopupMenu& m, FloatingTile* parent);

		void registerAllPanelTypes();

		Drawable* getIcon(PopupMenuOptions type) const;

		static Path getPath(PopupMenuOptions path);

		void addToPopupMenu(PopupMenu& m, PopupMenuOptions type, const String& name, bool isEnabled=true, bool isTicked=false);

	private:

		/** @internal */
		template <typename DerivedClass> static FloatingTileContent* createFunc(FloatingTile* parent) { return new DerivedClass(parent); }

		/** @internal */
		typedef FloatingTileContent* (*PCreateFunc)(FloatingTile*);

		Array<Identifier> ids;
		Array<PopupMenuOptions> idIndexes;
		Array <PCreateFunc> functions;;
	};

	/** @internal */
	int getFixedSizeForOrientation() const;

	/** Overwrite this method if your component should hide the "title" when not in layout mode.
	*
	*	This will make the parent shell use the full area for this component so make sure it doesn't interfer with the shell buttons.
	*/
	virtual bool showTitleInPresentationMode() const { return true; };

	void setStyleProperty(Identifier id, var value)
	{
		styleData.getDynamicObject()->setProperty(id, value);
	}

	/** Override this method when you want to be notified when the amount of siblings change. */
	virtual void siblingAmountChanged() {}

	var getStyleProperty(Identifier id, var defaultValue) const
	{
		if (styleData.getDynamicObject()->hasProperty(id))
			return styleData.getDynamicObject()->getProperty(id);
		else
			return defaultValue;
	}

	
protected:

	/** Overwrite this method if the component has a fixed width. */
	virtual int getFixedWidth() const { return 0; }

	/** Overwrite this method if the component has a fixed height. */
	virtual int getFixedHeight() const { return 0; }
	
private:

	struct ColourHolder : public ObjectWithDefaultProperties
	{
		enum class ColourId
		{
			bgColour,
			textColour,
			itemColour1,
			itemColour2,
			itemColour3,
			numColourIds
		};

		ColourHolder()
		{
			for (int i = 0; i < (int)ColourId::numColourIds; i++)
			{
				colours[i] = Colours::pink;
				defaultColours[i] = Colours::pink;
			}	
		}

		int getNumDefaultableProperties() const override { return (int)ColourId::numColourIds; };

		Identifier getDefaultablePropertyId(int i) const override
		{
			switch ((ColourId)i)
			{
			case ColourId::bgColour: return "bgColour";
			case ColourId::textColour: return "textColour";
			case ColourId::itemColour1: return "itemColour1";
			case ColourId::itemColour2: return "itemColour2";
			case ColourId::itemColour3: return "itemColour3";
			default: jassertfalse;  return Identifier();
			}
		}

		var getDefaultProperty(int id) const override
		{
			auto c = defaultColours[id];

			//jassert(c != Colours::pink);

			return var(c.toString());
		}

		Colour getColour(ColourId id) const
		{
			if (id < ColourId::numColourIds)
			{
				return colours[(int)id];
			}
			else
			{
				jassertfalse;
				return Colours::transparentBlack;
			}
		}

		void setColour(ColourId id, Colour newColour)
		{
			if (id < ColourId::numColourIds)
			{
				colours[(int)id] = newColour;
			}
		}

		void setDefaultColour(ColourId id, Colour newColour)
		{
			if (id < ColourId::numColourIds)
			{
				defaultColours[(int)id] = newColour;
			}
		}

		var toDynamicObject() const override
		{
			DynamicObject::Ptr o = new DynamicObject();

			var obj(o);

			for (int i = 0; i < (int)ColourId::numColourIds; i++)
			{
				storePropertyInObject(obj, i, colours[i].toString(), defaultColours[i].toString());
			}

			return obj;
		}

		void fromDynamicObject(const var& object) override
		{
			for (int i = 0; i < (int)ColourId::numColourIds; i++)
			{
				auto s = getPropertyWithDefault(object, i).toString();

				colours[i] = Colour::fromString(s);
			}
				
		}

		Colour colours[(int)ColourId::numColourIds];

		Colour defaultColours[(int)ColourId::numColourIds];
	};

	ColourHolder colourData;

	String fontName;
	float fontSize = 14.0f;

	var styleData;

	Component::SafePointer<FloatingTile> parent;
	
	String customTitle;
	String dynamicTitle;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingTileContent)

	public:

	typedef ColourHolder::ColourId PanelColourId;

	void setPanelColour(PanelColourId id, Colour newColour)
	{
		colourData.setColour(id, newColour);
	}

	void setDefaultPanelColour(PanelColourId id, Colour newColour)
	{
		colourData.setDefaultColour(id, newColour);
		colourData.setColour(id, newColour);
	}


	Colour findPanelColour(PanelColourId colourId)
	{
		return colourData.getColour(colourId);
	}

};




struct FloatingPanelTemplates
{
	static Component* createHiseLayout(FloatingTile* root);

	static Component* createMainPanel(FloatingTile* root);

	static Component* createScriptingWorkspace(FloatingTile* root);

	static Component* createSamplerWorkspace(FloatingTile* root);

	static var createSettingsWindow(MainController* mc);

	static void create2x2Matrix(FloatingTile* parent);

	static void create3Columns(FloatingTile* parent);

	static void create3Rows(FloatingTile* parent);

	struct Helpers;
	
};




#endif  // FLOATINGTILECONTENT_H_INCLUDED
