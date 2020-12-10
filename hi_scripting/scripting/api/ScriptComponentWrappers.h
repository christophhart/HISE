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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SCRIPT_COMPONENT_WRAPPERS_H_INCLUDED
#define SCRIPT_COMPONENT_WRAPPERS_H_INCLUDED

namespace hise { using namespace juce;

class ScriptedControlAudioParameter : public AudioProcessorParameterWithID,
									  public AsyncUpdater
{
public:

	// ================================================================================================================

	enum class Type
	{
		Slider = 0,
		Button,
		ComboBox,
		Panel,
		Unsupported
	};

	ScriptedControlAudioParameter(ScriptingApi::Content::ScriptComponent *newComponent, 
								  AudioProcessor *parentProcessor, 
								  ScriptBaseMidiProcessor *scriptProcessor, 
								  int index);

	void setControlledScriptComponent(ScriptingApi::Content::ScriptComponent *newComponent);

	void handleAsyncUpdate() override
	{
		setParameterNotifyingHostInternal(indexForHost, valueForHost);
	}

	// ================================================================================================================

	float getValue() const override;
	void setValue(float newValue) override;
	float getDefaultValue() const override;

	String getLabel() const override;
	String getText(float value, int) const override;

	float getValueForText(const String &text) const override;
	int getNumSteps() const override;

	void setParameterNotifyingHost(int index, float newValue);
	bool isAutomatable() const override { return true; };

    
    bool isMetaParameter() const override;
    
	static Type getType(ScriptingApi::Content::ScriptComponent *component);

	static String getNameForComponent(ScriptingApi::Content::ScriptComponent *component)
	{
		const String givenName = component->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::pluginParameterName);

		if (givenName.isEmpty()) return component->getName().toString();
		else return givenName;
	}

	Identifier getId() const { return id; }

	void deactivateUpdateForNextSetValue() { deactivated = true; }

private:

	void setParameterNotifyingHostInternal(int index, float newValue);

	float valueForHost = 0.0f;
	int indexForHost = -1;

	// ================================================================================================================

	bool deactivated;
	const Identifier id;
	NormalisableRange<float> range;
	Type type;
	AudioProcessor *parentProcessor;
	WeakReference<Processor> scriptProcessor;
	int componentIndex;
	String suffix;
	StringArray itemList;
    bool isMeta = false;
    
	float lastValue = -1.0f;
	bool lastValueInitialised = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptedControlAudioParameter);

	// ================================================================================================================
};



class ScriptContentComponent;


#define DECLARE_ID(x) const Identifier x(#x);
namespace ScriptComponentProperties
{
DECLARE_ID(x);
DECLARE_ID(y);
DECLARE_ID(width);
DECLARE_ID(height);

}
#undef DECLARE_ID

/** A baseclass for the wrappers of script created components.
*
*	They usually contain the component and are subclassed from their listener. In their listener callback
*	you simply call changed() with whatever new value comes in.
*/
class ScriptCreatedComponentWrapper: public AsyncValueTreePropertyListener
{
public:

