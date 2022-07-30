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


#ifndef FLOATINGTILECONTENT_H_INCLUDED
#define FLOATINGTILECONTENT_H_INCLUDED

#define OLD_COLOUR 0

#ifndef HI_ENABLE_EXTERNAL_CUSTOM_TILES
#define HI_ENABLE_EXTERNAL_CUSTOM_TILES 0
#endif


#if HI_ENABLE_EXTERNAL_CUSTOM_TILES
#define REGISTER_EXTERNAL_FLOATING_TILE(x) registerType<x>(FloatingTileContent::Factory::PopupMenuOptions::numOptions);
#else
#define REGISTER_EXTERNAL_FLOATING_TILE(x)
#endif

namespace hise { using namespace juce;

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
	virtual var toDynamicObject() const
	{
		jassert(useCustomDefaultValues);

		var obj(new DynamicObject());
		saveValuesFromList(obj);
		return obj;
	}

	/** Restores the state of this object from a dynamic object. */
	virtual void fromDynamicObject(const var& objectData)
	{
		jassert(useCustomDefaultValues);
		loadValuesToList(objectData);
	}

	/** Overwrite this and get the default property. */
	virtual var getDefaultProperty(int id) const 
	{
		jassert(useCustomDefaultValues);

		return defaultValues.getValueAt(id);
	};
	
	var operator()(const var& data, int index) const
	{
		return getPropertyWithDefault(data, index);
	}

	void operator()(const var& data, int index, const var& value) const
	{
		storePropertyInObject(data, index, value);
	}

	/** Overwrite this and return the number of properties that are defaultable. */
	virtual int getNumDefaultableProperties() const
	{
		jassert(useCustomDefaultValues);

		return defaultValues.size();
	}
	
	/** Overwrite this and return the Identifier for the given property index. 
	*
	*	You might want to use the macro RETURN_DEFAULT_PROPERTY_ID(idToCheck, name) with a named enum for this. 
	*
	*/
	virtual Identifier getDefaultablePropertyId(int i) const
	{
		jassert(useCustomDefaultValues);

		return defaultValues.getName(i);
	}

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

		return getDefaultProperty(id);
	}

	void setDefaultValues(const NamedValueSet& newDefaultValues)
	{
		defaultValues = newDefaultValues;
		useCustomDefaultValues = true;
	}

	void setValueList(const Array<Value>& valueList)
	{
		jassert(useCustomDefaultValues);
		jassert(valueList.size() == defaultValues.size());

		values = valueList;
	}

	static Colour getColourFrom(const Value& colourValue)
	{
		return Colour((uint32)(int64)(colourValue.getValue()));
	}

protected:

	void saveValuesFromList(var& object) const
	{
		jassert(useCustomDefaultValues);

		for (int i = 0; i < getNumDefaultableProperties(); i++)
		{
			storePropertyInObject(object, i, values[i].getValue(), getDefaultProperty(i));
		}
	}

	void loadValuesToList(const var& object)
	{
		jassert(useCustomDefaultValues);

		for (int i = 0; i < getNumDefaultableProperties(); i++)
		{
			values[i].setValue(getPropertyWithDefault(object, i));
		}
	}

	NamedValueSet defaultValues;

	Array<Value> values;

	bool useCustomDefaultValues = false;
};


