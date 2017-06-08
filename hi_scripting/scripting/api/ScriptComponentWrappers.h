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


class ScriptedControlAudioParameter : public AudioProcessorParameterWithID
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

	float lastValue = -1.0f;
	bool lastValueInitialised = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptedControlAudioParameter);

	// ================================================================================================================
};

class ScriptContentComponent;

/** A baseclass for the wrappers of script created components.
*
*	They usually contain the component and are subclassed from their listener. In their listener callback
*	you simply call changed() with whatever new value comes in.
*/
class ScriptCreatedComponentWrapper
{
public:

	/** Don't forget to deregister the listener here. */
	virtual ~ScriptCreatedComponentWrapper() {};

	/** Overwrite this method and update the component. */
	virtual void updateComponent() = 0;

	/** Call this in your listener callback with the new value. */
	void changed(var newValue);

	Component *getComponent() { return component; }

	

	int getIndex() const { return index; }

protected:

	/** You need to do this tasks in your constructor:
	*
	*	1. Create the component and setup the component with every property you need.
	*	2. Add the wrapper itself as listener to the component (if you want the control callback).
	*/
	ScriptCreatedComponentWrapper(ScriptContentComponent *content, int index_):
		contentComponent(content),
		index(index_)
	{

	}
	
	Processor *getProcessor();;

	ScriptingApi::Content *getContent();

	ScriptingApi::Content::ScriptComponent *getScriptComponent() { return getContent()->getComponent(getIndex()); };

	/** the component that will be owned by this wrapper. */
	ScopedPointer<Component> component;

	/** the parent component. */
	ScriptContentComponent *contentComponent;

private:

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

	static Array<SliderRange> sliderRanges;

};

class ScriptComponentEditListener
{
public:

	virtual ~ScriptComponentEditListener()
	{
		masterReference.clear();
	}

	virtual void scriptComponentChanged(ReferenceCountedObject *componentThatWasChanged, Identifier idThatWasChanged) = 0;

private:

	friend class WeakReference<ScriptComponentEditListener>;
	WeakReference<ScriptComponentEditListener>::Master masterReference;

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
		void sliderValueChanged(Slider *s) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderWrapper)
	};

	class ButtonWrapper : public ScriptCreatedComponentWrapper,
						  public ButtonListener
	{
	public:

		ButtonWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptButton *sb, int index);

		void updateComponent() override;

		void buttonClicked(Button* ) override { /*changed(b->getToggleState());*/ };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonWrapper)
	};

	class LabelWrapper : public ScriptCreatedComponentWrapper,
						 public LabelListener
	{
	public:

		LabelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptLabel *scriptComboBox, int index);

		void updateComponent() override;

		void labelTextChanged(Label *l) override { changed(l->getText()); };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelWrapper)
	};
	

	class ComboBoxWrapper: public ScriptCreatedComponentWrapper,
						   public ComboBoxListener
	{
	public:

		ComboBoxWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptComboBox *scriptComboBox, int index);

		void updateComponent() override;

		void comboBoxChanged(ComboBox* ) override { /*changed(c->getText());*/ };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBoxWrapper)
	};

	class TableWrapper : public ScriptCreatedComponentWrapper
	{
	public:

		TableWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptTable *table, int index);

		void updateComponent() override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableWrapper)
	};

	class ModulatorMeterWrapper : public ScriptCreatedComponentWrapper
	{
	public:

		ModulatorMeterWrapper(ScriptContentComponent *content, ScriptingApi::Content::ModulatorMeter *meter, int index);

		void updateComponent() override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorMeterWrapper)
	};
	
	class PlotterWrapper : public ScriptCreatedComponentWrapper
	{
	public:

		PlotterWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptedPlotter *plotter, int index);

		void updateComponent() override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlotterWrapper)
	};

	class ImageWrapper : public ScriptCreatedComponentWrapper,
                         public MouseCallbackComponent::Listener
	{
	public:

		ImageWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptImage *image, int index);

		void updateComponent() override;
        
        void mouseCallback(const var &mouseInformation) override;
        
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageWrapper)
	};

	class PanelWrapper : public ScriptCreatedComponentWrapper,
                         public MouseCallbackComponent::Listener,
						 public MouseCallbackComponent::RectangleConstrainer::Listener
	{
	public:

		PanelWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptPanel *panel, int index);
		~PanelWrapper();

		void updateComponent() override;

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
		

	};

	class SliderPackWrapper : public ScriptCreatedComponentWrapper,
							  public SliderPack::Listener
	{
	public:

		SliderPackWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptSliderPack *pack, int index);

		void updateComponent() override;

		void sliderPackChanged(SliderPack *, int newIndex) override { changed(newIndex); };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPackWrapper)
	};
	

	class AudioWaveformWrapper : public ScriptCreatedComponentWrapper,
								 public AudioDisplayComponent::Listener
	{
	public:

		AudioWaveformWrapper(ScriptContentComponent *content, ScriptingApi::Content::ScriptAudioWaveform *waveform, int index);

		void updateComponent() override;

		void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea);

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioWaveformWrapper)
	};

	

};