	class ValuePopup : public Component,
					   public Timer
	{
	public:

		class Properties : public ObjectWithDefaultProperties,
						   public ControlledObject,
						   public ReferenceCountedObject
		{
		public:

			typedef ReferenceCountedObjectPtr<Properties> Ptr ;

			struct LayoutData
			{
				float radius;
				float lineThickness;
				float margin;
			};

			enum ID
			{
				fontName,
				fontSize,
				borderSize,
				borderRadius,
				margin,
				bgColour,
				itemColour,
				itemColour2,
				textColour,
				numIds
			};

			Properties(MainController* mc, const var& data):
				ControlledObject(mc)
			{

				setDefaultValues({
					{"fontName", "Default"},
					{"fontSize", 14.0},
					{"borderSize", 2.0 },
					{"borderRadius", 2.0},
					{"margin", 3.0},
					{"bgColour", 0xFFFFFFFF},
					{"itemColour", 0xaa222222 },
					{"itemColour2", 0xaa222222 },
					{"textColour", 0xFFFFFFFF}
				});

				setValueList({ fName, fSize, bSize_, bRadius, mrgin, bg, item, item2, text });
				fromDynamicObject(data);
			}

			void fromDynamicObject(const var& data) override
			{
				ObjectWithDefaultProperties::fromDynamicObject(data);
				f = getMainController()->getFontFromString(fName.toString(), fSize.getValue());
			}

			Colour getColour(ID id)
			{
				switch (id)
				{
				case bgColour: return getColourFrom(bg);
				case itemColour: return getColourFrom(item);
				case itemColour2: return getColourFrom(item2);
				case textColour: return getColourFrom(text);
                default: break;
				}
					
				jassertfalse;
				return Colours::transparentBlack;
			}

			Font getFont() const { return f; }

			LayoutData getLayoutData() const { return { bRadius.getValue(), bSize_.getValue(), mrgin.getValue() }; }

			

		private:

			Font f;
			Value fName, fSize, bSize_, bRadius, mrgin, bg, item, item2, text;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Properties);
		};

		ValuePopup(ScriptCreatedComponentWrapper& p):
			parent(p),
			shadow({ Colours::black, 10,{ 0, 0 } })
		{
			f = GLOBAL_BOLD_FONT();

			shadow.setOwner(this);

			updateText();
			startTimer(30);
		}

		void setFont(Font newFont)
		{
			f = newFont;
		}

		void updateText();

		void timerCallback() override
		{
			updateText();
		}

		void paint(Graphics& g) override;

		Colour bgColour;
		Colour itemColour;
		Colour itemColour2;
		Colour textColour;

		String currentText;

		Font f;

		ScriptCreatedComponentWrapper& parent;

		DropShadower shadow;
	};

	/** Don't forget to deregister the listener here. */
	virtual ~ScriptCreatedComponentWrapper();;

	/** Overwrite this method and update the component. */
	virtual void updateComponent() = 0;

	/** Overwrite this method and update the component for the property that has changed. */
	virtual void updateComponent(int index, var newValue);

	/** Overwrite this method and update the value of the component. */
	virtual void updateValue(var newValue) {};

	/** Call this in your listener callback with the new value. */
	void changed(var newValue);

	Component *getComponent() { return component; }

	const Component *getComponent() const { return component; }

	virtual void asyncValueTreePropertyChanged(ValueTree& v, const Identifier& id);

	virtual void valueTreeParentChanged(ValueTree& v) override;

	int getIndex() const { return index; }

	ScriptingApi::Content::ScriptComponent *getScriptComponent()
	{
		return scriptComponent;
	};

	const ScriptingApi::Content::ScriptComponent *getScriptComponent() const
	{
		return scriptComponent;
	};

	ScopedPointer<ValuePopup> currentPopup;

protected:

	/** You need to do this tasks in your constructor:
	*
	*	1. Create the component and setup the component with every property you need.
	*	2. Add the wrapper itself as listener to the component (if you want the control callback).
	*/
	ScriptCreatedComponentWrapper(ScriptContentComponent *content, int index_);
	
	/** Call this constructor for a content without data management. */
	ScriptCreatedComponentWrapper(ScriptContentComponent* content, ScriptComponent* sc);

	Processor *getProcessor();

	ScriptingApi::Content *getContent();

	void initAllProperties();

	void showValuePopup();

	void updatePopupPosition();

	virtual Point<int> getValuePopupPosition(Rectangle<int> /*componentBounds*/) const { return Point<int>(); }

	virtual String getTextForValuePopup()
	{
		jassertfalse;
		return ""; 
	};

	/** the component that will be owned by this wrapper. */
	ScopedPointer<Component> component;

	/** the parent component. */
	ScriptContentComponent *contentComponent;

	void closeValuePopupAfterDelay();

