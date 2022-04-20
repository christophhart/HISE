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

class ValueTreeUpdateWatcher : ValueTree::Listener
{
public:

	/** A small helper class that prevents multiple updates for every call to a ValueTree method.
	*
	*	Use this when you change multiple properties within a function and need to update the watcher
	*	only when the function is finished. Create one of these on the stack and it will coallescate
	*	the updates until it goes out of scope.
	*/
	class ScopedDelayer
	{
	public:

		ScopedDelayer(ValueTreeUpdateWatcher* watcher_) :
			watcher(watcher_)
		{
			if(watcher != nullptr)
				watcher->delayCalls = true;
		};

		~ScopedDelayer()
		{
			if (watcher != nullptr)
			{
				watcher->delayCalls = false;

				if (watcher->shouldCallAfterDelay)
					watcher->callListener();
			}
		}

	private:

		WeakReference<ValueTreeUpdateWatcher> watcher;
			
	};

	class ScopedSuspender
	{
	public:
		
		ScopedSuspender(ValueTreeUpdateWatcher* watcher_) :
			watcher(watcher_)
		{
			if (watcher.get() != nullptr)
			{
				watcher->isSuspended = true;
			}
		};

		~ScopedSuspender()
		{
			if (watcher.get() != nullptr)
			{
				watcher->isSuspended = false;
			}
		};
	
	private:

		WeakReference<ValueTreeUpdateWatcher> watcher;
	};

	struct Listener
	{
		virtual ~Listener() {};

		virtual void valueTreeNeedsUpdate() = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	ValueTreeUpdateWatcher(ValueTree& v, Listener* l) :
		state(v),
		listener(l)
	{
		state.addListener(this);
	};

	void valueTreePropertyChanged(ValueTree& /*treeWhosePropertyHasChanged*/, const Identifier& property) override
	{
		if (isCurrentlyUpdating)
			return;

		static const Identifier id("id");
		static const Identifier parentComponent("parentComponent");

		if (property == id || property == parentComponent)
			callListener();
	}

	void valueTreeChildAdded(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenAdded*/) override
	{
		callListener();
	}

	void valueTreeChildRemoved(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override
	{
		callListener();
	}

	void valueTreeChildOrderChanged(ValueTree& /*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override
	{
		callListener();
	}

	void valueTreeParentChanged(ValueTree& /*treeWhoseParentHasChanged*/) override {};

private:

	bool delayCalls = false;

	bool shouldCallAfterDelay = false;

	void callListener()
	{
		if (isSuspended)
			return;

		if (delayCalls)
		{
			shouldCallAfterDelay = true;
			return;
		}
		
		if (isCurrentlyUpdating)
			return;

		isCurrentlyUpdating = true;

		

        if(listener.get() != nullptr)
            listener->valueTreeNeedsUpdate();
        
		isCurrentlyUpdating = false;
	}

	bool isCurrentlyUpdating = false;

	bool isSuspended = false;

	ValueTree state;
	WeakReference<Listener> listener;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(ValueTreeUpdateWatcher);
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

	SelectorTypes getTypeForId(const Identifier &id) const;

	void addToTypeSelector(SelectorTypes type, Identifier id, double min = 0.0, double max = 1.0, double interval = 0.01);

	SliderRange getRangeForId(const Identifier &id) const
	{
		return sliderRanges[id.toString()];
	}

private:

	Array<Identifier> toggleProperties;
	Array<Identifier> sliderProperties;
	Array<Identifier> colourProperties;
	Array<Identifier> choiceProperties;
	Array<Identifier> multilineProperties;
	Array<Identifier> fileProperties;
	Array<Identifier> codeProperties;
	HashMap<String, SliderRange> sliderRanges;
};

/** This is the interface area that can be filled with buttons, knobs, etc.
*	@ingroup scriptingApi
*
*/
class ScriptingApi::Content : public ScriptingObject,
	public DynamicObject,
	public RestorableObject,
	public ValueTreeUpdateWatcher::Listener
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
        
        virtual void contentRebuildStateChanged(bool /*isRebuilding*/) {};

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
		public AssignableObject,
		public SafeChangeBroadcaster,
		public UpdateDispatcher::Listener
	{
		using Ptr = ReferenceCountedObjectPtr<ScriptComponent>;

		struct PropertyWithValue
		{
			int id;
			var value = var::undefined();
		};

		// ============================================================================================================

		enum Properties
		{
			text = 0,
			visible,
			enabled,
			locked,
			x,
			y,
			width,
			height,
			min,
			max,
			defaultValue,
			tooltip,
			bgColour,
			itemColour,
			itemColour2,
			textColour,
			macroControl,
			saveInPreset,
			isPluginParameter,
			pluginParameterName,
            isMetaParameter,
			linkedTo,
			useUndoManager,
			parentComponent,
			processorId,
			parameterId,
			numProperties
		};

		struct SubComponentListener
		{
			virtual ~SubComponentListener() {};

			virtual void subComponentAdded(ScriptComponent* newComponent) = 0;
			virtual void subComponentRemoved(ScriptComponent* componentAboutToBeRemoved) = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(SubComponentListener);
		};

		struct ZLevelListener
		{
			enum class ZLevel
			{
				Back,
				Default,
				Front,	     
				AlwaysOnTop,
				numZLevels
			};

			virtual ~ZLevelListener() {};

			virtual void zLevelChanged(ZLevel newZLevel) = 0;

			virtual void wantsToLoseFocus() {}

			JUCE_DECLARE_WEAK_REFERENCEABLE(ZLevelListener);
		};

		ScriptComponent(ProcessorWithScriptingContent* base, Identifier name_, int numConstants = 0);

		virtual ~ScriptComponent();

		virtual StringArray getOptionsFor(const Identifier &id);
		virtual ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) = 0;

		void handleAsyncUpdate() override
		{
			sendSynchronousChangeMessage();
		}

		Identifier getName() const;
		Identifier getObjectName() const override;

		String getParentComponentId() const;

		bool hasParentComponent() const;

		ScriptComponent* getParentScriptComponent();

		const ScriptComponent* getParentScriptComponent() const;

		virtual void preRecompileCallback() 
		{
			controlSender.cancelMessage();
		};

		virtual ValueTree exportAsValueTree() const override;;
		virtual void restoreFromValueTree(const ValueTree &v) override;;

		String getDebugValue() const override { return getValue().toString(); };
		String getDebugName() const override { return name.toString(); };
		String getDebugDataType() const override { return getObjectName().toString(); }
		virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		bool isLocked() const
		{
			return getScriptObjectProperty(locked);
		}