class JSONEditor : public Component,
				   public ButtonListener,
				   public CodeDocument::Listener
{
public:

	using F5Callback = std::function<void(const var&)>;

	using CompileCallback = std::function<Result(const String& text, var& data)>;

	JSONEditor(ObjectWithDefaultProperties* editedObject):
		editedComponent(dynamic_cast<Component*>(editedObject))
	{
		constructionTime = Time::getApproximateMillisecondCounter();

		setName("JSON Editor");

		tokeniser = new JavascriptTokeniser();
		doc = new CodeDocument();
		doc->replaceAllContent(JSON::toString(editedObject->toDynamicObject(), false, DOUBLE_TO_STRING_DIGITS));
		doc->setSavePoint();
		doc->clearUndoHistory();
		doc->addListener(this);

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

		addButtonAndLabel();

		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
	}

	JSONEditor(var object)
	{
		constructionTime = Time::getApproximateMillisecondCounter();

		auto s = JSON::toString(object, false, DOUBLE_TO_STRING_DIGITS);

		tokeniser = new JavascriptTokeniser();
		doc = new CodeDocument();
		doc->replaceAllContent(s);
		doc->setSavePoint();
		doc->clearUndoHistory();
		doc->addListener(this);

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
		editor->setReadOnly(true);
		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

		addButtonAndLabel();

		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

	}

	

	JSONEditor(const String& f, CodeTokeniser* t)
	{
		constructionTime = Time::getApproximateMillisecondCounter();

		setName("External Script Preview");

		tokeniser = t;
		doc = new CodeDocument();
		doc->replaceAllContent(f);
		doc->setSavePoint();
		doc->clearUndoHistory();
		doc->addListener(this);

		addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

		editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
		editor->setReadOnly(true);
		editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

		addButtonAndLabel();


		constrainer.setMinimumWidth(200);
		constrainer.setMinimumHeight(300);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
	}

	void addButtonAndLabel();

	void buttonClicked(Button* /*b*/) override
	{
		executeCallback();
	}

	void codeDocumentTextInserted(const String& /*newText*/, int /*insertIndex*/)
	{
		setChanged();
	}

	void codeDocumentTextDeleted(int /*startIndex*/, int /*endIndex*/)
	{
		setChanged();
	}

	void setChanged();

	~JSONEditor()
	{
		editor = nullptr;
		doc = nullptr;
	}

	bool keyPressed(const KeyPress& key)
	{
		

		if (key == KeyPress::F5Key)
		{
			if (callback)
			{
				executeCallback();
			}
			else
			{
				if (editedComponent == nullptr)
					return false;

				replace();
			}

			return true;
		}

		return false;
	}

	void setEditable(bool shouldBeEditable)
	{
		editor->setReadOnly(!shouldBeEditable);
	}

	void setDataToEdit(var newData)
	{
		doc->clearUndoHistory();
		doc->replaceAllContent(JSON::toString(newData, false, DOUBLE_TO_STRING_DIGITS));
	}

	void replace();
	
	void executeCallback();

	void setCallback(const F5Callback& newCallback, bool closeAfterExecution=false)
	{
		callback = newCallback;
		closeAfterCallbackExecution = closeAfterExecution;
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto bottomRow = b.removeFromBottom(24);

		applyButton->setBounds(bottomRow.removeFromRight(50));
		changeLabel->setBounds(bottomRow);

		editor->setBounds(b);
		resizer->setBounds(getWidth() - 12, getHeight() - 12, 12, 12);
	}

	static Result defaultJSONParse(const String& s, var& value)
	{
		return JSON::parse(s, value);
	}

	void setCompileCallback(const CompileCallback& c, bool closeAfterExecution)
	{
		compileCallback = c;
		closeAfterCallbackExecution = closeAfterExecution;
	}

private:

	uint32 constructionTime;

	CompileCallback compileCallback = defaultJSONParse;
	F5Callback callback;

	bool closeAfterCallbackExecution = false;

	Component::SafePointer<Component> editedComponent;

	ScopedPointer<ResizableCornerComponent> resizer;

	ComponentBoundsConstrainer constrainer;

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<CodeTokeniser> tokeniser;
	ScopedPointer<CodeEditorComponent> editor;

	ScopedPointer<Label> changeLabel;
	ScopedPointer<TextButton> applyButton;
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

Description

![%TYPE% Screenshot](http://hise.audio/images/floating_tile_gifs/%TYPE%.gif)

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour
`ColourData::bgColour`    | the background colour
`ColourData::itemColour1` | the first item colour
`Font`					  | the font
`FontSize`				  | the font size

### Example JSON

```
const var data = {%EXAMPLE_JSON};
```

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

		setDefaultPanelColour(PanelColourId::itemColour3, Colours::white.withAlpha(0.6f));

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
	void setCustomTitle(String newCustomTitle);

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
    String getBestTitle() const;
	

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
			MidiLearnPanel,
			MidiKeyboard,
			MidiPlayerOverlay,
			FrontendMacroPanel,
			PresetBrowser,
			SampleConnector,
			SampleEditor,
			SampleMapEditor,
			SamplerTable,
			ComplexDataManager,
			ScriptConnectorPanel,
			ScriptEditor,
			ScriptContent,
			ScriptComponentList,
			InterfaceContent,
			TablePanel,
			SliderPackPanel,
			Plotter,
			AudioAnalyser,
			Console,
			ApiCollection,
			RLottieDevPanel,
			ScriptWatchTable,
			ScriptComponentEditPanel,
			DspNetworkGraph,
			DspNodeList,
			DspNodeParameterEditor,
			ExpansionEditBar,
			ModuleBrowser,
			PatchBrowser,
			FileBrowser,
			ImageTable,
			AudioFileTable,
			SamplePoolTable,
			SampleMapPoolTable,
			MidiFilePoolTable,
			PopoutButton,
			PerformanceStatistics,
			ActivityLed,
            MatrixPeakMeterPanel,
			ActivationPanel,
			TuningWindow,
			PluginSettings,
			TooltipPanel,
			MidiSourceList,
			MidiChannelList,
			AboutPage,
			SampleMapBrowser,
			WavetablePreview,
			AHDSRGraph,
			MarkdownEditor,
			ServerController,
			MarkdownPreviewPanel,
			FilterGraphPanel,
			DraggableFilterPanel,
			MPEPanel,
			Matrix2x2,
			ThreeColumns,
			ThreeRows,
			toggleLayoutMode,
			toggleGlobalLayoutMode,
			exportAsJSON,
			loadFromJSON,
			SnexEditor,
			SnexOptimisations,
			SnexAssembly,
			SnexGraph,
			SnexParameterList,
			SnexWorkbenchInfo,
			SnexTestDataInfo,
			SnexComplexTestData,
			SnexWorkbenchPlayer,
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
		FloatingTileContent* createFromId(const Identifier &id, FloatingTile* parent) const;

		PopupMenuOptions getOption(const FloatingTile* t) const;

		/** Returns the list of all registered items. */
		const Array<Identifier> &getIdList() const { return ids; }

		void handlePopupMenu(PopupMenu& m, FloatingTile* parent);

		void registerLayoutPanelTypes();

		void registerAllPanelTypes();

		void registerFrontendPanelTypes();

#if HI_ENABLE_EXTERNAL_CUSTOM_TILES
		void registerExternalPanelTypes();
#endif

		std::unique_ptr<Drawable> getIcon(PopupMenuOptions type) const;

		static Path getPath(PopupMenuOptions path);

		void addToPopupMenu(PopupMenu& m, PopupMenuOptions type, const String& name, bool isEnabled=true, bool isTicked=false);

		PopupMenuOptions getIndex(int i) const { return idIndexes[i]; }

	private:

		/** @internal */
		template <typename DerivedClass> static FloatingTileContent* createFunc(FloatingTile* parent) { return new DerivedClass(parent); }

		/** @internal */
		typedef FloatingTileContent* (*PCreateFunc)(FloatingTile*);

		Array<Identifier> ids;
		Array<PopupMenuOptions> idIndexes;
		Array <PCreateFunc> functions;;
	};

	struct FloatingTilePathFactory: public hise::PathFactory
	{
		FloatingTilePathFactory()
		{
			f.registerAllPanelTypes();

			for (const auto& id : f.getIdList())
				ids.add(MarkdownLink::Helpers::getSanitizedFilename(id.toString()));
		}

		String getId() const override { return "FloatingTile Icons"; };

		Path createPath(const String& id) const override;

		StringArray sa;
		Factory f;
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

	int getPreferredHeight() const;

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
				colours[i] = Colours::transparentBlack;
				defaultColours[i] = Colours::transparentBlack;
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

		Colour getDefaultColour(ColourId id) const
		{
			if (id < ColourId::numColourIds)
				return defaultColours[(int)id];

			return Colours::transparentBlack;
		}

		var toDynamicObject() const override
		{
			auto o = new DynamicObject();

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
				auto colourVar = getPropertyWithDefault(object, i);

				if (colourVar.isString())
				{
					auto stringValue = colourVar.toString();

					// thanks to the awesomeness of XML, this could still be a number, yay...

					int64 normal = stringValue.getLargeIntValue();
					int64 asHex = stringValue.getHexValue64();

					if (stringValue.containsAnyOf("ABCDEFabcdefx"))
					{
						colours[i] = Colour((uint32)asHex);
					}
					else
					{
						colours[i] = Colour((uint32)normal);
					}
				}
				else if (colourVar.isInt64() || colourVar.isInt64())
				{
					colours[i] = Colour((uint32)(int64)colourVar);
				}
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

	Colour getDefaultPanelColour(PanelColourId id) const
	{
		return colourData.getDefaultColour(id);
	}


	Colour findPanelColour(PanelColourId colourId)
	{
		return colourData.getColour(colourId);
	}

};

#define DECLARE_ID(x) static const Identifier x(#x);

namespace FloatingTileKeyPressIds
{
	DECLARE_ID(fold_editor);
	DECLARE_ID(fold_interface);
	DECLARE_ID(fold_browser)
	DECLARE_ID(focus_editor);
	DECLARE_ID(fold_watch);
	DECLARE_ID(fold_list);
	DECLARE_ID(fold_console);
	DECLARE_ID(fold_properties);
	DECLARE_ID(focus_interface);
	DECLARE_ID(focus_browser)
	DECLARE_ID(cycle_editor);
	DECLARE_ID(cycle_browser)
}

#undef DECLARE_ID

struct FloatingPanelTemplates
{
	static Component* createHiseLayout(FloatingTile* root);

	static Component* createMainPanel(FloatingTile* root);

	static Component* createScriptingWorkspace(FloatingTile* root);

	static Component* createSamplerWorkspace(FloatingTile* root);

	static Component* createCodeEditorPanel(FloatingTile* root);
	static Component* createScriptnodeEditorPanel(FloatingTile* root);

	static var createSettingsWindow(MainController* mc);

	static void create2x2Matrix(FloatingTile* parent);

	static void create3Columns(FloatingTile* parent);

	static void create3Rows(FloatingTile* parent);

	struct Helpers;
	
};


} // namespace hise

#endif  // FLOATINGTILECONTENT_H_INCLUDED