private:

	struct ValuePopupHandler : public Timer
	{
		ValuePopupHandler(ScriptCreatedComponentWrapper& p):
			parent(p)
		{

		}

		void timerCallback() override
		{
			if (auto c = parent.getComponent())
			{
				Desktop::getInstance().getAnimator().fadeOut(parent.currentPopup, 200);

				auto parentTile = c->findParentComponentOfClass<FloatingTile>(); // always in a tile...

				if (parentTile == nullptr)
				{
					// Ouch...
					jassertfalse;
					return;
				}

				parentTile->removeChildComponent(parent.currentPopup);
				parent.currentPopup = nullptr;

				stopTimer();
			}
		}

		ScriptCreatedComponentWrapper& parent;
	};

	ValuePopupHandler valuePopupHandler;

	ScriptingApi::Content::ScriptComponent::Ptr scriptComponent;

	const int index;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptCreatedComponentWrapper)
};


class ScriptComponentPropertyTypeSelector
{
public:

	struct SliderRange
	{
		double min, max, interval;
	};


	enum SelectorTypes
	{
		ToggleSelector = 0,
		ColourPickerSelector,
		SliderSelector,
		ChoiceSelector,
		MultilineSelector,
		TextSelector,
		FileSelector,
		CodeSelector,
		numSelectorTypes
	};

	static SelectorTypes getTypeForId(const Identifier &id)
	{
		if (toggleProperties.contains(id)) return ToggleSelector;
		else if (sliderProperties.contains(id)) return SliderSelector;
		else if (colourProperties.contains(id)) return ColourPickerSelector;
		else if (choiceProperties.contains(id)) return ChoiceSelector;
		else if (multilineProperties.contains(id)) return MultilineSelector;
		else if (fileProperties.contains(id)) return FileSelector;
		else if (codeProperties.contains(id)) return CodeSelector;
		else return TextSelector;
	}

	static void addToTypeSelector(SelectorTypes type, Identifier id, double min=0.0, double max=1.0, double interval=0.01)
	{
		switch (type)
		{
		case ScriptComponentPropertyTypeSelector::ToggleSelector:
			toggleProperties.addIfNotAlreadyThere(id);
			break;
		case ScriptComponentPropertyTypeSelector::ColourPickerSelector:
			colourProperties.addIfNotAlreadyThere(id);
			break;
		case ScriptComponentPropertyTypeSelector::SliderSelector:
			{
			sliderProperties.addIfNotAlreadyThere(id);
			int index = sliderProperties.indexOf(id);
			SliderRange range;

			range.min = min;
			range.max = max;
			range.interval = interval;

			sliderRanges.set(index, range);
			break;
			}
		case ScriptComponentPropertyTypeSelector::ChoiceSelector:
			choiceProperties.addIfNotAlreadyThere(id);
			break;
		case ScriptComponentPropertyTypeSelector::MultilineSelector:
			multilineProperties.addIfNotAlreadyThere(id);
			break;
		case ScriptComponentPropertyTypeSelector::FileSelector:
			fileProperties.addIfNotAlreadyThere(id);
			break;
		case ScriptComponentPropertyTypeSelector::TextSelector:
			break;
		case ScriptComponentPropertyTypeSelector::CodeSelector:
			codeProperties.addIfNotAlreadyThere(id);
		case ScriptComponentPropertyTypeSelector::numSelectorTypes:
			break;
		default:
			break;
		}
	}


	static SliderRange getRangeForId(const Identifier &id)
	{
		return sliderRanges[sliderProperties.indexOf(id)];
	}

private:

	static Array<Identifier> toggleProperties;
	static Array<Identifier> sliderProperties;
	static Array<Identifier> colourProperties;
	static Array<Identifier> choiceProperties;
	static Array<Identifier> multilineProperties;
	static Array<Identifier> fileProperties;
	static Array<Identifier> codeProperties;

	static Array<SliderRange> sliderRanges;

};


class ScriptCreatedComponentWrappers
{
public:

	class SliderWrapper: public ScriptCreatedComponentWrapper,
						 public SliderListener
	{
	public:
		SliderWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSlider *scriptSlider, int index);