		var getAssignedValue(int index) const override;
		void assign(const int index, var newValue) override;
		int getCachedIndex(const var &indexExpression) const override;

		virtual void resetValueToDefault()
		{
			setValue(0);
		}

		void setScriptObjectProperty(int p, var newValue, NotificationType notifyListeners = dontSendNotification);

		virtual void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification);
		virtual bool isAutomatable() const { return false; }

		virtual bool isClickable() const { return getScriptObjectProperty(enabled); };

		

		virtual void handleDefaultDeactivatedProperties();

		const Identifier getIdFor(int p) const;
		int getNumIds() const;

		const var getScriptObjectProperty(int p) const;

		const var getScriptObjectProperty(Identifier id) const;

		var getNonDefaultScriptObjectProperties() const;

		String getScriptObjectPropertiesAsJSON() const;
		
		

		bool isPropertyDeactivated(Identifier &id) const;
		bool hasProperty(const Identifier& id) const;

		Rectangle<int> getPosition() const;

		int getIndexForProperty(const Identifier& id) const
		{
			return propertyIds.indexOf(id);
		}

		

		ReferenceCountedObject* getCustomControlCallback();

		/** This updates the internal content data object from the script processor.
		*
		*	You should never use this, but use ScriptComponentEditBroadcaster::setPropertyForSelection() instead.
		*/
		void updateContentPropertyInternal(int propertyId, const var& newValue);

		void updateContentPropertyInternal(const Identifier& propertyId, const var& newValue);

		virtual void cancelPendingFunctions() {};

		virtual bool isShowing(bool checkParentComponentVisibility = true) const;

		template <class ChildType> class ChildIterator
		{
		public:

			ChildIterator(ScriptComponent* c)
			{
				for (int i = 0; i < c->parent->getNumComponents(); i++)
				{
					if (auto sc = dynamic_cast<ChildType*>(c->parent->getComponent(i)))
					{
						auto scTree = sc->getPropertyValueTree();
						auto cTree = c->getPropertyValueTree();

						if (scTree == cTree || scTree.isAChildOf(cTree))
						{
							childComponents.add(sc);
						}
					}
				}
			}

			ChildType* getNextChildComponent()
			{
				
				return childComponents[index++];
			}

		private:

			int index = 0;

			Array<ChildType*> childComponents;
		};

		// API Methods =====================================================================================================

		/** returns the value of the property. */
		var get(String propertyName) const;

		/** Sets the property. */
		void set(String propertyName, var value);;

		/** Returns the current value. */
		virtual var getValue() const
        {
			var rv;
			{
				SimpleReadWriteLock::ScopedReadLock sl(valueLock);
				rv = value;
			}
			
            jassert(!value.isString());
            return rv;
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
        
		/** Returns list of component's children */
		var getChildComponents();
				
		/** Returns a [x, y, w, h] array that was reduced by the given amount. */
		var getLocalBounds(float reduceAmount);

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

		/** Call this to indicate that the value has changed (the onControl callback will be executed. */
		virtual void changed();

		/** Returns a list of all property IDs as array. */
		var getAllProperties();

		/** Changes the depth hierarchy (z-axis) of sibling components (Back, Default, Front or AlwaysOnTop). */
		void setZLevel(String zLevel);

		/** Adds a callback to react on key presses (when this component is focused). */
		void setKeyPressCallback(var keyboardFunction);

		/** Call this method in order to give away the focus for this component. */
		void loseFocus();

		/** Attaches the local look and feel to this component. */
		void setLocalLookAndFeel(var lafObject);

		// End of API Methods ============================================================================================

		bool handleKeyPress(const KeyPress& k);

		void handleFocusChange(bool isFocused);

		bool wantsKeyboardFocus() const { return (bool)keyboardCallback; }

		void addSubComponentListener(SubComponentListener* l)
		{
			subComponentListeners.addIfNotAlreadyThere(l);
		}

		void removeSubComponentListener(SubComponentListener* l)
		{
			subComponentListeners.removeAllInstancesOf(l);
		}

		void sendSubComponentChangeMessage(ScriptComponent* s, bool wasAdded, NotificationType notify=sendNotificationAsync);
		
		void setChanged(bool isChanged = true) noexcept{ hasChanged = isChanged; }
		bool isChanged() const noexcept{ return hasChanged; };

		var value;
		Identifier name;
		Content *parent;
		bool skipRestoring;

		struct Wrapper;

		bool isConnectedToProcessor() const;;

		bool isConnectedToGlobalCable() const;

		void sendGlobalCableValue(var v);

		Processor* getConnectedProcessor() const { return connectedProcessor.get(); };

		int getConnectedParameterIndex() { return connectedParameterIndex; };

        bool isConnectedToMacroControll() const noexcept
        {
            return !macroRecursionProtection && connectedMacroIndex >= 0;
        }
        
        void setMacroRecursionProtection(bool shouldBeEnabled)
        {
            macroRecursionProtection = shouldBeEnabled;
        }
        
        int getMacroControlIndex() const { return connectedMacroIndex; }
        
		ValueTree getPropertyValueTree() { return propertyTree; }

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

		void repaintThisAndAllChildren();



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
					scriptChangedProperties.addIfNotAlreadyThere(id);
				}
				if (searchedProperty == id)
				{
					throw String("Here...");
				}
			}
		}

		/** Returns a local look and feel if it was registered before. */
		LookAndFeel* createLocalLookAndFeel();

		static Array<Identifier> numberPropertyIds;
		static bool numbersInitialised;

		ScriptComponent* getLinkedComponent() { return linkedComponent.get(); }
		const ScriptComponent* getLinkedComponent() const { return linkedComponent.get(); }

		void cancelChangedControlCallback()
		{
			controlSender.cancelMessage();
		}

		void addZLevelListener(ZLevelListener* l)
		{
			zLevelListeners.addIfNotAlreadyThere(l);
		}

		void removeZLevelListener(ZLevelListener* l)
		{
			zLevelListeners.removeAllInstancesOf(l);
		}

	protected:

		bool isCorrectlyInitialised(int p) const
		{
			return initialisedProperties[p];
		}

		bool isCorrectlyInitialised(const Identifier& id) const
		{
			int p = propertyIds.indexOf(id);

			return initialisedProperties[p];
		}

		void initInternalPropertyFromValueTreeOrDefault(int id, bool justSetInitFlag=false);

		void setDefaultValue(int p, const var &defaultValue);
		
		

		void addLinkedTarget(ScriptComponent* newTarget)
		{
			linkedComponentTargets.addIfNotAlreadyThere(newTarget);
		}

		void removeLinkedTarget(ScriptComponent* targetToRemove)
		{
			linkedComponentTargets.removeAllInstancesOf(targetToRemove);
		}

		void updatePropertiesAfterLink(NotificationType notifyEditor);

		virtual Array<PropertyWithValue> getLinkProperties() const;

		Array<Identifier> propertyIds;
		Array<Identifier> deactivatedProperties;
		Array<Identifier> priorityProperties;
		
		bool removePropertyIfDefault = true;

