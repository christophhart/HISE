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

#ifndef SCRIPTINGAPICONTENT_H_INCLUDED
#define SCRIPTINGAPICONTENT_H_INCLUDED




/** This is the interface area that can be filled with buttons, knobs, etc.
*	@ingroup scriptingApi
*
*/
class ScriptingApi::Content : public ScriptingObject,
	public DynamicObject,
	public SafeChangeBroadcaster,
	public RestorableObject
{
public:

	// ================================================================================================================

	class PluginParameterConnector
	{
	public:

		PluginParameterConnector() : parameter(nullptr), nextUpdateIsDeactivated(false) {}
		virtual ~PluginParameterConnector() {};

		bool isConnected() const { return parameter != nullptr; };
		void setConnected(ScriptedControlAudioParameter *controllingParameter);
		void sendParameterChangeNotification(float newValue);
		void deactivateNextUpdate() { nextUpdateIsDeactivated = true; }

	private:

		ScriptedControlAudioParameter *parameter;
		bool nextUpdateIsDeactivated;
	};

	struct ScriptComponent : public RestorableObject,
		public ConstScriptingObject,
		public SafeChangeBroadcaster,
		public AssignableObject,
		public DebugableObject
	{
		// ============================================================================================================

		enum Properties
		{
			text = 0,
			visible,
			enabled,
			x,
			y,
			width,
			height,
			min,
			max,
			tooltip,
			bgColour,
			itemColour,
			itemColour2,
			textColour,
			macroControl,
			zOrder,
			saveInPreset,
			isPluginParameter,
			numProperties
		};

		File getExternalFile(var newValue);

		ScriptComponent(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int width, int height, int numConstants=0);
		virtual ~ScriptComponent() {};

		virtual StringArray getOptionsFor(const Identifier &id);
		virtual ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) = 0;

		Identifier getName() const;
		Identifier getObjectName() const override;

		virtual ValueTree exportAsValueTree() const override;;
		virtual void restoreFromValueTree(const ValueTree &v) override;;

		String getDebugValue() const override { return getValue().toString(); };
		String getDebugName() const override { return name.toString(); };
		String getDebugDataType() const override { return getObjectName().toString(); }
		virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		var getAssignedValue(int index) const override;
		void assign(const int index, var newValue) override;
		int getCachedIndex(const var &indexExpression) const override;

		virtual void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification);
		virtual bool isAutomatable() const { return false; }

		const Identifier getIdFor(int p) const;
		int getNumIds() const;

		const var getScriptObjectProperty(int p) const;
		String getScriptObjectPropertiesAsJSON() const;
		DynamicObject *getScriptObjectProperties() const { return componentProperties.get(); }
		bool isPropertyDeactivated(Identifier &id) const;
		Rectangle<int> getPosition() const;

		// API Methods =====================================================================================================

		/** returns the value of the property. */
		var get(String propertyName) const { return componentProperties->getProperty(Identifier(propertyName)); }

		/** Sets the property. */
		void set(String propertyName, var value);;

		/** Returns the current value. */
		virtual var getValue() const { return value; }

		/** Sets the current value
		*
		*	It is safe to call this on the message callbacks, the control will be updated asynchronously.
		*	If you call this method within the onInit callback, it will not restore the value after compilation.
		*
		*	@param componentIndex the index of the component that was returned at the creation.
		*	@param newValue the new value.
		*/
		virtual void setValue(var newValue);

		/** Sets the current value from a range 0.0 ... 1.0. */
		virtual void setValueNormalized(double normalizedValue) { setValue(normalizedValue); };

		/** Returns the normalited value. */
		virtual double getValueNormalized() const { return getValue(); };

		/** sets the colour of the component (BG, IT1, IT2, TXT). */
		void setColour(int colourId, int colourAs32bitHex);

		/** Restores all properties from a JSON object. */
		void setPropertiesFromJSON(const var &jsonData);

		/** Sets the position of the component. */
		void setPosition(int x, int y, int w, int h);;

		/** Hides / Shows the control. */
		void showControl(bool shouldBeVisible);

		/** Shows a informative text on mouse hover. */
		void setTooltip(const String &tooltip);

		/** Adds the knob / button to a macro controller (from 0 to 7). */
		void addToMacroControl(int macroIndex);

		/** Returns the width of the component. */
		var getWidth() const;

		/** Returns the height of the component. */
		var getHeight() const;

		// End of API Methods ============================================================================================

		void setChanged(bool isChanged = true) noexcept{ changed = isChanged; }
		bool isChanged() const noexcept{ return changed; };

		var value;
		Identifier name;
		Content *parent;
		bool skipRestoring;

		struct Wrapper;

	protected:

		void setDefaultValue(int p, const var &defaultValue);
		void setScriptObjectProperty(int p, var value) { componentProperties->setProperty(getIdFor(p), value); }

		Array<Identifier> propertyIds;
		Array<Identifier> deactivatedProperties;
		Array<Identifier> priorityProperties;
		DynamicObject::Ptr componentProperties;

	private:

		NamedValueSet defaultValues;
		bool changed;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponent);
	};

	struct ScriptSlider : public ScriptComponent,
		public PluginParameterConnector
	{
	public:

		// ========================================================================================================

		enum Properties
		{
			Mode = ScriptComponent::Properties::numProperties,
			Style,
			stepSize,
			middlePosition,
			defaultValue,
			suffix,
			filmstripImage,
			numStrips,
			isVertical,
			scaleFactor,
			numProperties
		};

		ScriptSlider(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int, int);
		~ScriptSlider();

		Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptSlider") }
		virtual bool isAutomatable() const { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;

		// ======================================================================================================== API Methods

		/** Set the value from a 0.0 to 1.0 range */
		void setValueNormalized(double normalizedValue) override;

		double getValueNormalized() const override;

		/** Sets the range and the step size of the knob. */
		void setRange(double min, double max, double stepSize);

		/** Sets the knob to the specified mode. */
		void setMode(String mode);

		/** Sets the value that is shown in the middle position. */
		void setMidPoint(double valueForMidPoint);

		/** Sets the style Knob, Horizontal, Vertical. */
		void setStyle(String style);;

		/** Sets the lower range end to the given value. */
		void setMinValue(double min) noexcept;

		/** Sets the upper range end to the given value. */
		void setMaxValue(double max) noexcept;

		/** Returns the lower range end. */
		double getMinValue() const;

		/** Returns the upper range end. */
		double getMaxValue() const;

		/** Checks if the given value is within the range. */
		bool contains(double value);

		// ========================================================================================================

		struct Wrapper;

		HiSlider::Mode m;
		Slider::SliderStyle styleId;
		const Image *getImage() const { return image; };

	private:

		double minimum, maximum;
		Image const *image;
	};

	struct ScriptButton : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			filmstripImage = ScriptComponent::Properties::numProperties,
			isVertical,
			scaleFactor,
			radioGroup,
			isPluginParameter,
			numProperties
		};

		ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int, int);
		~ScriptButton();

		// ========================================================================================================

		Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptButton") }
		bool isAutomatable() const override { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		const Image *getImage() const { return image; };
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;

	private:

		// ========================================================================================================

		Image const *image;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptButton)
	};

	struct ScriptComboBox : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			Items = ScriptComponent::numProperties,
			isPluginParameter,
			numProperties
		};

		ScriptComboBox(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int);
		~ScriptComboBox() {};

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptComboBox"); }
		bool isAutomatable() const override { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification);
		StringArray getItemList() const;

		// ======================================================================================================== API Methods

		/** Returns the currently selected item text. */
		String getItemText() const;

		/** Adds an item to a combo box. */
		void addItem(const String &newName);

		// ========================================================================================================

		struct Wrapper;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComboBox);
	};


	struct ScriptLabel : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			FontName = ScriptComponent::Properties::numProperties,
			FontSize,
			FontStyle,
			Alignment,
			Editable,
			Multiline,
			numProperties
		};


		ScriptLabel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int);
		~ScriptLabel() {};
		
		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptLabel"); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		Justification getJustification();

		// ======================================================================================================== API Methods

		/** makes a label `editable`.
		> This is a test.
		*/
		void setEditable(bool shouldBeEditable);

		// ========================================================================================================

		struct Wrapper;
	};


	struct ScriptTable : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			TableIndex = ScriptComponent::Properties::numProperties,
			ProcessorId,
			numProperties
		};

		ScriptTable(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int height);
		~ScriptTable();

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { return "ScriptTable"; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;

		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;

		Table *getTable();
		const Table *getTable() const;
		LookupTableProcessor * getTableProcessor() const;

		// ======================================================================================================== API Method

		/** Returns the table value from 0.0 to 1.0 according to the input value from 0 to 127. */
		float getTableValue(int inputValue);

		/** Connects the table to an existing Processor. */
		void connectToOtherTable(const String &id, int index);

		// ========================================================================================================

		struct Wrapper;

	private:

		ScopedPointer<MidiTable> ownedTable;

		WeakReference<Table> referencedTable;
		WeakReference<Processor> connectedProcessor;

		bool useOtherTable;
		int lookupTableIndex;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptTable);
	};


	struct ScriptSliderPack : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			SliderAmount = ScriptComponent::Properties::numProperties,
			StepSize,
			FlashActive,
			ShowValueOverlay,
			ProcessorId,
			numProperties
		};

		ScriptSliderPack(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height);;
		~ScriptSliderPack();

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptSliderPack"); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		
		void setScriptProcessor(ProcessorWithScriptingContent *sb);
		SliderPackData *getSliderPackData();
		const SliderPackData *getSliderPackData() const;

		// ======================================================================================================== API Methods

		/** sets the slider value at the given index.*/
		void setSliderAtIndex(int index, double value);

		/** Returns the value at the given index. */
		double getSliderValueAt(int index);

		/** Sets all slider values to the given value. */
		void setAllValues(double value);

		/** Returns the number of sliders. */
		int getNumSliders() const;

		// ========================================================================================================

		struct Wrapper;

	private:

		void connectToOtherSliderPack(const String &otherPackId);

		ScopedPointer<SliderPackData> packData;
		WeakReference<SliderPackData> existingData;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptSliderPack);

		// ========================================================================================================
	};


	struct ScriptImage : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			Alpha = ScriptComponent::numProperties,
			FileName,
			Offset,
			Scale,
			AllowCallbacks,
			PopupMenuItems,
			PopupOnRightClick,
			numProperties
		};

		ScriptImage(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height);;
		~ScriptImage();

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptImage"); }
		virtual String getDebugValue() const override { return getScriptObjectProperty(Properties::FileName); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		StringArray getItemList() const;
		const Image *getImage() const;

		void setScriptProcessor(ProcessorWithScriptingContent *sb);

		// ======================================================================================================== API Method

		/** Sets the transparency (0.0 = full transparency, 1.0 = full opacity). */
		void setAlpha(float newAlphaValue);;

		/** Sets the image file that will be displayed. 
		*
		*	If forceUseRealFile is true, then it will reload the file from disk instead of caching it. */
		void setImageFile(const String &absoluteFileName, bool forceUseRealFile);

		

		// ========================================================================================================

		struct Wrapper;

	private:

		Image const *image;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptImage);

		// ========================================================================================================
	};

	struct ScriptPanel : public ScriptComponent,
						 public Timer
	{
		// ========================================================================================================

		enum Properties
		{
			borderSize = ScriptComponent::numProperties,
			borderRadius,
			allowCallbacks,
			PopupMenuItems,
			PopupOnRightClick,
			popupMenuAlign,
			numProperties
		};

		ScriptPanel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);;
		~ScriptPanel()
		{
			graphics = nullptr;

			timerRoutine = var::undefined();
			mouseRoutine = var::undefined();
			paintRoutine = var::undefined();

			
		}

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptPanel"); }
		
		StringArray getOptionsFor(const Identifier &id) override;
		StringArray getItemList() const;

		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		// ======================================================================================================== API Methods

		/** Triggers a repaint. */
		void repaint();

		/** Sets a paint routine (a function with one parameter). */
		void setPaintRoutine(var paintFunction);

		/** Sets a mouse callback. */
		void setMouseCallback(var mouseCallbackFunction);

		/** Sets a timer callback. */
		void setTimerCallback(var timerCallback);

		/** Call this to indicate that the value has changed (the onControl callback will be executed. */
		void changed();

		/** Loads a image which can be drawn with the paint function later on. */
		void loadImage(String imageName, String prettyName);

		void setCustom(String propertyName, var newValue);

		var getCustom(String propertyName);

		// ========================================================================================================

		struct Wrapper;

		Image getImage() const
		{
			return paintCanvas;
		}

		bool isUsingCustomPaintRoutine() const { return !paintRoutine.isUndefined(); }

		void mouseCallback(var mouseInformation);

		void timerCallback() override;

		const Image* getLoadedImage(const String &prettyName) const
		{
			for (int i = 0; i < loadedImages.size(); i++)
			{
				if (loadedImages[i].prettyName == prettyName)
					return loadedImages[i].image;
			}
		};

	private:

		void internalRepaint();

		struct AsyncControlCallbackSender : public AsyncUpdater
		{
			AsyncControlCallbackSender(ScriptPanel* parent_, ProcessorWithScriptingContent* p_) : parent(parent_), p(p_) {};

			void handleAsyncUpdate();

			ScriptPanel* parent;
			ProcessorWithScriptingContent* p;
		};

		struct AsyncRepainter : public AsyncUpdater
		{
			AsyncRepainter(ScriptPanel* parent_) : parent(parent_){}

			void handleAsyncUpdate()
			{
				parent->internalRepaint();
			}

			ScriptPanel* parent;
		};

		struct NamedImage
		{
			NamedImage(const Image* image_, const String &prettyName_ , const String &fileName_) : image(image_), prettyName(prettyName_), fileName(fileName_) {};
			
			NamedImage(): image(nullptr), fileName(String()), prettyName(String()) {}

			const Image* image;
			const String fileName;
			const String prettyName;
		};

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptPanel);

		ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> graphics;

		var paintRoutine = var::undefined();
		var mouseRoutine = var::undefined();
		var timerRoutine = var::undefined();

		DynamicObject::Ptr customProperties;

		Image paintCanvas;

		Array<NamedImage> loadedImages;

		AsyncRepainter repainter;
		AsyncControlCallbackSender controlSender;

		// ========================================================================================================
	};


	struct ScriptedPlotter : public ScriptComponent
	{
	public:

		// ========================================================================================================

		ScriptedPlotter(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height);;

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { return "ScriptedPlotter"; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		void addModulator(Modulator *m) { mods.add(m); };
		void clearModulators() { mods.clear(); };

		// ======================================================================================================== API Methods

		/** Searches a processor and adds the modulator to the plotter. */
		void addModulatorToPlotter(String processorName, String modulatorName);

		/** Removes all modulators from the plotter. */
		void clearModulatorPlotter();

		// ========================================================================================================

		struct Wrapper;

		Array<WeakReference<Modulator>> mods;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptedPlotter);

		// ========================================================================================================
	};

	struct ModulatorMeter : public ScriptComponent
	{

		// ========================================================================================================

		enum Properties
		{
			ModulatorId = ScriptComponent::numProperties,
			numProperties
		};

		ModulatorMeter(ProcessorWithScriptingContent *base, Content *parentContent, Identifier modulatorName, int x, int y, int width, int height);;
		~ModulatorMeter() {};

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ModulatorMeter"); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		void setScriptProcessor(ProcessorWithScriptingContent *sb);

		// ========================================================================================================

		WeakReference<Modulator> targetMod;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorMeter)
	};



	struct ScriptAudioWaveform : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			processorId = ScriptComponent::numProperties,
			numProperties
		};

		ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height);
		~ScriptAudioWaveform() {};

		// ========================================================================================================

		virtual Identifier 	getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptAudioWaveform"); };
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		AudioSampleProcessor * getAudioProcessor();;
		void connectToAudioSampleProcessor(String processorId);

		// ========================================================================================================

	private:

		WeakReference<Processor> connectedProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptAudioWaveform);

		// ========================================================================================================
	};


	struct ScriptPluginEditor : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			processorId = ScriptComponent::numProperties,
			numProperties
		};

		ScriptPluginEditor(ProcessorWithScriptingContent *base, Content*parentContent, Identifier name, int x, int y, int width, int height);
		~ScriptPluginEditor();

		// ========================================================================================================

		virtual Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptPluginEditor") };
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		void connectToAudioProcessorWrapper(String processorId);
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		AudioProcessorWrapper *getProcessor();

		// ========================================================================================================

	private:

		WeakReference<Processor> connectedProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptPluginEditor)
	};

	// ================================================================================================================

	Content(ProcessorWithScriptingContent *p);;
	~Content();

	static Identifier getClassName()   { RETURN_STATIC_IDENTIFIER("CONTENT") };

	// ================================================================================================================

	/** Adds a toggle button to the Content and returns the component index.
	*
	*	@param knobName the name for the knob. It should contain no whitespace
	*	@param x the x position
	*	@param y the y position
	*/
	ScriptButton *addButton(Identifier buttonName, int x, int y);

	/** Adds a knob to the Content and returns the component index.
	*
	*	@param knobName the name for the knob. It should contain no whitespace
	*	@param x the x position
	*	@param y the y position
	*/
	ScriptSlider *addKnob(Identifier knobName, int x, int y);

	/** Adds a table editor to the Content and returns the component index. */
	ScriptTable *addTable(Identifier tableName, int x, int y);

	/** Adds a comboBox to the Content and returns the component index. */
	ScriptComboBox *addComboBox(Identifier boxName, int x, int y);

	/** Adds a text input label. */
	ScriptLabel *addLabel(Identifier label, int x, int y);

	/** Adds a peak meter that displays the modulator's output. */
	ModulatorMeter *addModulatorMeter(Identifier modulatorName, int x, int y);

	/** Adds a plotter that plots multiple modulators. */
	ScriptedPlotter *addPlotter(Identifier plotterName, int x, int y);

	/** Adds a image to the script interface. */
	ScriptImage *addImage(Identifier imageName, int x, int y);

	/** Adds a panel (rectangle with border and gradient). */
	ScriptPanel *addPanel(Identifier panelName, int x, int y);

	/** Adds a audio waveform display. */
	ScriptAudioWaveform *addAudioWaveform(Identifier audioWaveformName, int x, int y);

	/** Adds a slider pack. */
	ScriptSliderPack *addSliderPack(Identifier sliderPackName, int x, int y);

	/** Adds a plugin editor window. */
	ScriptPluginEditor *addPluginEditor(Identifier pluginEditorName, int x, int y);

	/** Restore the widget from a JSON object. */
	void setPropertiesFromJSON(const Identifier &name, const var &jsonData);

	/** Creates a Path that can be drawn to a ScriptPanel. */
	var createPath();

	/** Sets the colour for the panel. */
	void setColour(int red, int green, int blue) { colour = Colour((uint8)red, (uint8)green, (uint8)blue); };

	/** Sets the height of the content. */
	void setHeight(int newHeight) noexcept;

	/** Sets the height of the content. */
	void setWidth(int newWidth) noexcept;

	/** sets the Tooltip that will be shown if the mouse hovers over the script's tab button. */
	void setContentTooltip(const String &tooltipToShow) { tooltip = tooltipToShow; }

	/** Sets the main toolbar properties from a JSON object. */
	void setToolbarProperties(const var &toolbarProperties);

	/** Sets the name that will be displayed in big fat Impact. */
	void setName(const String &newName)	{ name = newName; };

	/** Saves all controls that should be saved into a XML data file. */
	void storeAllControlsAsPreset(const String &fileName);

	/** Restores all controls from a previously saved XML data file. */
	void restoreAllControlsFromPreset(const String &fileName);

	// ================================================================================================================

	void restoreAllControlsFromPreset(const ValueTree &preset);
	Colour getColour() const { return colour; };
	void endInitialization();

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	bool isEmpty();
	int getNumComponents() const noexcept{ return components.size(); };
	ScriptComponent *getComponent(int index);;
	const ScriptComponent *getComponent(int index) const { return components[index]; };
	ScriptComponent * getComponentWithName(const Identifier &componentName);
	const ScriptComponent * getComponentWithName(const Identifier &componentName) const;

	struct Wrapper;

private:

	template<class Subtype> Subtype *addComponent(Identifier name, int x, int y, int width = -1, int height = -1);

	friend class ScriptContentComponent;
	friend class WeakReference<ScriptingApi::Content>;
	WeakReference<ScriptingApi::Content>::Master masterReference;

	CriticalSection lock;
	bool allowGuiCreation;
	int width, height;
	ReferenceCountedArray<ScriptComponent> components; // This is ref counted to allow anonymous controls
	Colour colour;
	String name;
	String tooltip;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Content);

	// ================================================================================================================
};



#endif  // SCRIPTINGAPICONTENT_H_INCLUDED