		void updateComponent() override;
		void updateComponent(int index, var newValue) override;

		void updateValue(var newValue) override;

		void sliderValueChanged(Slider *s) override;

		void updateTooltip(Slider * s);

		void sliderDragStarted(Slider* s) override;

		void sliderDragEnded(Slider* s) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderWrapper);

		Point<int> getValuePopupPosition(Rectangle<int> componentBounds) const;

		String getTextForValuePopup() override;

	private:

		

		void updateFilmstrip();
		void updateColours(HiSlider * s);
		void updateSensitivity(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s);
		void updateSliderRange(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s);
		void updateSliderStyle(ScriptingApi::Content::ScriptSlider* sc, HiSlider * s);

		String filmStripName;
		int numStrips = 0;
		double scaleFactor = 1.0;
	};

	class ButtonWrapper : public ScriptCreatedComponentWrapper,
						  public ButtonListener
	{
	public:

		ButtonWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptButton *sb, int index);

		~ButtonWrapper();

		void updateFilmstrip(HiToggleButton* b, ScriptingApi::Content::ScriptButton* sb);

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		void updateColours(HiToggleButton * b);

		void updateValue(var newValue) override;

		void buttonClicked(Button* ) override { /*changed(b->getToggleState());*/ };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonWrapper)
	};

	class LabelWrapper : public ScriptCreatedComponentWrapper,
						 public LabelListener
	{
	public:

		LabelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptLabel *scriptComboBox, int index);

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;
		void labelTextChanged(Label *l) override;;

		void updateValue(var newValue) override;

	private:

		void updateEditability(ScriptingApi::Content::ScriptLabel * sl, MultilineLabel * l);
		void updateFont(ScriptingApi::Content::ScriptLabel * sl, MultilineLabel * l);
		void updateColours(MultilineLabel * l);

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelWrapper)
	};
	

	class ComboBoxWrapper: public ScriptCreatedComponentWrapper,
						   public ComboBoxListener
	{
	public:

		ComboBoxWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptComboBox *scriptComboBox, int index);

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		void updateFont(ScriptComponent* cb);

		void updateValue(var newValue) override;

		void comboBoxChanged(ComboBox* ) override { /*changed(c->getText());*/ };

	private:

		PopupLookAndFeel plaf;

		void updateItems(HiComboBox * cb);
		void updateColours(HiComboBox * cb);

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBoxWrapper)
	};

	class TableWrapper : public ScriptCreatedComponentWrapper,
						 public TableEditor::Listener
		
	{
	public:

		TableWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptTable *table, int index);

		~TableWrapper();

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		void updateConnectedTable(TableEditor * t);
		
		String getTextForTablePopup(float x, float y);

		void pointDragStarted(Point<int> position, float index, float value) override;
		void pointDragEnded() override;
		void pointDragged(Point<int> position, float index, float value) override;
		void curveChanged(Point<int> position, float curveValue) override;

		Point<int> getValuePopupPosition(Rectangle<int> componentBounds) const override;

		String getTextForValuePopup() override { return popupText; }

	private:

		String popupText;
		Point<int> localPopupPosition;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableWrapper)
	};


	class ImageWrapper : public ScriptCreatedComponentWrapper,
                         public MouseCallbackComponent::Listener
	{
	public:

		ImageWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptImage *image, int index);

		void updateComponent() override;
        
		void updateComponent(int index, var newValue);

		void updateImage(ImageComponentWithMouseCallback * ic, ScriptingApi::Content::ScriptImage * si);

		void updatePopupMenu(ScriptingApi::Content::ScriptImage * si, ImageComponentWithMouseCallback * ic);

        void mouseCallback(const var &mouseInformation) override;
        
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageWrapper)
	};

	class PanelWrapper : public ScriptCreatedComponentWrapper,
                         public MouseCallbackComponent::Listener,
						 public MouseCallbackComponent::RectangleConstrainer::Listener,
						 public ScriptComponent::SubComponentListener
	{
	public:

		PanelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPanel *panel, int index);

		PanelWrapper(ScriptContentComponent* content, ScriptingApi::Content::ScriptPanel* panel);

		~PanelWrapper();

		OwnedArray<PanelWrapper> childPanelWrappers;
		
		void subComponentAdded(ScriptComponent* newComponent) override;
		void subComponentRemoved(ScriptComponent* componentAboutToBeRemoved) override;

		void initPanel(ScriptingApi::Content::ScriptPanel* panel);

		void rebuildChildPanels();

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		void updateRange(BorderPanel * bpc);

		void updateColourAndBorder(BorderPanel * bpc);

		void updateValue(var newValue) override;

		void mouseCallback(const var &mouseInformation) override;

		void boundsChanged(const Rectangle<int> &newBounds) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelWrapper)
	};


	class ViewportWrapper : public ScriptCreatedComponentWrapper
	{
	public:

		ViewportWrapper(ScriptContentComponent* content, ScriptingApi::Content::ScriptedViewport* viewport, int index);
		~ViewportWrapper();

		void updateComponent() override;
		void updateComponent(int index, var newValue) override;
		void updateValue(var newValue) override;

	private:

		void updateItems(ScriptingApi::Content::ScriptedViewport * vpc);
		void updateColours();
		void updateFont(ScriptingApi::Content::ScriptedViewport * vpc);

		class ColumnListBoxModel : public ListBoxModel
		{
		public:
			ColumnListBoxModel(ViewportWrapper* parent);

			int getNumRows() override;


			void listBoxItemClicked(int row, const MouseEvent &) override;
			void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
			void returnKeyPressed(int row) override;
			
			bool shouldUpdate(const StringArray& newItems)
			{
				return list != newItems;
			}

			void setItems(const StringArray& newItems)
			{
				list.clear();
				list.addArray(newItems);
			}

			Justification justification;

			Colour bgColour;
			Colour itemColour1;
			Colour itemColour2;
			Colour textColour;
			
			Font font;
			
			ViewportWrapper* parent;
			StringArray list;
		};

		bool shouldUseList = false;

		ScopedPointer<ColumnListBoxModel> model;
		ScopedPointer<LookAndFeel> slaf;

	};

	class SliderPackWrapper : public ScriptCreatedComponentWrapper,
							  public SliderPack::Listener
	{
	public:

		SliderPackWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSliderPack *pack, int index);

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		
		void updateValue(var newValue) override;

		void sliderPackChanged(SliderPack *, int newIndex) override { changed(newIndex); };

	private:

		void updateColours(SliderPack * sp);
		void updateRange(SliderPackData* data);


		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPackWrapper)
	};
	

	class AudioWaveformWrapper : public ScriptCreatedComponentWrapper,
								 public AudioDisplayComponent::Listener,
								 public Timer
	{
	public:

		

		AudioWaveformWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptAudioWaveform *waveform, int index);

		~AudioWaveformWrapper();

		void updateComponent() override;

		void updateComponent(int index, var newValue) override;

		void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea);

		void timerCallback() override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioWaveformWrapper)
	private:

        void updateSampleIndex(ScriptingApi::Content::ScriptAudioWaveform *waveform, AudioDisplayComponent* asb, int newValue);
        
		class SamplerListener;

		ScopedPointer<SamplerListener> samplerListener;

		void updateColours(AudioDisplayComponent* asb);
        
        int lastIndex = -1;
		ScriptingObjects::ScriptAudioFile::Ptr saf;
	};

	class FloatingTileWrapper : public ScriptCreatedComponentWrapper
	{
	public:

		FloatingTileWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptFloatingTile *floatingTile, int index);

		void updateComponent() override;
		void updateComponent(int index, var newValue) override;
		void updateValue(var newValue) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingTileWrapper)
	};


};

} // namespace hise

#endif // SCRIPT_COMPONENT_WRAPPERS_H_INCLUDED