#if USE_BACKEND
		juce::SharedResourcePointer<hise::ScriptComponentPropertyTypeSelector> selectorTypes;
#endif

	private:

		var localLookAndFeel;

		WeakCallbackHolder keyboardCallback;

		struct AsyncControlCallbackSender : private UpdateDispatcher::Listener
        {
		public:

            AsyncControlCallbackSender(ScriptComponent* parent_, ProcessorWithScriptingContent* p_);;
            
			void sendControlCallbackMessage();

			void cancelMessage();

		private:

			bool changePending = false;

            void handleAsyncUpdate();
            
            ScriptComponent* parent;
            ProcessorWithScriptingContent* p;
        };
        
		struct GlobalCableConnection;

		AsyncControlCallbackSender controlSender;

		bool isPositionProperty(Identifier id) const;

		ValueTree propertyTree;

		Array<Identifier> scriptChangedProperties;

		Array<WeakReference<SubComponentListener>> subComponentListeners;
		Array<WeakReference<ZLevelListener>> zLevelListeners;

		ZLevelListener::ZLevel currentZLevel = ZLevelListener::ZLevel::Default;

		mutable hise::SimpleReadWriteLock valueLock;

		bool countJsonSetProperties = true;
		Identifier searchedProperty;

		BigInteger initialisedProperties;

        WeakReference<ScriptComponent> linkedComponent;

		Array<WeakReference<ScriptComponent>> linkedComponentTargets;

		var customControlCallback;

		NamedValueSet defaultValues;
		bool hasChanged;

		WeakReference<Processor> connectedProcessor;
		int connectedParameterIndex = -1;

		ScopedPointer<GlobalCableConnection> globalConnection;

        int connectedMacroIndex = -1;
        bool macroRecursionProtection = false;

        JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptComponent);
        
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
			suffix,
			filmstripImage,
			numStrips,
			isVertical,
			scaleFactor,
			mouseSensitivity,
			dragDirection,
			showValuePopup,
			showTextBox,
			scrollWheel,
			enableMidiLearn,
			numProperties,
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

		void resetValueToDefault() override
		{
			auto f = (float)getScriptObjectProperty(ScriptComponent::defaultValue);
			FloatSanitizers::sanitizeFloatNumber(f);
			setValue(f);
		}

		void handleDefaultDeactivatedProperties() override;

		Array<PropertyWithValue> getLinkProperties() const override;

		// ======================================================================================================== API Methods

		/** Set the value from a 0.0 to 1.0 range */
		void setValueNormalized(double normalizedValue) override;

		/** Returns the normalized value. */
		double getValueNormalized() const override;

		/** Sets the range and the step size of the knob. */
		void setRange(double min, double max, double stepSize);

		/** Sets the knob to the specified mode. */
		void setMode(String mode);

		/** Pass a function that takes a double and returns a String in order to override the popup display text. */
		void setValuePopupFunction(var newFunction);

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

		HiSlider::Mode m = HiSlider::Mode::Linear;
		Slider::SliderStyle styleId;
		Image getImage() const { return image ? *image.getData() : Image(); };
		var sliderValueFunction;

	private:

		double minimum, maximum;
		PooledImage image;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptSlider)
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
			isMomentary,
			enableMidiLearn,
			numProperties
		};

		ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int, int);
		~ScriptButton();

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptButton") }

		Identifier 	getObjectName() const override { return getStaticObjectName(); }
		bool isAutomatable() const override { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		const Image getImage() const { return image ? *image.getData() : Image(); };
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		StringArray getOptionsFor(const Identifier &id) override;

		void handleDefaultDeactivatedProperties() override;

		// ======================================================================================================== API Methods

		/** Sets a FloatingTile that is used as popup. */
		void setPopupData(var jsonData, var position);

		// ========================================================================================================

		void resetValueToDefault() override
		{
			setValue((int)getScriptObjectProperty(defaultValue));
		}

		Rectangle<int> getPopupPosition() const
		{
			return popupPosition;
		}

		const var& getPopupData() const
		{
			return popupData;
		}

	private:

		struct Wrapper;

		// ========================================================================================================

		Rectangle<int> popupPosition;

		var popupData;

		PooledImage image;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptButton)
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptButton)
	};

	struct ScriptComboBox : public ScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			Items = ScriptComponent::numProperties,
			FontName,
			FontSize,
			FontStyle,
			enableMidiLearn,
			numProperties
		};

		ScriptComboBox(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int);
		~ScriptComboBox() {};

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptComboBox"); }

		StringArray getOptionsFor(const Identifier &id) override;

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		bool isAutomatable() const override { return true; }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification);
		StringArray getItemList() const;

		void resetValueToDefault() override
		{
			setValue((int)getScriptObjectProperty(defaultValue));
		}

		void handleDefaultDeactivatedProperties();

		Array<PropertyWithValue> getLinkProperties() const override;

		// ======================================================================================================== API Methods

		/** Returns the currently selected item text. */
		String getItemText() const;

		/** Adds an item to a combo box. */
		void addItem(const String &newName);

		// ========================================================================================================

		struct Wrapper;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComboBox);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptComboBox)
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
				triggerAsyncUpdate();
			}
		}

		void resetValueToDefault() override
		{
			setValue("");
		}

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification)
		{
			if (id == getIdFor((int)ScriptComponent::Properties::text))
			{
				jassert(isCorrectlyInitialised(ScriptComponent::Properties::text));
				setValue(newValue.toString());
			}
			
			ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

		}

		void handleDefaultDeactivatedProperties() override;

		bool isClickable() const override
		{
			return getScriptObjectProperty(Editable) && ScriptComponent::isClickable();
		}

		// ======================================================================================================== API Methods

		/** makes a label `editable`. */
		void setEditable(bool shouldBeEditable);

		// ========================================================================================================

		struct Wrapper;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptLabel);
	};


	struct ComplexDataScriptComponent : public ScriptComponent,
										public ExternalDataHolder,
										public ComplexDataUIUpdaterBase::EventListener
	{
		ComplexDataScriptComponent(ProcessorWithScriptingContent* base, Identifier name, snex::ExternalData::DataType type_);;
			
		Table* getTable(int) override
		{
			return static_cast<Table*>(getUsedData(snex::ExternalData::DataType::Table));
		}

		SliderPackData* getSliderPack(int) override
		{
			return static_cast<SliderPackData*>(getUsedData(snex::ExternalData::DataType::SliderPack));
		}

		MultiChannelAudioBuffer* getAudioFile(int) override
		{
			return static_cast<MultiChannelAudioBuffer*>(getUsedData(snex::ExternalData::DataType::AudioFile));
		}

		FilterDataObject* getFilterData(int index) override
		{
			jassertfalse;
			return nullptr; // soon;
		}

		SimpleRingBuffer* getDisplayBuffer(int index) override
		{
			jassertfalse;
			return nullptr; // soon
		}

		int getNumDataObjects(ExternalData::DataType t) const override
		{
			return t == type ? 1 : 0;
		}

		bool removeDataObject(ExternalData::DataType t, int index) override { return true; }

		// override this and return the property id used for the index
		virtual int getIndexPropertyId() const = 0;

		StringArray getOptionsFor(const Identifier &id) override;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override
		{
			ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

			if (getIdFor(processorId) == id || getIdFor(getIndexPropertyId()) == id)
			{
				updateCachedObjectReference();
			}

			if (getIdFor(parameterId) == id)
			{
				// don't do anything if you try to connect a table to a parameter...
				return;
			}
		}

		ValueTree exportAsValueTree() const override;

		void restoreFromValueTree(const ValueTree &v) override;

		void handleDefaultDeactivatedProperties();

		void referToDataBase(var newData)
		{
			if (auto td = dynamic_cast<ScriptingObjects::ScriptComplexDataReferenceBase*>(newData.getObject()))
			{
				if (td->getDataType() != type)
					reportScriptError("Data Type mismatch");

				otherHolder = td->getHolder();

				setScriptObjectPropertyWithChangeMessage(getIdFor(getIndexPropertyId()), td->getIndex(), sendNotification);
				updateCachedObjectReference();
			}
			else if (auto cd = dynamic_cast<ComplexDataScriptComponent*>(newData.getObject()))
			{
				if (cd->type != type)
					reportScriptError("Data Type mismatch");

				otherHolder = cd;
				updateCachedObjectReference();
			}
			else if ((newData.isInt() || newData.isInt64()) && (int)newData == -1)
			{
				// just go back to its own data object
				otherHolder = nullptr;
				updateCachedObjectReference();
			}
		}

		ComplexDataUIBase* getCachedDataObject() const { return cachedObjectReference.get(); };

		var registerComplexDataObjectAtParent(int index = -1);

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			
		}

		ComplexDataUIBase::SourceWatcher& getSourceWatcher() { return sourceWatcher; }

	protected:

		void updateCachedObjectReference()
		{
			if (cachedObjectReference != nullptr)
				cachedObjectReference->getUpdater().removeEventListener(this);

			cachedObjectReference = getComplexBaseType(type, 0);

			if (cachedObjectReference != nullptr)
				cachedObjectReference->getUpdater().addEventListener(this);

			sourceWatcher.setNewSource(cachedObjectReference);
		}

	private:

		const snex::ExternalData::DataType type;

		ComplexDataUIBase* getUsedData(snex::ExternalData::DataType requiredType);

		ExternalDataHolder* getExternalHolder();

		// Overrides itself and the connected processor
		WeakReference<ExternalDataHolder> otherHolder;

		WeakReference<ComplexDataUIBase> cachedObjectReference;
		ReferenceCountedObjectPtr<ComplexDataUIBase> ownedObject;

		ComplexDataUIBase::SourceWatcher sourceWatcher;
	};

	struct ScriptTable : public ComplexDataScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			TableIndex = ScriptComponent::Properties::numProperties,
			customColours,
			numProperties
		};

		ScriptTable(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int height);
		~ScriptTable();

		// ========================================================================================================

		int getIndexPropertyId() const override { return TableIndex; };

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptTable"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		void handleDefaultDeactivatedProperties() override;

		void resetValueToDefault() override;

		// ======================================================================================================== API Method

		/** Pass a function that takes a double and returns a String in order to override the popup display text. */
		void setTablePopupFunction(var newFunction);

		void connectToOtherTable(String processorId, int index)
		{
			setScriptObjectProperty(ScriptingApi::Content::ScriptComponent::processorId, processorId, dontSendNotification);
			setScriptObjectProperty(getIndexPropertyId(), index, sendNotification);
		}

		/** Returns the table value from 0.0 to 1.0 according to the input value from 0.0 to 1.0. */
		float getTableValue(float inputValue);

		/** Connects the table to an existing Processor. */
		/** Makes the table snap to the given x positions (from 0.0 to 1.0). */
		void setSnapValues(var snapValueArray);

		/** Connects it to a table data object (or UI element in the same interface). -1 sets it back to its internal data object. */
		void referToData(var tableData);

		/** Registers this table (and returns a reference to the data) with the given index so you can use it from the outside. */
		var registerAtParent(int index);

		// ========================================================================================================

		struct Wrapper;

		var snapValues;
		var tableValueFunction;

	private:

		Table* getCachedTable()  { return static_cast<Table*>(getCachedDataObject()); };
		const Table* getCachedTable() const { return static_cast<const Table*>(getCachedDataObject()); };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptTable);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptTable);
	};

	struct ScriptSliderPack : public ComplexDataScriptComponent
	{
		// ========================================================================================================

		enum Properties
		{
			SliderAmount = ScriptComponent::Properties::numProperties,
			StepSize,
			FlashActive,
			ShowValueOverlay,
			SliderPackIndex,
			numProperties
		};

		ScriptSliderPack(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height);;
		~ScriptSliderPack();

		// ========================================================================================================

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptSliderPack"); }

		int getIndexPropertyId() const override { return SliderPackIndex; };

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;
		void setValue(var newValue) override;

		void resetValueToDefault() override
		{
			auto f = (float)getScriptObjectProperty(defaultValue);
			FloatSanitizers::sanitizeFloatNumber(f);
			setAllValues((double)f);
		}

		var getValue() const override;

		// ======================================================================================================== API Methods

		/** Connects this SliderPack to an existing SliderPackData object. -1 sets it back to its internal data object. */
		void referToData(var sliderPackData);

		/** sets the slider value at the given index.*/
		void setSliderAtIndex(int index, double value);

		/** Returns the value at the given index. */
		double getSliderValueAt(int index);

		/** Sets all slider values to the given value. */
		void setAllValues(double value);

		/** Returns the number of sliders. */
		int getNumSliders() const;

		/** Sets a non-uniform width per slider using an array in the form [0.0, ... a[i], ... 1.0]. */
		void setWidthArray(var normalizedWidths);
        
		/** Registers this sliderpack to the script processor to be acessible from the outside. */
		var registerAtParent(int pIndex);

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

		// ========================================================================================================

		void changed() override;

		struct Wrapper;

		SliderPackData* getSliderPackData() { return getCachedSliderPack(); };

		Array<var> widthArray;

	private:

		const SliderPackData* getCachedSliderPack() const { return static_cast<const SliderPackData*>(getCachedDataObject()); };
		SliderPackData* getCachedSliderPack() { return static_cast<SliderPackData*>(getCachedDataObject()); };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptSliderPack);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptSliderPack);

		// ========================================================================================================
	};

	struct ScriptAudioWaveform : public ComplexDataScriptComponent
	{
		enum Properties
		{
			itemColour3 = ScriptComponent::Properties::numProperties,
			opaque,
			showLines,
			showFileName,
			sampleIndex,
			enableRange,
			numProperties
		};

		// ========================================================================================================

		ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height);
		~ScriptAudioWaveform() {};

		// ========================================================================================================

		int getIndexPropertyId() const override { return sampleIndex; };

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptAudioWaveform"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); };
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		// ======================================================================================================== API Methods

		/** Connects this AudioFile to an existing ScriptAudioFile object. -1 sets it back to its internal data object. */
		void referToData(var audioData);

		/** Returns the current range start. */
		int getRangeStart();

		/** Returns the current range end. */
		int getRangeEnd();

		/** Registers this waveform to the script processor to be acessible from the outside. */
		var registerAtParent(int pIndex);

		// ========================================================================================================

		void handleDefaultDeactivatedProperties() override;

		void resetValueToDefault() override;

		StringArray getOptionsFor(const Identifier &id) override;
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;

		ModulatorSampler* getSampler();

		// ========================================================================================================

	private:

		struct Wrapper;

		MultiChannelAudioBuffer* getCachedAudioFile() { return static_cast<MultiChannelAudioBuffer*>(getCachedDataObject()); };
		const MultiChannelAudioBuffer* getCachedAudioFile() const { return static_cast<const MultiChannelAudioBuffer*>(getCachedDataObject()); };
		

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptAudioWaveform);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptAudioWaveform);

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
			BlendMode,
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
		void handleDefaultDeactivatedProperties() override;

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

		void updateBlendMode();

		Image blendImage;

		PooledImage image;

		gin::BlendMode blendMode = gin::BlendMode::Normal;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptImage);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptImage);

		// ========================================================================================================
	};

	struct ScriptPanel : public ScriptComponent,
						 public SuspendableTimer,
						 public GlobalSettingManager::ScaleFactorListener,
						 public HiseJavascriptEngine::CyclicReferenceCheckBase,
						 public MainController::SampleManager::PreloadListener
	{
		// ========================================================================================================

		struct MouseCursorInfo
		{
			Path path;
			Colour c = juce::Colours::white;
			Point<float> hitPoint = { 0.0f, 0.0f };
		} mouseCursorPath;

		struct AnimationListener
		{
			virtual ~AnimationListener() {};

			virtual void animationChanged() = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(AnimationListener);
		};

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
			isPopupPanel,
			numProperties
		};

		enum class DebugWatchIndex
		{
			Data,
			ChildPanels,
			PaintRoutine,
			TimerCallback,
			MouseCallback,
			PreloadCallback,
			FileCallback,
			NumDebugWatchIndexes
		};

		ScriptPanel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);;
		
		ScriptPanel(ScriptPanel* parent);

        ~ScriptPanel();
		
		void init();

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
			cachedList.clear();

			ScriptComponent::preRecompileCallback();

			timerRoutine.clear();
			loadRoutine.clear();
			mouseRoutine.clear();

			stopTimer();
		}

		bool updateCyclicReferenceList(ThreadData& data, const Identifier& id) override;

		void prepareCycleReferenceCheck() override;

		void handleDefaultDeactivatedProperties() override;

		int getNumChildElements() const override;

		DebugInformationBase* getChildElement(int index) override;

		DebugInformationBase::Ptr createChildElement(DebugWatchIndex index) const;

		

		// ======================================================================================================== API Methods

		/** Triggers an asynchronous repaint. */
		void repaint();

		/** Calls the paint routine immediately. */
		void repaintImmediately();

		/** Sets a Path as mouse cursor for this panel. */
		void setMouseCursor(var pathIcon, var colour, var hitPoint);

		/** Sets an JSON animation. */
		void setAnimation(String base64LottieAnimation);

		/** Sets a frame to be displayed. */
		void setAnimationFrame(int numFrame);

		/** Returns a JSON object containing the data of the animation object. */
		var getAnimationData();

		/** Sets a paint routine (a function with one parameter). */
		void setPaintRoutine(var paintFunction);

		/** Sets a mouse callback. */
		void setMouseCallback(var mouseCallbackFunction);

		/** Sets a file drop callback. */
		void setFileDropCallback(String callbackLevel, String wildcard, var dropFunction);

		/** Sets a timer callback. */
		void setTimerCallback(var timerCallback);

		/** Sets a loading callback that will be called when the preloading starts or finishes. */
		void setLoadingCallback(var loadingCallback);

		/** Disables the paint routine and just uses the given (clipped) image. */
		void setImage(String imageName, int xOffset, int yOffset);

		/** Loads a image which can be drawn with the paint function later on. */
		void loadImage(String imageName, String prettyName);

		/** Unload all images from the panel. */
		void unloadAllImages();

		/** If `allowedDragging` is enabled, it will define the boundaries where the panel can be dragged. */
		void setDraggingBounds(var area);

		/** Sets a FloatingTile that is used as popup. The position is a array [x , y, width, height] that is used for the popup dimension */
		void setPopupData(var jsonData, var position);
        
        /** Sets a new value, stores this action in the undo manager and calls the control callbacks. */
        void setValueWithUndo(var oldValue, var newValue, var actionName);
        
		/** Opens the panel as popup. */
		void showAsPopup(bool closeOtherPopups);

		/** Closes the popup manually. */
		void closeAsPopup();

		/** Returns true if the popup is currently showing. */
		bool isVisibleAsPopup();

		/** If this is set to true, the popup will be modal with a dark background that can be clicked to close. */
		void setIsModalPopup(bool shouldBeModal)
		{
			isModalPopup = shouldBeModal;
		}

		/** Adds a child panel to this panel. */
		var addChildPanel();

		/** Removes the panel from its parent panel if it was created with addChildPanel(). */
		bool removeFromParent();

		/** Returns a list of all panels that have been added as child panel. */
		var getChildPanelList();

		/** Returns the panel that this panel has been added to with addChildPanel. */
		var getParentPanel();

		int getNumSubPanels() const { return childPanels.size(); }

		ScriptPanel* getSubPanel(int index)
		{
			return childPanels[index].get();
		}

		// ========================================================================================================

