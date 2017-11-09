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

#ifndef SCRIPTINGAPICONTENT_H_INCLUDED
#define SCRIPTINGAPICONTENT_H_INCLUDED

namespace hise { using namespace juce;


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

	class RebuildListener
	{
	public:

		virtual ~RebuildListener()
		{
			masterReference.clear();
		}

		virtual void contentWasRebuilt() = 0;

	private:

		friend class WeakReference<RebuildListener>;

		WeakReference<RebuildListener>::Master masterReference;
	};

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
		using Ptr = ReferenceCountedObjectPtr<ScriptComponent>;

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
			saveInPreset,
			isPluginParameter,
			pluginParameterName,
			useUndoManager,
			parentComponent,
			processorId,
			parameterId,
			numProperties
		};

		File getExternalFile(var newValue);

		ScriptComponent(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int width, int height, int numConstants=0);
		
		virtual ~ScriptComponent() 
		{
			childComponents.clear();
		};

		virtual StringArray getOptionsFor(const Identifier &id);
		virtual ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) = 0;


		Identifier getName() const;
		Identifier getObjectName() const override;

		virtual void preRecompileCallback() {};

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

		var getNonDefaultScriptObjectProperties() const;

		String getScriptObjectPropertiesAsJSON() const;
		DynamicObject *getScriptObjectProperties() const { return componentProperties.get(); }
		bool isPropertyDeactivated(Identifier &id) const;
		Rectangle<int> getPosition() const;

		int getParentComponentIndex() const;

		bool addChildComponent(ScriptComponent* childComponent);

		/** Checks if the component is a child component.
		*
		*	It also looks in the child components' child componenents ... */
		bool isChildComponent(ScriptComponent* childComponent);

		int getNumChildComponents() const
		{
			return childComponents.size();
		}

		ScriptComponent* getChildComponent(int index)
		{
			if (index < childComponents.size())
				return childComponents[index].get();

			return nullptr;
		}

		ReferenceCountedObject* getCustomControlCallback();

		/** This updates the internal content data object from the script processor.
		*
		*	You should never use this, but use ScriptComponentEditBroadcaster::setPropertyForSelection() instead.
		*/
		void updateContentPropertyInternal(int propertyId, const var& newValue);

		void updateContentPropertyInternal(const Identifier& propertyId, const var& newValue);

		void notifyChildComponents();

		bool isShowing() const;

		template <class ChildType> class ChildIterator
		{
		public:

			ChildIterator(ScriptComponent* c)
			{
				addToArray(c);
			}

			ChildType* getNextChildComponent()
			{
				return childComponents[index++];
			}

		private:

			int index = 0;

			void addToArray(ScriptComponent* c)
			{
				if (auto cTyped = dynamic_cast<ChildType*>(c))
					childComponents.add(cTyped);

				for (int i = 0; i < c->getNumChildComponents(); i++)
					addToArray(c->getChildComponent(i));
			}

			Array<ChildType*> childComponents;
		};

		// API Methods =====================================================================================================

		/** returns the value of the property. */
		var get(String propertyName) const { return componentProperties->getProperty(Identifier(propertyName)); }

		/** Sets the property. */
		void set(String propertyName, var value);;

		/** Returns the current value. */
		virtual var getValue() const
        {
            jassert(!value.isString());
            return value;
        }

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

		/** Sets the current value and adds it to the undo list. Don't call this from onControl! */
		void setValueWithUndo(var newValue);

		/** Returns the normalized value. */
		virtual double getValueNormalized() const { return getValue(); };

		/** sets the colour of the component (BG, IT1, IT2, TXT). */
		void setColour(int colourId, int colourAs32bitHex);

        /** Returns the absolute x-position relative to the interface. */
        int getGlobalPositionX();
        
        /** Returns the absolute y-position relative to the interface. */
        int getGlobalPositionY();
        
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

		/** Pass a inline function for a custom callback event. */
		void setControlCallback(var controlFunction);

		// End of API Methods ============================================================================================

		

		void setChanged(bool isChanged = true) noexcept{ changed = isChanged; }
		bool isChanged() const noexcept{ return changed; };

		var value;
		Identifier name;
		Content *parent;
		bool skipRestoring;

		struct Wrapper;

		bool isConnectedToProcessor() const;;

		Processor* getConnectedProcessor() const { return connectedProcessor.get(); };

		int getConnectedParameterIndex() { return connectedParameterIndex; };

		struct ScopedPropertyEnabler
		{
			ScopedPropertyEnabler(ScriptComponent* c_) :
				c(c_)
			{
				c->countJsonSetProperties = false;
			};

			~ScopedPropertyEnabler()
			{
				c->countJsonSetProperties = true;
				c = nullptr;
			}

			ScriptComponent::Ptr c;
		};

		void cleanScriptChangedPropertyIds()
		{
			scriptChangedProperties.clearQuick();
			
		}

		bool isPropertyOverwrittenByScript(const Identifier& id)
		{
			return scriptChangedProperties.contains(id);
		}

		void setPropertyToLookFor(const Identifier& id)
		{
			searchedProperty = id;
		}

		void handleScriptPropertyChange(const Identifier& id)
		{
			if (countJsonSetProperties)
			{
				if (searchedProperty.isNull())
				{
					scriptChangedProperties.add(id);
				}
				if (searchedProperty == id)
				{
					throw String("Here...");
				}
			}
		}

	protected:

		void setDefaultValue(int p, const var &defaultValue);
		void setScriptObjectProperty(int p, var newValue) { componentProperties->setProperty(getIdFor(p), newValue); }

		Array<Identifier> propertyIds;
		Array<Identifier> deactivatedProperties;
		Array<Identifier> priorityProperties;
		DynamicObject::Ptr componentProperties;

	private:

		Array<Identifier> scriptChangedProperties;
		bool countJsonSetProperties = true;
		Identifier searchedProperty;

		ReferenceCountedArray<ScriptComponent> childComponents;

		var customControlCallback;

		NamedValueSet defaultValues;
		bool changed;

		int parentComponentIndex;

		WeakReference<Processor> connectedProcessor;
		int connectedParameterIndex = -1;

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
			mouseSensitivity,
			dragDirection,
			numProperties
		};

		ScriptSlider(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int, int);
		~ScriptSlider();

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptSlider"); }

		Identifier 	getObjectName() const override { return getStaticObjectName(); }
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
		Image getImage() const { return image; };

	private:

		double minimum, maximum;
		Image image;
	};

	struct ScriptButton : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			filmstripImage = ScriptComponent::Properties::numProperties,
			numStrips,
			isVertical,
			scaleFactor,
			radioGroup,
			isPluginParameter,
			numProperties
		};

		ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int, int);
		~ScriptButton();

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptButton") }

		Identifier 	getObjectName() const override { return getStaticObjectName(); }
		bool isAutomatable() const override { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		const Image getImage() const { return image; };
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;

	private:

		// ========================================================================================================

		Image image;

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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptComboBox"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptLabel"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		Justification getJustification();

		void restoreFromValueTree(const ValueTree &v) override
		{
			setValue(v.getProperty("value", ""));
			sendChangeMessage();
		}

		ValueTree exportAsValueTree() const override
		{
			ValueTree v = ScriptComponent::exportAsValueTree();

			v.setProperty("value", getValue(), nullptr);
			
			return v;
		}

		/** Returns the current value. */
		virtual var getValue() const override
		{
			return getScriptObjectProperty(ScriptComponent::Properties::text);
		}

		void setValue(var newValue) override
		{
			jassert(newValue != "internal");

			if (newValue.isString())
			{
				setScriptObjectProperty(ScriptComponent::Properties::text, newValue);
				sendChangeMessage();
			}
		}

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification)
		{
			if (id == getIdFor((int)ScriptComponent::Properties::text))
			{
				setValue(newValue.toString());
			}
			else
			{
				ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
			}
		}

		// ======================================================================================================== API Methods

		/** makes a label `editable`. */
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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptTable"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
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
			SliderPackIndex,
			numProperties
		};

		ScriptSliderPack(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height);;
		~ScriptSliderPack();

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptSliderPack"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		
		void setValue(var newValue) override;

		var getValue() const override;

		void setScriptProcessor(ProcessorWithScriptingContent *sb);
		SliderPackData *getSliderPackData();
		const SliderPackData *getSliderPackData() const;

		// ======================================================================================================== API Methods

		/** Connects this SliderPack to an existing SliderPackData object. */
		void referToData(var sliderPackData);

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

		String otherPackId;
		int otherPackIndex = 0;

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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptImage"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		virtual String getDebugValue() const override { return getScriptObjectProperty(Properties::FileName); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;
		StringArray getItemList() const;
		const Image getImage() const;

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

		Image image;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptImage);

		// ========================================================================================================
	};

	struct ScriptPanel : public ScriptComponent,
						 public Timer,
						 public GlobalSettingManager::ScaleFactorListener,
						 public HiseJavascriptEngine::CyclicReferenceCheckBase,
						 public MainController::SampleManager::PreloadListener
	{
		// ========================================================================================================

		enum Properties
		{
			borderSize = ScriptComponent::numProperties,
			borderRadius,
            opaque,
			allowDragging,
			allowCallbacks,
			PopupMenuItems,
			PopupOnRightClick,
			popupMenuAlign,
			selectedPopupIndex,
			stepSize,
			enableMidiLearn,
			holdIsRightClick,
			numProperties
		};

		ScriptPanel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);;
		
        ~ScriptPanel();
		

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptPanel"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		
		StringArray getOptionsFor(const Identifier &id) override;
		StringArray getItemList() const;

		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		bool isAutomatable() const override { return true; }

		void preloadStateChanged(bool isPreloading) override;

		void preRecompileCallback() override
		{
			stopTimer();
			repaintNotifier.removeAllChangeListeners();
			controlSender.cancelPendingUpdate();
			repainter.stopTimer();
		}

		bool updateCyclicReferenceList(ThreadData& data, const Identifier& id) override;

		void prepareCycleReferenceCheck() override;

		// ======================================================================================================== API Methods

		/** Triggers an asynchronous repaint. */
		void repaint();

		/** Calls the paint routine immediately. */
		void repaintImmediately();

		/** Sets a paint routine (a function with one parameter). */
		void setPaintRoutine(var paintFunction);

		/** Sets a mouse callback. */
		void setMouseCallback(var mouseCallbackFunction);

		/** Sets a timer callback. */
		void setTimerCallback(var timerCallback);

		/** Sets a loading callback that will be called when the preloading starts or finishes. */
		void setLoadingCallback(var loadingCallback);

		/** Disables the paint routine and just uses the given (clipped) image. */
		void setImage(String imageName, int xOffset, int yOffset);

		/** Call this to indicate that the value has changed (the onControl callback will be executed. */
		void changed();

		/** Loads a image which can be drawn with the paint function later on. */
		void loadImage(String imageName, String prettyName);

		/** If `allowedDragging` is enabled, it will define the boundaries where the panel can be dragged. */
		void setDraggingBounds(var area);

		/** Sets a FloatingTile that is used as popup. The position is a array [x , y, width, height] that is used for the popup dimension */
		void setPopupData(var jsonData, var position);

        
        /** Sets a new value, stores this action in the undo manager and calls the control callbacks. */
        void setValueWithUndo(var oldValue, var newValue, var actionName);
        
		// ========================================================================================================

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor=sendNotification) override
		{
			ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

			if (id == getIdFor((int)ScriptComponent::Properties::visible))
			{
				setScriptObjectProperty(ScriptComponent::visible, newValue);

				ChildIterator<ScriptPanel> iter(this);

				while (auto childPanel = iter.getNextChildComponent())
					childPanel->repaintImmediately();	
			}
			
			
		}

		struct Wrapper;

		Image getImage() const
		{
			return paintCanvas;
		}

		bool isUsingCustomPaintRoutine() const { return HiseJavascriptEngine::isJavascriptFunction(paintRoutine); }

		bool isUsingClippedFixedImage() const { return usesClippedFixedImage; };

		void scaleFactorChanged(float /*newScaleFactor*/) override {} // Do nothing until fixed...

		void mouseCallback(var mouseInformation);

		var getJSONPopupData() const { return jsonPopupData; }

		

		Rectangle<int> getPopupSize() const { return popupBounds; }

        SafeChangeBroadcaster* getRepaintNotifier() { return &repaintNotifier; };
        
		void timerCallback() override;

		Image getLoadedImage(const String &prettyName) const
		{
			for (size_t i = 0; i < loadedImages.size(); i++)
			{
				if (std::get<(int)NamedImageEntries::PrettyName>(loadedImages[i]) == prettyName)
					return std::get<(int)NamedImageEntries::Image>(loadedImages[i]);
			}

			return Image();
		};

		Rectangle<int> getDragBounds() const;

        struct RepaintNotifier: public SafeChangeBroadcaster
        {
            RepaintNotifier(ScriptPanel* panel_):
            panel(panel_)
            {
                
            }
            
            ScriptPanel* panel;
        };
        
		

	private:

		double getScaleFactorForCanvas() const;

		Rectangle<int> getBoundsForImage() const;

		bool usesClippedFixedImage = false;

		var jsonPopupData;

		Rectangle<int> popupBounds;

		friend class WeakReference<ScriptPanel>;

		WeakReference<ScriptPanel>::Master masterReference;

		void internalRepaint();

		struct AsyncControlCallbackSender : public AsyncUpdater
		{
			AsyncControlCallbackSender(ScriptPanel* parent_, ProcessorWithScriptingContent* p_) : parent(parent_), p(p_) {};

			void handleAsyncUpdate();

			WeakReference<ScriptPanel> parent;
			ProcessorWithScriptingContent* p;
		};

		struct AsyncRepainter : public Timer
		{
			AsyncRepainter(ScriptPanel* parent_) : parent(parent_){}

            ~AsyncRepainter()
            {
                
            }
            
            void triggerAsyncUpdate()
            {
                if(!isTimerRunning()) startTimer(30);
            }
            
            void timerCallback() override
            {
                handleAsyncUpdate();
                stopTimer();
            }
            
			void handleAsyncUpdate()
			{
				if (parent.get() != nullptr)
				{
					parent->internalRepaint();
				}
			}

			WeakReference<ScriptPanel> parent;
		};

        
        
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptPanel);

		ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> graphics;

		var paintRoutine;
		var mouseRoutine;
		var timerRoutine;
		var loadRoutine;

		var dragBounds;

		Image paintCanvas;

		enum class NamedImageEntries
		{
			Image=0,
			PrettyName,
			FileName
		};

		using NamedImage =	std::tuple < const Image, String, String > ;
		
		std::vector<NamedImage> loadedImages;


		AsyncRepainter repainter;
        
        RepaintNotifier repaintNotifier;
        
		AsyncControlCallbackSender controlSender;

		// ========================================================================================================
	};

	struct ScriptedViewport : public ScriptComponent
	{
	public:

		enum Properties
		{
			scrollbarThickness = ScriptComponent::numProperties,
			autoHide,
			numProperties
		};

		ScriptedViewport(ProcessorWithScriptingContent* base, Content* parentContent, Identifier viewportName, int x, int y, int width, int height);

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptedViewport"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		
	};


	struct ScriptedPlotter : public ScriptComponent
	{
	public:

		// ========================================================================================================

		ScriptedPlotter(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height);;

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptedPlotter"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ModulatorMeter"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
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

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptAudioWaveform"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); };
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

	struct ScriptFloatingTile : public ScriptComponent
	{
		enum Properties
		{
			updateAfterInit = ScriptComponent::Properties::numProperties,
			numProperties
		};

		// ========================================================================================================

		ScriptFloatingTile(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);
		~ScriptFloatingTile() {};

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptFloatingTile"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); };
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		
		
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		
		void setValue(var newValue) override
		{
			value = newValue;
		}

		var getValue() const override
		{
			return value;
		}

		var getContentData() { return jsonData; }

		// ========================================================================================================

		/** Sets the JSON object for the given floating tile. */
		void setContentData(var data)
		{
			jsonData = data;
			sendChangeMessage();
		}

		// ========================================================================================================

	private:

		struct Wrapper;

		var jsonData;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptFloatingTile);

		// ========================================================================================================
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

	/** Adds a viewport. */
	ScriptedViewport* addScriptedViewport(Identifier viewportName, int x, int y);

	/** Adds a floating layout component. */
	ScriptFloatingTile* addFloatingTile(Identifier floatingTileName, int x, int y);

	/** Returns the reference to the given component. */
	var getComponent(var name);

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

    /** Sets this script as main interface with the given size. */
    void makeFrontInterface(int width, int height);
    
	/** Sets this script as main interface with the given device resolution (only works with mobile devices). */
	void makeFullScreenInterface();

	/** sets the Tooltip that will be shown if the mouse hovers over the script's tab button. */
	void setContentTooltip(const String &tooltipToShow) { tooltip = tooltipToShow; }

	/** Sets the main toolbar properties from a JSON object. */
	void setToolbarProperties(const var &toolbarProperties);

	/** Sets the name that will be displayed in big fat Impact. */
	void setName(const String &newName)	{ name = newName; };

	/** Saves all controls that should be saved into a XML data file. */
	void storeAllControlsAsPreset(const String &fileName, const ValueTree& automationData);

	/** Restores all controls from a previously saved XML data file. */
	void restoreAllControlsFromPreset(const String &fileName);

	/** Set this to true to render all script panels with double resolution for retina or rescaling. */
	void setUseHighResolutionForPanels(bool shouldUseDoubleResolution);

	// ================================================================================================================

	// Restores the content and sets the attributes so that the macros and the control callbacks gets executed.
	void restoreAllControlsFromPreset(const ValueTree &preset);

	Colour getColour() const { return colour; };
	void endInitialization();

	void beginInitialization();

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	void addRebuildListener(RebuildListener* listener)
	{
		rebuildListeners.addIfNotAlreadyThere(listener);
	}

	void removeRebuildListener(RebuildListener* listenerToRemove)
	{
		rebuildListeners.removeAllInstancesOf(listenerToRemove);
	}

	void cleanJavascriptObjects();

	bool isEmpty();
	int getNumComponents() const noexcept{ return components.size(); };
	ScriptComponent *getComponent(int index);
	const ScriptComponent *getComponent(int index) const { return components[index]; };
	ScriptComponent * getComponentWithName(const Identifier &componentName);
	const ScriptComponent * getComponentWithName(const Identifier &componentName) const;
	int getComponentIndex(const Identifier &componentName) const;

	int getContentHeight() const { return height; }
	int getContentWidth() const { return width; }

	bool usesDoubleResolution() const
	{
		return useDoubleResolution;
	}

	ValueTree getContentProperties()
	{
		return contentPropertyData;
	}

	const ValueTree getContentProperties() const
	{
		return contentPropertyData;
	}

	ValueTree getValueTreeForComponent(const Identifier& id);


	void resetContentProperties()
	{
		rebuildComponentListFromValueTree(true);
	};

	bool contentNeedsRebuilding = false;


	void addComponentsFromValueTree(ValueTree& v);

	Result createComponentsFromValueTree(const ValueTree& newProperties);

	struct Wrapper;

	struct Helpers
	{
		static void copyComponentSnapShotToValueTree(Content* c);

		static void gotoLocation(ScriptComponent* sc);

		static Identifier getUniqueIdentifier(Content* c, const String& id);

		static void deleteComponent(Content* c, const Identifier& id, bool rebuildContent=true);

		static void deleteSelection(Content* c, ScriptComponentEditBroadcaster* b);

		static void renameComponent(Content* c, const Identifier& id, const Identifier& newId);

		static void duplicateSelection(Content* c, ReferenceCountedArray<ScriptComponent> selection, int deltaX, int deltaY);

		static void moveComponentsAfter(ScriptComponent* target, var list);

		static void pasteProperties(ReferenceCountedArray<ScriptComponent> selection, var clipboardData);

		static void setComponentValueTreeFromJSON(Content* c, const Identifier& id, const var& data);

		static Result setParentComponent(ScriptComponent* parent, var newChildren, bool rebuildNow=true);

		static ScriptComponent* createComponentFromId(Content* c, const Identifier& typeId, const Identifier& name, int x, int y, int width, int h);

		template <class T> static T* createComponentIfTypeMatches(ScriptingApi::Content* c, const Identifier& typeId, const Identifier& name, int x, int y, int w, int h)
		{
			if (typeId == T::getStaticObjectName())
				return new T(c->getScriptProcessor(), c, name, x, y, w, h);

			return nullptr;
		}

		static String createScriptVariableDeclaration(ReferenceCountedArray<ScriptComponent> selection);

		static String createCustomCallbackDefinition(ReferenceCountedArray<ScriptComponent> selection);

		static void recompileAndSearchForPropertyChange(ScriptComponent * sc, const Identifier& id);
	};

	template <class SubType> SubType* createNewComponent(const Identifier& id, int x, int y, int w, int h)
	{
		SubType* newComponent = new SubType(getScriptProcessor(), this, id, x, y, w, h);

		static const Identifier xId("x");
		static const Identifier yId("y");
		static const Identifier wId("width");
		static const Identifier hId("height");

		components.add(newComponent);

		ValueTree newData("Component");
		newData.setProperty("type", newComponent->getObjectName().toString(), nullptr);
		newData.setProperty("id", id.toString(), nullptr);
		newData.setProperty(xId, x, nullptr);
		newData.setProperty(yId, y, nullptr);
		newData.setProperty(wId, w, nullptr);
		newData.setProperty(hId, h, nullptr);

		contentPropertyData.addChild(newData, -1, nullptr);
		
		return newComponent;
	}

private:

	void sendRebuildMessage()
	{
		for (int i = 0; i < rebuildListeners.size(); i++)
		{
			if(rebuildListeners[i] != nullptr)
			{
				rebuildListeners[i]->contentWasRebuilt();
			}
			else
			{
				rebuildListeners.remove(i--);
			}
		}
	}

	Array<WeakReference<RebuildListener>> rebuildListeners;

	var templateFunctions;

	template<class Subtype> Subtype *addComponent(Identifier name, int x, int y, int width = -1, int height = -1);

	void rebuildComponentListFromValueTree(bool rebuildContent=false, Identifier* errorId=nullptr);

	friend class ScriptContentComponent;
	friend class WeakReference<ScriptingApi::Content>;
	WeakReference<ScriptingApi::Content>::Master masterReference;

	bool useDoubleResolution = false;

	ValueTree contentPropertyData;

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

using ScriptComponent = ScriptingApi::Content::ScriptComponent;
using ScriptComponentSelection = ReferenceCountedArray<ScriptComponent>;

} // namespace hise
#endif  // SCRIPTINGAPICONTENT_H_INCLUDED