#if HISE_INCLUDE_RLOTTIE
		bool isAnimationActive() const { return animation != nullptr && animation->isValid(); }
		RLottieAnimation::Ptr getAnimation() { return animation.get(); }
#endif

		void showAsModalPopup();

		void forcedRepaint()
		{
		};

		MouseCursorInfo getMouseCursorPath() const
		{
			return mouseCursorPath;
		}

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor=sendNotification) override
		{
			

			ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

#if HISE_INCLUDE_RLOTTIE
			if (id == getIdFor((int)ScriptComponent::height) ||
				id == getIdFor((int)ScriptComponent::width))
			{
				if (animation != nullptr)
				{
					auto pos = getPosition();
					animation->setSize(pos.getWidth(), pos.getHeight());
				}
			}
#endif
		}

		struct Wrapper;

		bool isModal() const
		{
			return isModalPopup;
		}

		bool isUsingCustomPaintRoutine() const { return HiseJavascriptEngine::isJavascriptFunction(paintRoutine); }

		bool isUsingClippedFixedImage() const { return usesClippedFixedImage; };

		void scaleFactorChanged(float /*newScaleFactor*/) override {} // Do nothing until fixed...

		void fileDropCallback(var fileInformation);

		void mouseCallback(var mouseInformation);

		void mouseCallbackInternal(const var& mouseInformation, Result& r);

		var getJSONPopupData() const { return jsonPopupData; }

		void cancelPendingFunctions() override
		{
			stopTimer();
		}

		void resetValueToDefault() override
		{
			auto f = (float)getScriptObjectProperty(defaultValue);
			FloatSanitizers::sanitizeFloatNumber(f);
			setValue(f);
			repaint();
		}

		Rectangle<int> getPopupSize() const { return popupBounds; }

		void timerCallback() override;

		bool timerCallbackInternal(MainController * mc, Result &r);

		Image getLoadedImage(const String &prettyName) const
		{
			for (const auto& img : loadedImages)
			{
				if (img.prettyName == prettyName)
					return img.image ? *img.image.getData() : Image();
			}

			return Image();
		};

		Rectangle<int> getDragBounds() const;

		bool isShowing(bool checkParentComponentVisibility=true) const override
		{
			if (!ScriptComponent::isShowing(checkParentComponentVisibility))
				return false;

			if (!getScriptObjectProperty(isPopupPanel))
				return true;

			return shownAsPopup;
		}

		void repaintWrapped();

		DrawActions::Handler* getDrawActionHandler()
		{
			if (graphics != nullptr)
				return &graphics->getDrawHandler();

			return nullptr;
		}

		void addAnimationListener(AnimationListener* l);

		void removeAnimationListener(AnimationListener* l);

		String fileDropExtension;
		String fileDropLevel;

	private:

#if HISE_INCLUDE_RLOTTIE
		void updateAnimationData();
		ScopedPointer<RLottieAnimation> animation;
		var animationData;
#endif

		Array<WeakReference<AnimationListener>> animationListeners;

		bool shownAsPopup = false;
		bool isModalPopup = false;

		double getScaleFactorForCanvas() const;

		Rectangle<int> getBoundsForImage() const;

		bool usesClippedFixedImage = false;

		var jsonPopupData;

		Rectangle<int> popupBounds;

		void internalRepaint(bool forceRepaint=false);

		bool internalRepaintIdle(bool forceRepaint, Result& r);

		ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> graphics;

		var paintRoutine;

		void buildDebugListIfEmpty() const;

		WeakCallbackHolder timerRoutine;
		WeakCallbackHolder loadRoutine;
		WeakCallbackHolder mouseRoutine;
		WeakCallbackHolder fileDropRoutine;

		var dragBounds;

		struct NamedImage
		{
			PooledImage image;
			String prettyName;
		};
		
		WeakReference<ScriptPanel> parentPanel;
		ReferenceCountedArray<ScriptPanel> childPanels;

		mutable DebugInformationBase::List cachedList;

		bool isChildPanel = false;

		Array<NamedImage> loadedImages;
		
		// ========================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptPanel);
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptPanel);
	};

	struct ScriptedViewport : public ScriptComponent
	{
	public:

		enum Properties
		{
			scrollbarThickness = ScriptComponent::numProperties,
			autoHide,
			useList,
			viewPositionX,
			viewPositionY,
			Items,
			FontName,
			FontSize,
			FontStyle,
			Alignment,
			numProperties
		};

		ScriptedViewport(ProcessorWithScriptingContent* base, Content* parentContent, Identifier viewportName, int x, int y, int width, int height);

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptedViewport"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */) override;

		StringArray getOptionsFor(const Identifier &id) override;
		Justification getJustification();
		const StringArray& getItemList() const { return currentItems; }

		void handleDefaultDeactivatedProperties() override;
		void resetValueToDefault() override
		{
			setValue((int)getScriptObjectProperty(defaultValue));
		}

		Array<PropertyWithValue> getLinkProperties() const override;

		LambdaBroadcaster<double, double> positionBroadcaster;

	private:

		StringArray currentItems;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptedViewport);
	};

	

	struct ScriptFloatingTile : public ScriptComponent,
								public Dispatchable
	{
		enum Properties
		{
			itemColour3 = ScriptComponent::Properties::numProperties,
			updateAfterInit,
			ContentType,
			Font,
			FontSize,
			Data,
			numProperties
		};

		// ========================================================================================================

		ScriptFloatingTile(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);
		~ScriptFloatingTile() {};

		// ========================================================================================================

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */) override;

		DynamicObject* createOrGetJSONData()
		{
			auto obj = jsonData.getDynamicObject();

			if (obj == nullptr)
			{
				obj = new DynamicObject();
				jsonData = var(obj);
			}

			return obj;
		}

		bool fillScriptPropertiesWithFloatingTile(FloatingTile* ft);


		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptFloatingTile"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); };
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		StringArray getOptionsFor(const Identifier &id) override;
		
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		
		void handleDefaultDeactivatedProperties() override;

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

			if (auto obj = jsonData.getDynamicObject())
			{
				auto id = obj->getProperty("Type");

				// We need to make sure that the content type is changed so that the floating tile
				// receives an update message
				setScriptObjectProperty(Properties::ContentType, "", dontSendNotification);

				setScriptObjectProperty(Properties::ContentType, id, sendNotification);
			}
			
			//triggerAsyncUpdate();
		}

		// ========================================================================================================

	private:

		struct Wrapper;

		var jsonData;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptFloatingTile);

		// ========================================================================================================
	};


	using ScreenshotListener = ScreenshotListener;

	struct VisualGuide
	{
		enum class Type
		{
			HorizontalLine,
			VerticalLine,
			Rectangle
		};

		Rectangle<float> area;
		Colour c;
		Type t;
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

	/** Adds a image to the script interface. */
	ScriptImage *addImage(Identifier imageName, int x, int y);

	/** Adds a panel (rectangle with border and gradient). */
	ScriptPanel *addPanel(Identifier panelName, int x, int y);

	/** Adds a audio waveform display. */
	ScriptAudioWaveform *addAudioWaveform(Identifier audioWaveformName, int x, int y);

	/** Adds a slider pack. */
	ScriptSliderPack *addSliderPack(Identifier sliderPackName, int x, int y);

	/** Adds a viewport. */
	ScriptedViewport* addViewport(Identifier viewportName, int x, int y);

	/** Adds a floating layout component. */
	ScriptFloatingTile* addFloatingTile(Identifier floatingTileName, int x, int y);

	/** Returns the reference to the given component. */
	var getComponent(var name);
	
	/** Returns the current tooltip. */
	String getCurrentTooltip();

	/** Returns an array of all components that match the given regex. */
    var getAllComponents(String regex);

	/** Restore the Component from a JSON object. */
	void setPropertiesFromJSON(const Identifier &name, const var &jsonData);

	/** sets the data for the value popups. */
	void setValuePopupData(var jsonData);

	/** Creates a Path that can be drawn to a ScriptPanel. */
	var createPath();

	/** Creates an OpenGL framgent shader. */
	var createShader(const String& fileName);

	/** Sets the colour for the panel. */
	void setColour(int red, int green, int blue) { colour = Colour((uint8)red, (uint8)green, (uint8)blue); };

	/** Sets the height of the content. */
	void setHeight(int newHeight) noexcept;

	/** Sets the height of the content. */
	void setWidth(int newWidth) noexcept;

	/** Creates a screenshot of the area relative to the content's origin. */
	void createScreenshot(var area, var directory, String name);

	/** Creates either a line or rectangle with the given colour. */
	void addVisualGuide(var guideData, var colour);

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

	/** Creates a look and feel that you can attach manually to certain components. */
	var createLocalLookAndFeel();

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

	void removeAllScriptComponents();

	UpdateDispatcher* getUpdateDispatcher() { return &updateDispatcher; }

	bool isEmpty();
	int getNumComponents() const noexcept{ return components.size(); };
	ScriptComponent *getComponent(int index);
	const ScriptComponent *getComponent(int index) const { return components[index].get(); };
	ScriptComponent * getComponentWithName(const Identifier &componentName);
	const ScriptComponent * getComponentWithName(const Identifier &componentName) const;
	int getComponentIndex(const Identifier &componentName) const;

	bool hasComponent(const ScriptComponent* sc) const { return components.indexOf(sc) != -1; };

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

	void addScreenshotListener(ScreenshotListener* l)
	{
		screenshotListeners.addIfNotAlreadyThere(l);
	}

	void removeScreenshotListener(ScreenshotListener* l)
	{
		screenshotListeners.removeAllInstancesOf(l);
	}

	var getValuePopupProperties() const { return valuePopupData; };

	ValueTree getValueTreeForComponent(const Identifier& id);

	ValueTreeUpdateWatcher* getUpdateWatcher() { return updateWatcher; }

	void resetContentProperties()
	{
		//MessageManagerLock mmLock;

		rebuildComponentListFromValueTree();
	};

	void valueTreeNeedsUpdate()
	{
#if USE_BACKEND
		ValueTreeUpdateWatcher::ScopedDelayer sd(updateWatcher);
		rebuildComponentListFromValueTree();
#endif
	}

	void addComponentsFromValueTree(const ValueTree& v);

	Result createComponentsFromValueTree(const ValueTree& newProperties, bool buildComponentList=true);

	struct Wrapper;

	struct Helpers
	{
		using ValueTreeIteratorFunction = std::function<bool(ValueTree&)>;

		static bool callRecursive(ValueTree& v, const ValueTreeIteratorFunction& f);

		static void gotoLocation(ScriptComponent* sc);

		static Identifier getUniqueIdentifier(Content* c, const String& id);

		static void deleteComponent(Content* c, const Identifier& id, NotificationType rebuildContent=sendNotification);

		static void deleteSelection(Content* c, ScriptComponentEditBroadcaster* b);

		static bool renameComponent(Content* c, const Identifier& id, const Identifier& newId);

		static void duplicateSelection(Content* c, ReferenceCountedArray<ScriptComponent> selection, int deltaX, int deltaY, UndoManager* undoManager);

		static Colour getCleanedObjectColour(const var& value);

		static void setComponentValueTreeFromJSON(Content* c, const Identifier& id, const var& data, UndoManager* undoManager);

		static Result setParentComponent(ScriptComponent* parent, var newChildren);

		/** Sets the parent component by reordering the internal data structure. */
		static Result setParentComponent(Content* content, const var& parentId, const var& childIdList);

		static void createNewComponentData(Content* c, ValueTree& p, const String& typeName, const String& id);

		template <class T> static T* createComponentIfTypeMatches(ScriptingApi::Content* c, const Identifier& typeId, const Identifier& name, int x, int y, int w, int h)
		{
			if (typeId == T::getStaticObjectName())
				return new T(c->getScriptProcessor(), c, name, x, y, w, h);

			return nullptr;
		}

		static String createScriptVariableDeclaration(ReferenceCountedArray<ScriptComponent> selection);

		static String createCustomCallbackDefinition(ReferenceCountedArray<ScriptComponent> selection);

		static void recompileAndSearchForPropertyChange(ScriptComponent * sc, const Identifier& id);
		static ScriptComponent * createComponentFromValueTree(Content* c, const ValueTree& v);
		static bool hasLocation(ScriptComponent* sc);
		static void sanitizeNumberProperties(juce::ValueTree copy);
		static var getCleanedComponentValue(const var& data, bool allowStrings);
	};

	template <class SubType> SubType* createNewComponent(const Identifier& id, int x, int y)
	{
		static const Identifier xId("x");
		static const Identifier yId("y");
		//static const Identifier wId("width");
		//static const Identifier hId("height");

		ValueTree newData("Component");
		newData.setProperty("type", SubType::getStaticObjectName().toString(), nullptr);
		newData.setProperty("id", id.toString(), nullptr);
		newData.setProperty(xId, x, nullptr);
		newData.setProperty(yId, y, nullptr);
		//newData.setProperty(wId, w, nullptr);
		//newData.setProperty(hId, h, nullptr);

#if JUCE_WINDOWS && USE_BACKEND
		auto undoManager = &getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster()->getUndoManager();
#else
		UndoManager* undoManager = nullptr;
#endif

		{
			ValueTreeUpdateWatcher::ScopedSuspender ss(updateWatcher);
			contentPropertyData.addChild(newData, -1, undoManager);
		}
		
		SubType* newComponent = new SubType(getScriptProcessor(), this, id, x, y, 0, 0);



		components.add(newComponent);

		asyncRebuildBroadcaster.notify();

		return newComponent;
	}

	

	bool asyncFunctionsAllowed() const
	{
		return allowAsyncFunctions;
	}

	void addPanelPopup(ScriptPanel* panel, bool closeOther)
	{
		if (closeOther)
		{
			for (auto p : popupPanels)
			{
				if (p == panel)
					continue;

				p->closeAsPopup();
			}
				

			popupPanels.clear();
		}

		popupPanels.add(panel);
	}

    void setIsRebuilding(bool isCurrentlyRebuilding)
    {
        for(auto rl: rebuildListeners)
        {
            if(rl.get() != nullptr)
                rl->contentRebuildStateChanged(isCurrentlyRebuilding);
        }
    }
    
	void suspendPanelTimers(bool shouldBeSuspended);

	Array<VisualGuide> guides;

private:

	struct AsyncRebuildMessageBroadcaster : public AsyncUpdater
	{
		AsyncRebuildMessageBroadcaster(Content& parent_) :
			parent(parent_)
		{};

		~AsyncRebuildMessageBroadcaster()
		{
			cancelPendingUpdate();
		}

		Content& parent;

		void notify()
		{
			if (MessageManager::getInstance()->isThisTheMessageThread())
			{
				handleAsyncUpdate();
			}
			else
				triggerAsyncUpdate();
		}

	private:

		void handleAsyncUpdate() override
		{
			parent.sendRebuildMessage();
		}
	};

	Array<WeakReference<ScreenshotListener>> screenshotListeners;

	static void initNumberProperties();

    bool isRebuilding = false;
    
	UpdateDispatcher updateDispatcher;

	ReferenceCountedArray<ScriptPanel> popupPanels;

	bool allowAsyncFunctions = false;

	void sendRebuildMessage();

	Array<WeakReference<RebuildListener>> rebuildListeners;

	var templateFunctions;
	var valuePopupData;

	template<class Subtype> Subtype* addComponent(Identifier name, int x, int y)
	{
		if (!allowGuiCreation)
		{
			reportScriptError("Tried to add a component after onInit()");
			return nullptr;
		}

		if (auto sc = getComponentWithName(name))
		{

			sc->handleScriptPropertyChange("x");
			sc->handleScriptPropertyChange("y");
			sc->setScriptObjectProperty(ScriptComponent::Properties::x, x);
			sc->setScriptObjectProperty(ScriptComponent::Properties::y, y);
			return dynamic_cast<Subtype*>(sc);
		}

		ValueTree newChild("Component");
		newChild.setProperty("type", Subtype::getStaticObjectName().toString(), nullptr);
		newChild.setProperty("id", name.toString(), nullptr);
		newChild.setProperty("x", x, nullptr);
		newChild.setProperty("y", y, nullptr);
		contentPropertyData.addChild(newChild, -1, nullptr);

		Subtype *t = new Subtype(getScriptProcessor(), this, name, x, y, 0, 0);

		components.add(t);

		restoreSavedValue(name);

		
		return t;
	}

	

	void restoreSavedValue(const Identifier& id);

	void rebuildComponentListFromValueTree();

	friend class ScriptContentComponent;
	friend class WeakReference<ScriptingApi::Content>;
	WeakReference<ScriptingApi::Content>::Master masterReference;

	bool useDoubleResolution = false;

	ValueTree contentPropertyData;

	ScopedPointer<ValueTreeUpdateWatcher> updateWatcher;

	bool allowGuiCreation;
	int width, height;
	ReferenceCountedArray<ScriptComponent> components; // This is ref counted to allow anonymous controls
	Colour colour;
	String name;
	String tooltip;

	

	AsyncRebuildMessageBroadcaster asyncRebuildBroadcaster;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Content);
	

	// ================================================================================================================
};

using ScriptComponent = ScriptingApi::Content::ScriptComponent;
using ScriptComponentSelection = ReferenceCountedArray<ScriptComponent>;



struct ContentValueTreeHelpers
{
	static void removeFromParent(ValueTree& v)
	{
		v.getParent().removeChild(v, nullptr);
	}

	static Result setNewParent(ValueTree& newParent, ValueTree& child)
	{
		if (newParent.isAChildOf(child))
		{
			return Result::fail("Can't set child as parent of child");
		}

		if (child.getParent() == newParent)
			return Result::ok();

		removeFromParent(child);
		newParent.addChild(child, -1, nullptr);

		return Result::ok();
	}

	static void updatePosition(ValueTree& v, Point<int> localPoint, Point<int> oldParentPosition)
	{
		static const Identifier x("x");
		static const Identifier y("y");

		auto newPoint = localPoint - oldParentPosition;

		v.setProperty(x, newPoint.getX(), nullptr);
		v.setProperty(y, newPoint.getY(), nullptr);
	}

	static Point<int> getLocalPosition(const ValueTree& v)
	{
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier root("ContentProperties");

		if (v.getType() == root)
		{
			return Point<int>();
		}

		return Point<int>(v.getProperty(x), v.getProperty(y));
	}

	static bool isShowing(const ValueTree& v)
	{
		static const Identifier visible("visible");
		static const Identifier co("Component");

		if (v.getProperty(visible, true))
		{
			auto p = v.getParent();

			if (p.getType() == co)
				return isShowing(p);
			else
				return true;
		}
		else
		{
			return false;
		}
	}

	static bool getAbsolutePosition(const ValueTree& v, Point<int>& offset)
	{
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier root("ContentProperties");

		auto parent = v.getParent();

		bool ok = parent.isValid() && parent.getType() != root;

		while (parent.isValid() && parent.getType() != root)
		{
			offset.addXY((int)parent.getProperty(x), (int)parent.getProperty(y));
			parent = parent.getParent();
		}

		return ok;
	}
};



} // namespace hise
#endif  // SCRIPTINGAPICONTENT_H_INCLUDED
