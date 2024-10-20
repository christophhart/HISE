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
		ScopedDelayer(ValueTreeUpdateWatcher* watcher_);;
		~ScopedDelayer();
	private:
		WeakReference<ValueTreeUpdateWatcher> watcher;
	};

	class ScopedSuspender
	{
	public:
		ScopedSuspender(ValueTreeUpdateWatcher* watcher_);;
		~ScopedSuspender();;
	private:
		WeakReference<ValueTreeUpdateWatcher> watcher;
	};

	struct Listener
	{
		virtual ~Listener();;
		virtual void valueTreeNeedsUpdate() = 0;

	private:
		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	ValueTreeUpdateWatcher(ValueTree& v, Listener* l);;

	void valueTreePropertyChanged(ValueTree& /*treeWhosePropertyHasChanged*/, const Identifier& property) override;
	void valueTreeChildAdded(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenAdded*/) override;
	void valueTreeChildRemoved(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override;
	void valueTreeChildOrderChanged(ValueTree& /*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override;
	void valueTreeParentChanged(ValueTree& /*treeWhoseParentHasChanged*/) override;;

private:

	bool delayCalls = false;
	bool shouldCallAfterDelay = false;

	void callListener();

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

	struct ScriptComponent;

	// ================================================================================================================

	class RebuildListener
	{
	public:

		enum class DragAction
		{
			Start,
			End,
			Repaint,
			Query,
			Drag
		};

		virtual ~RebuildListener()
		{
			masterReference.clear();
		}

		virtual void contentWasRebuilt() = 0;
        
        virtual void contentRebuildStateChanged(bool /*isRebuilding*/) {};

		virtual bool onDragAction(DragAction a, ScriptComponent* source, var& data) { return false; }

		virtual void suspendStateChanged(bool isSuspended) {};

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
		NEW_AUTOMATION_WITH_COMMA(dispatch::ListenerOwner)
		public UpdateDispatcher::Listener
	{
		using Ptr = ReferenceCountedObjectPtr<ScriptComponent>;

		using CustomAutomationPtr = MainController::UserPresetHandler::CustomAutomationData::Ptr;

		struct PropertyWithValue
		{
			int id;
			var value = var::undefined();
		};

		struct MouseListenerData
		{
			using StateFunction = std::function<var(int)>;

			WeakReference<WeakCallbackHolder::CallableObject> listener;
			MouseCallbackComponent::CallbackLevel mouseCallbackLevel = MouseCallbackComponent::CallbackLevel::NoCallbacks;
			StateFunction tickedFunction, enabledFunction, textFunction;
			std::function<StringArray()> popupMenuItemFunction = {};
			ModifierKeys popupModifier = ModifierKeys::rightButtonModifier;
			int delayMilliseconds = 0;
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
			automationId,
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

            virtual void wantsToGrabFocus() {};
            
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
            localLookAndFeel = var();
		};

		virtual ValueTree exportAsValueTree() const override;;
		virtual void restoreFromValueTree(const ValueTree &v) override;;

		String getDebugValue() const override { return getValue().toString(); };
		String getDebugName() const override { return name.toString(); };
		String getDebugDataType() const override { return getObjectName().toString(); }
		virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		virtual ValueToTextConverter getValueToTextConverter() const
		{
			return {};
		}

		Location getLocation() const override
		{
			return location;
		}

		Location location;

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

		void updateAutomation(int, float newValue)
		{
			setValue(newValue);
		}

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
		virtual var getValue() const;

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

		/** Registers a selection of key presses to be consumed by this component. */
		void setConsumedKeyPresses(var listOfKeys);

		/** Call this method in order to give away the focus for this component. */
		void loseFocus();

        /** Call this method in order to grab the keyboard focus for this component. */
        void grabFocus();
        
		/** Attaches the local look and feel to this component. */
		void setLocalLookAndFeel(var lafObject);

		/** Manually sends a repaint message for the component. */
		virtual void sendRepaintMessage();

		/** Returns the ID of the component. */
		String getId() const;

		/** Toggles the visibility and fades a component using the global animator. */
		void fadeComponent(bool shouldBeVisible, int milliseconds);

		/** Updates the value from the processor connection. Call this method whenever the module state has changed and you want
			to refresh the knob value to show the current state. */
		void updateValueFromProcessorConnection();

		/** Sets a variable for this component that can be queried from a style sheet. */
		void setStyleSheetProperty(const String& variableId, const var& value, const String& type);

		/** Sets the given class selectors for the component stylesheet. */
		void setStyleSheetClass(const String& classIds);

		/** Programatically sets a pseudo state (:hover, :active, :checked, :focus, :disabled) that will be used by the CSS renderer. */
		void setStyleSheetPseudoState(const String& pseudoState);

		// End of API Methods ============================================================================================

		var getLookAndFeelObject();

		void attachValueListener(WeakCallbackHolder::CallableObject* obj);

		void removeMouseListener(WeakCallbackHolder::CallableObject* obj);

		void attachMouseListener(WeakCallbackHolder::CallableObject* obj, MouseCallbackComponent::CallbackLevel cl, const MouseListenerData::StateFunction& sf = {}, const MouseListenerData::StateFunction& ef = {}, const MouseListenerData::StateFunction& tf = {}, const std::function<StringArray()>& popupItemFunction = {}, ModifierKeys pm = ModifierKeys::rightButtonModifier, int delayMs=0);

		const Array<MouseListenerData>& getMouseListeners() const { return mouseListeners; }

		bool handleKeyPress(const KeyPress& k);

		void handleFocusChange(bool isFocused);

		bool wantsKeyboardFocus() const { return (bool)keyboardCallback; }

		void addSubComponentListener(SubComponentListener* l);

		void removeSubComponentListener(SubComponentListener* l);

		void sendSubComponentChangeMessage(ScriptComponent* s, bool wasAdded, NotificationType notify=sendNotificationAsync);
		
		void setChanged(bool isChanged = true) noexcept{ hasChanged = isChanged; }
		bool isChanged() const noexcept{ return hasChanged; };

		var value;
		Identifier name;
		Content *parent;
		bool skipRestoring;

		ValueTree styleSheetProperties;

		WeakReference<WeakCallbackHolder::CallableObject> valueListener;
		
		Array<MouseListenerData> mouseListeners;

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
			ScopedPropertyEnabler(ScriptComponent* c_);;
			~ScopedPropertyEnabler();
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

		void handleScriptPropertyChange(const Identifier& id);

		/** Returns a local look and feel if it was registered before. */
		LookAndFeel* createLocalLookAndFeel(ScriptContentComponent* contentComponent, Component* componentToRegister);

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

		CustomAutomationPtr getCustomAutomation() { return currentAutomationData; }

		LambdaBroadcaster<bool> repaintBroadcaster;

		LambdaBroadcaster<bool, int> fadeListener;

		void setModulationData(MacroControlledObject::ModulationPopupData::Ptr newMod)
		{
			modulationData = newMod;
		}

		MacroControlledObject::ModulationPopupData::Ptr getModulationData() const { return modulationData; }

		int getStyleSheetPseudoState() const { return pseudoState; }

	protected:

		String getCSSFromLocalLookAndFeel()
		{
			if (auto l = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(localLookAndFeel.getObject()))
			{
				if(l->isUsingCSS())
				{
					return l->currentStyleSheet;
				}
			}

			return {};
		}

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

		CustomAutomationPtr currentAutomationData;

#if USE_BACKEND
		juce::SharedResourcePointer<hise::ScriptComponentPropertyTypeSelector> selectorTypes;
#endif

	private:

		enum class AllCatchBehaviour
		{
			Inactive,
			Exclusive,
			NonExlusive
		};

		int pseudoState = 0;

		void sendValueListenerMessage();

		var localLookAndFeel;

		MacroControlledObject::ModulationPopupData::Ptr modulationData;

        bool consumedCalled = false;
		AllCatchBehaviour catchAllKeys = AllCatchBehaviour::Exclusive;
		Array<juce::KeyPress> registeredKeys;

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
        
		struct AsyncValueUpdater : public AsyncUpdater
		{
			AsyncValueUpdater(ScriptComponent& p) :
				parent(p)
			{};

			void handleAsyncUpdate() override;

			ScriptComponent& parent;
		};
        
        
		struct GlobalCableConnection;

		AsyncControlCallbackSender controlSender;
		AsyncValueUpdater asyncValueUpdater;

		bool isPositionProperty(Identifier id) const;

		ValueTree propertyTree;

		Array<Identifier> scriptChangedProperties;

		IF_NEW_AUTOMATION_DISPATCH(dispatch::library::CustomAutomationSource::Listener automationListener);

        struct SubComponentNotifier: public AsyncUpdater
        {
            SubComponentNotifier(ScriptComponent& p):
              parent(p)
            {};
            
            void handleAsyncUpdate() override;
            
            struct Item
            {
                WeakReference<ScriptComponent> sc;
                bool wasAdded;
            };
            
            hise::SimpleReadWriteLock lock;
            Array<Item> pendingItems;
            
            ScriptComponent& parent;
        } subComponentNotifier;
        
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
			sendValueOnDrag,
			numProperties,
		};

        struct ModifierObject: public ConstScriptingObject
        {
            ModifierObject(ProcessorWithScriptingContent* sp):
               ConstScriptingObject(sp, 12)
            {
                using Action = SliderWithShiftTextBox::ModifierObject::Action;
                using Flags = ModifierKeys::Flags;
                
                addConstant("TextInput", "TextInput");
                addConstant("FineTune", "FineTune");
                addConstant("ResetToDefault", "ResetToDefault");
                addConstant("ContextMenu", "ContextMenu");
                
                static const String doubleClick("doubleClick");
                static const String rightClick("rightClick");
                static const String shiftDown("shiftDown");
                static const String cmdDown("cmdDown");
                static const String altDown("altDown");
                static const String ctrlDown("ctrlDown");
                static const String disabled("disabled");
                static const String noModKey("noKeyModifier");
                
                addConstant(disabled, 0);
                addConstant(noModKey, SliderWithShiftTextBox::ModifierObject::noKeyModifier);
                addConstant(shiftDown, Flags::shiftModifier);
                addConstant(rightClick, Flags::rightButtonModifier);
                addConstant(cmdDown, Flags::commandModifier);
                addConstant(altDown, Flags::altModifier);
                addConstant(ctrlDown, Flags::ctrlModifier);
                addConstant(doubleClick, SliderWithShiftTextBox::ModifierObject::doubleClickModifier);
            };
            
            static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("Modifiers"); }
            
            Identifier getObjectName() const override { return getStaticObjectName(); }
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

		void resetValueToDefault() override;

		void handleDefaultDeactivatedProperties() override;

		ValueToTextConverter getValueToTextConverter() const override
		{
			auto m = getScriptObjectProperty(ScriptSlider::Properties::Mode).toString();
			return ValueToTextConverter::createForMode(m);
		}

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

        /** Creates a object with constants for setModifiers(). */
        var createModifiers();
        
        /** Sets the modifiers for different actions using a JSON object. */
        void setModifiers(String action, var modifiers);
        
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
        var modObject;

        
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
            setValueOnClick,
			mouseCursor,
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

		ValueToTextConverter getValueToTextConverter() const override
		{
			return ValueToTextConverter::createForOptions({ "Off", "On" });
		}

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
            popupAlignment,
            useCustomPopup,
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

		ValueToTextConverter getValueToTextConverter() const override
		{
			auto sa = StringArray::fromLines(getScriptObjectProperty(Properties::Items).toString());
			sa.removeEmptyStrings();
			return ValueToTextConverter::createForOptions(sa);
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
            SendValueEachKeyPress,
			numProperties
		};


		ScriptLabel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int);
		~ScriptLabel();;
		
		// ========================================================================================================

		static Identifier getStaticObjectName();

		virtual Identifier 	getObjectName() const override;
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;
		StringArray getOptionsFor(const Identifier &id) override;
		Justification getJustification();

		void restoreFromValueTree(const ValueTree &v) override;

		ValueTree exportAsValueTree() const override;

		/** Returns the current value. */
		virtual var getValue() const override;

		void setValue(var newValue) override;

		void resetValueToDefault() override;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification);

		void handleDefaultDeactivatedProperties() override;

		bool isClickable() const override;

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
			
		Table* getTable(int) override;

		SliderPackData* getSliderPack(int) override;

		MultiChannelAudioBuffer* getAudioFile(int) override;

		FilterDataObject* getFilterData(int index) override;

		SimpleRingBuffer* getDisplayBuffer(int index) override;

		int getNumDataObjects(ExternalData::DataType t) const override;

		bool removeDataObject(ExternalData::DataType t, int index) override;

		// override this and return the property id used for the index
		virtual int getIndexPropertyId() const = 0;

		StringArray getOptionsFor(const Identifier &id) override;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor = sendNotification) override;

		ValueTree exportAsValueTree() const override;

		void restoreFromValueTree(const ValueTree &v) override;

		void handleDefaultDeactivatedProperties();

		void referToDataBase(var newData);

		ComplexDataUIBase* getCachedDataObject() const;;

		var registerComplexDataObjectAtParent(int index = -1);

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

		ComplexDataUIBase::SourceWatcher& getSourceWatcher();

	protected:

		void updateCachedObjectReference();

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

		void connectToOtherTable(String processorId, int index);

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
			CallbackOnMouseUpOnly,
			StepSequencerMode,
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

		void resetValueToDefault() override;

		var getValue() const override;

		// ======================================================================================================== API Methods

		/** Connects this SliderPack to an existing SliderPackData object. -1 sets it back to its internal data object. */
		void referToData(var sliderPackData);

		/** Enables or disables the control callback execution when the SliderPack is changed via setAllValues. */
		void setAllValueChangeCausesCallback(bool shouldBeEnabled);

		/** sets the slider value at the given index.*/
		void setSliderAtIndex(int index, double value);

		/** Returns the value at the given index. */
		double getSliderValueAt(int index);

		/** Sets all slider values to the given value. If value is a number it will be filled with the number. If it's a buffer (or array) it will set the values accordingly (without resizing the slider packs). */
		void setAllValues(var value);

		/** Like setAllValues, but with undo support (if useUndoManager is enabled). */
		void setAllValuesWithUndo(var value);

		/** Returns the number of sliders. */
		int getNumSliders() const;

		/** Sets a non-uniform width per slider using an array in the form [0.0, ... a[i], ... 1.0]. */
		void setWidthArray(var normalizedWidths);
        
		/** Registers this sliderpack to the script processor to be acessible from the outside. */
		var registerAtParent(int pIndex);

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

		/** Returns a Buffer object containing all slider values (as reference). */
		var getDataAsBuffer();

		/** Sets a preallocated length that will retain values when the slider pack is resized below that limit. */
		void setUsePreallocatedLength(int numMaxSliders);

		// ========================================================================================================

		void changed() override;

		struct Wrapper;

		SliderPackData* getSliderPackData() { return getCachedSliderPack(); };

		Array<var> widthArray;

	private:

		bool allValueChangeCausesCallback = true;

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
			loadWithLeftClick,
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

		/** Set the folder to be used when opening the file browser. */
		void setDefaultFolder(var newDefaultFolder);

		/** Sets the playback position. */
		void setPlaybackPosition(double normalisedPosition);

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
			BlendMode,
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
			MouseCursorInfo();

			MouseCursorInfo(MouseCursor::StandardCursorType t);;

			MouseCursorInfo(const Path& p, Colour c_, Point<float> hp);;

			MouseCursor::StandardCursorType defaultCursorType = MouseCursor::NormalCursor;
			Path path;
			Colour c = juce::Colours::white;
			Point<float> hitPoint = { 0.0f, 0.0f };
		} mouseCursorPath;

		struct AnimationListener
		{
			virtual ~AnimationListener();;

			virtual void animationChanged() = 0;

            virtual void paintRoutineChanged() = 0;
            
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
            bufferToImage,
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

		static Identifier getStaticObjectName();
		virtual Identifier 	getObjectName() const override;

		StringArray getOptionsFor(const Identifier &id) override;
		StringArray getItemList() const;

		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		bool isAutomatable() const override;

		void preloadStateChanged(bool isPreloading) override;

		void preRecompileCallback() override;

		bool updateCyclicReferenceList(ThreadData& data, const Identifier& id) override;

		void prepareCycleReferenceCheck() override;

		void handleDefaultDeactivatedProperties() override;

		int getNumChildElements() const override;

		DebugInformationBase* getChildElement(int index) override;

		DebugInformationBase::Ptr createChildElement(DebugWatchIndex index) const;

		void sendRepaintMessage() override;

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

		/** Starts dragging an external file (or a number of files). */
		bool startExternalFileDrag(var fileOrFilesToDrag, bool moveOriginalFiles, var finishCallback);

		/** Starts dragging something inside the UI. */
		bool startInternalDrag(var dragData);

		/** Loads a image which can be drawn with the paint function later on. */
		void loadImage(String imageName, String prettyName);

		/** Unload all images from the panel. */
		void unloadAllImages();
		
		/** Checks if the image has been loaded into the panel */
		bool isImageLoaded(String prettyName);

		/** If `allowedDragging` is enabled, it will define the boundaries where the panel can be dragged. */
		void setDraggingBounds(var area);

		/** Sets a FloatingTile that is used as popup. The position is a array [x , y, width, height] that is used for the popup dimension */
		void setPopupData(var jsonData, var position);
        
        /** Sets a new value, stores this action in the undo manager and calls the control callbacks. */
        void setPanelValueWithUndo(var oldValue, var newValue, var actionName);
        
		/** Opens the panel as popup. */
		void showAsPopup(bool closeOtherPopups);

		/** Closes the popup manually. */
		void closeAsPopup();

		/** Returns true if the popup is currently showing. */
		bool isVisibleAsPopup();

		/** If this is set to true, the popup will be modal with a dark background that can be clicked to close. */
		void setIsModalPopup(bool shouldBeModal);

		/** Adds a child panel to this panel. */
		var addChildPanel();

		/** Removes the panel from its parent panel if it was created with addChildPanel(). */
		bool removeFromParent();

		/** Returns a list of all panels that have been added as child panel. */
		var getChildPanelList();

		/** Returns the panel that this panel has been added to with addChildPanel. */
		var getParentPanel();

		int getNumSubPanels() const;

		ScriptPanel* getSubPanel(int index);

		// ========================================================================================================

#if HISE_INCLUDE_RLOTTIE
		bool isAnimationActive() const;
		RLottieAnimation::Ptr getAnimation();
#endif

		void showAsModalPopup();

		void forcedRepaint();;

		MouseCursorInfo getMouseCursorPath() const;

		LambdaBroadcaster<MouseCursorInfo>& getCursorUpdater();

		LambdaBroadcaster<MouseCursorInfo> cursorUpdater;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor=sendNotification) override;

		struct Wrapper;

		bool isModal() const;

		bool isUsingCustomPaintRoutine() const;

		bool isUsingClippedFixedImage() const;;

		void scaleFactorChanged(float /*newScaleFactor*/) override;
		// Do nothing until fixed...

		void fileDropCallback(var fileInformation);

		void mouseCallback(var mouseInformation);

		void mouseCallbackInternal(const var& mouseInformation, Result& r);

		var getJSONPopupData() const;

		void cancelPendingFunctions() override;

		void resetValueToDefault() override;

		Rectangle<int> getPopupSize() const;

		void timerCallback() override;

		bool timerCallbackInternal(MainController * mc, Result &r);

		Image getLoadedImage(const String &prettyName) const;;

		Rectangle<int> getDragBounds() const;

		bool isShowing(bool checkParentComponentVisibility=true) const override;

		void repaintWrapped();

		DrawActions::Handler* getDrawActionHandler();

		void addAnimationListener(AnimationListener* l);

		void removeAnimationListener(AnimationListener* l);

		String fileDropExtension;
		String fileDropLevel;

		dispatch::AccumulatedFlowManager flowManager;

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

		ValueToTextConverter getValueToTextConverter() const override
		{
			auto sa = StringArray::fromLines(getScriptObjectProperty(Properties::Items).toString());
			sa.removeEmptyStrings();
			return ValueToTextConverter::createForOptions(sa);
		}

		void setValue(var newValue) override;

		// ============================================================================ API Methods



		/** Turns this viewport into a table with the given metadata. This can only be done in the onInit callback. */
		void setTableMode(var tableMetadata);

		/** Define the columns of the table. This can only be done in the onInit callback. */
		void setTableColumns(var columnMetadata);

		/** Update the row data for the table. */
		void setTableRowData(var tableData);

		/** Set a function that is notified for all user interaction with the table. */
		void setTableCallback(var callbackFunction);

		/** Returns the index of the original data passed into setTableRowData. */
		int getOriginalRowIndex(int rowIndex);

		/** Sets a custom function that can be used in order to sort the table if the user clicks on a column header. */
		void setTableSortFunction(var sortFunction);

		/** Specify the event types that should trigger a setValue() callback. */
		void setEventTypesForValueCallback(var eventTypeList);

		// ============================================================================ API Methods

		Array<PropertyWithValue> getLinkProperties() const override;

		LambdaBroadcaster<double, double> positionBroadcaster;

		ScriptTableListModel::Ptr getTableModel() { return tableModel; }

	private:

		ScriptTableListModel::Ptr tableModel;

		struct Wrapper;

		StringArray currentItems;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptedViewport);
	};

	struct ScriptWebView : public ScriptComponent
	{
		enum Properties
		{
			enableCache = ScriptComponent::numProperties,
			enablePersistence,
			scaleFactorToZoom,
            enableDebugMode
		};

		ScriptWebView(ProcessorWithScriptingContent* base, Content* parentContent, Identifier webViewName, int x, int y, int width, int height);

		~ScriptWebView();

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptWebView"); }

		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */) override;

		void handleDefaultDeactivatedProperties() override;

		hise::WebViewData::Ptr getData() const { return data; }

		// ========================================================================================================= API Methods

		/** Binds a HiseScript function to a Javascript callback id. */
		void bindCallback(const String& callbackId, const var& functionToCall);

		/** Calls the JS function (in the global scope) with the given arguments. */
		void callFunction(const String& javascriptFunction, const var& args);

		/** Evaluates the code in the web view. You need to pass in an unique identifier so that it will initialise new web views correctly. */
		void evaluate(const String& identifier, const String& jsCode);

        /** Sets the file to be displayed by the WebView. */
        void setIndexFile(var indexFile);
        
		/** Resets the entire webview. */
		void reset();

		// =========================================================================================================

	private:

		struct HiseScriptCallback
		{
			HiseScriptCallback(ScriptWebView* wv, const String& callback, const var& function) :
				f(wv->getScriptProcessor(), nullptr, function, 1),
				callbackId(callback)
			{
				f.incRefCount();
				f.setHighPriority();
				f.setThisObject(wv);
			};

			var operator()(const var& args);

			const String& callbackId;
			WeakCallbackHolder f;
		};

		OwnedArray<HiseScriptCallback> callbacks;

		struct Wrapper;

		hise::WebViewData::Ptr data;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptWebView);
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
		~ScriptFloatingTile();;

		// ========================================================================================================

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */) override;

		DynamicObject* createOrGetJSONData();

		bool fillScriptPropertiesWithFloatingTile(FloatingTile* ft);


		static Identifier getStaticObjectName();
		virtual Identifier 	getObjectName() const override;;
		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		StringArray getOptionsFor(const Identifier &id) override;
		
		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;
		
		void handleDefaultDeactivatedProperties() override;

		void setValue(var newValue) override;

		var getValue() const override;

		var getContentData();

		// ========================================================================================================

		/** Sets the JSON object for the given floating tile. */
		void setContentData(var data);

		// ========================================================================================================

	private:

		struct Wrapper;

		var jsonData;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptFloatingTile);

		// ========================================================================================================
	};

	/*

		TODO:

		- implement all other callbacks
		- remove all Broadcaster stuff again OK
		- add export & import from .dat file && Base64
		- remove all floating tile stuff OK
		- fix hiding of backdrop when opening new window OK
		- fix recompilation while function is in progress OK
		- add font, text colour & other colour properties OK
		- append to Stylesheet content, not replace
		- big todo: make debugger in scriptwatchtable popup (or even as floating tile?)
		- big todo: remove Javascript API altogether?
		- add API for handling modal popups within dialog
		- add mp.navigate(index, bool submitCurrentPage)
		- add mp.refresh() individual
		- fix stopped tasks being repeated & stuck when stopped & reloaded window OK
		- add mp.exportAsJSON(file)
	 */
	struct ScriptMultipageDialog: public ScriptComponent
	{
		enum Properties
		{
			Font  = ScriptComponent::Properties::numProperties,
			FontSize,
			EnableConsoleOutput,
			DialogWidth,
			DialogHeight,
			UseViewport,
			StyleSheet,
			ConfirmClose,
			numProperties
		};

		// ========================================================================================================

		ScriptMultipageDialog(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height);
		~ScriptMultipageDialog() override;;

		// ========================================================================================================

		void setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */) override;

		static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptMultipageDialog"); }
		virtual Identifier 	getObjectName() const override { return getStaticObjectName(); }

		ScriptCreatedComponentWrapper *createComponentWrapper(ScriptContentComponent *content, int index) override;

		StringArray getOptionsFor(const Identifier &id) override;
		
		void handleDefaultDeactivatedProperties() override;

		void setValue(var newValue) override {};

		var getValue() const override { return var(); }

		// ========================================================================================================

		/** Clears the dialog. */
		void resetDialog();

		/** Adds a page to the dialog and returns the element index of the page. */
		int addPage();

		/** Adds a modal page to the dialog that can be populated like a normal page and shown using showModalPage(). */
		int addModalPage();

		/** Shows a modal page with the given index and the state object. */
		void showModalPage(int pageIndex, var modalState, var finishCallback);

		/** Adds an element to the parent with the given type and properties. */
		int add(int parentIndex, String type, const var& properties);

		/** Registers a callable object to the dialog and returns the codestring that calls it from within the dialogs Javascript engine. */
		String bindCallback(String id, var callback, var notificationType);

		/** Registers a function that will be called when the dialog is finished. */
		void setOnFinishCallback(var onFinish);

		/** Registers a function that will be called when the dialog shows a new page. */
		void setOnPageLoadCallback(var onPageLoad);

		/** Shows the dialog (with optionally clearing the state. */
		void show(bool clearState);

		/** Navigates to the given page index. */
		bool navigate(int pageIndex, bool submitCurrentPage)
		{
			getMultipageState()->currentPageIndex = pageIndex;

			if(submitCurrentPage && getMultipageState()->getFirstDialog() != nullptr)
			{
				SafeAsyncCall::call<multipage::State>(*getMultipageState(), [pageIndex](multipage::State& s)
				{
					s.currentPageIndex = pageIndex - 1;
					s.getFirstDialog()->navigate(true);
				});
				
				return true;
			}
			else
			{
				SafeAsyncCall::call<multipage::State>(*getMultipageState(), [pageIndex](multipage::State& s)
				{
					s.currentPageIndex = pageIndex;

					for(auto f: s.currentDialogs)
						f->refreshCurrentPage();
					
				});

				return true;
			}
		}

		/** Closes the dialog (as if the user pressed the cancel button). */
		void cancel();

		/** Sets the property for the given element ID and updates the dialog. */
		void setElementProperty(int elementId, String propertyId, const var& newValue);

		/** Sets the value of the given element ID and calls the callback. */
		void setElementValue(int elementId, var value);

		/** Returns the value for the given element ID. */
		var getElementProperty(int elementId, String propertyId) const;

		/** returns the state object for the dialog. */
		var getState();

		/** Loads the dialog from a file (on the disk). */
		void loadFromDataFile(var fileObject);

		/** Exports the entire dialog. */
		String exportAsMonolith(var optionalFile);

		// ========================================================================================================

		void sendRepaintMessage() override
		{
			if(backdropBroadcaster.getLastValue<0>() != Backdrop::MessageType::Hide)
				backdropBroadcaster.sendMessage(sendNotificationAsync, Backdrop::MessageType::RefreshDialog);
		}

		void preRecompileCallback() override;

		static void onMultipageLog(ScriptMultipageDialog& m, multipage::MessageType mt, const String& message);

		multipage::State::Ptr getMultipageState()
		{
			if(multiState == nullptr)
			{
				multiState = new multipage::State(dialogData, {});

#if USE_BACKEND
				multiState->eventLogger.addListener(*this, onMultipageLog);
#endif
			}
				

			return multiState;
		}

		struct Backdrop: public Component
		{
			enum class MessageType
			{
				Hide,
				Show,
				RefreshDialog,
				numBackdropMessages
			};

			Backdrop(ScriptMultipageDialog* d):
			  dialogData(d)
			{
				setInterceptsMouseClicks(true, true);
			};

			void mouseDown(const MouseEvent& e)
			{
				if(closeOnClick)
					dialogData->cancel();
			}
			
			void create(const String& cssToUse={})
			{
				destroy();

				if(dialogData != nullptr)
				{
					auto state = dialogData->getMultipageState();
					auto data = dialogData->createDialogData(cssToUse);

					
					addAndMakeVisible(currentDialog = new multipage::Dialog(data, *state.get(), true));


					currentDialog->setFinishCallback(BIND_MEMBER_FUNCTION_0(Backdrop::onFinish));

					currentDialog->loadStyleFromPositionInfo();
					currentDialog->refreshCurrentPage();

					closeOnClick = !dialogData->getScriptObjectProperty(Properties::ConfirmClose);
						
					setVisible(true);
					resized();
				}

			}

			void onFinish()
			{
				if(dialogData.get())
					dialogData->cancel();
				//dialogData->backdropBroadcaster.sendMessage(sendNotificationSync, MessageType::Hide);
			}

			void paint(Graphics& g)
			{
				g.fillAll(juce::Colours::black.withAlpha(0.9f));
			}

			void destroy()
			{
				if(currentDialog != nullptr)
				{
					MessageManagerLock mm;
					currentDialog = nullptr;
				}
			}

			static void onMessage(Backdrop& bp, MessageType mt)
			{
				if(mt == MessageType::Show)
				{
					bp.create();
				}
				if(mt == MessageType::Hide)
				{
					bp.destroy();
				}
				if(mt == MessageType::RefreshDialog)
				{
					auto colour = bp.dialogData->getScriptObjectProperty(ScriptComponent::Properties::bgColour);

					bp.bgColour = ApiHelpers::getColourFromVar(colour);
					bp.repaint(); 

					if(bp.currentDialog != nullptr)
						bp.currentDialog->refreshCurrentPage();
				}
			}

			void resized() override
			{
				if(currentDialog != nullptr && !getLocalBounds().isEmpty())
				{
					auto size = currentDialog->getPositionInfo({}).fixedSize;
					currentDialog->centreWithSize(size.getX(), size.getY());
				}
			}

			bool closeOnClick = false;
			Colour bgColour;
			WeakReference<ScriptMultipageDialog> dialogData;
			ScopedPointer<multipage::Dialog> currentDialog;
			JUCE_DECLARE_WEAK_REFERENCEABLE(Backdrop);
		};

		Backdrop* createBackdrop()
		{
			auto bp = new Backdrop(this);
			backdropBroadcaster.addListener(*bp, Backdrop::onMessage, false);
			
			if(isDialogVisible())
				bp->create();

			return bp;
		}

	private:

		File monolithFile;

		static void handleVisibility(ScriptMultipageDialog& d, Backdrop::MessageType t)
		{
			auto isVisible = (bool)d.getScriptObjectProperty(ScriptComponent::Properties::visible);
			auto shouldBeVisible = t != Backdrop::MessageType::Hide;

			if(isVisible != shouldBeVisible)
			{
				ScopedPropertyEnabler sds(&d);
				d.set("visible", shouldBeVisible);

				if(!shouldBeVisible)
				{
					d.getMultipageState()->onDestroy();
				}
			}
		}

		struct ValueCallback
		{
			ValueCallback(ScriptMultipageDialog* p, String name_, const var& functionToCall, dispatch::DispatchType n_):
			  callback(p->getScriptProcessor(), p, functionToCall, 2),
			  name(name_),
			  n(n_)
			{
				callback.incRefCount();
				callback.setThisObject(p);
				args[0] = var(name);
			};

			var operator()(const var::NativeFunctionArgs& a);

			String name;
			var args[2];
			WeakCallbackHolder callback;
			const dispatch::DispatchType n;
		};

		ScopedPointer<ValueCallback> onModalFinish;

		OwnedArray<ValueCallback> valueCallbacks;

		var createDialogData(String cssToUse={});

		int addPageInternal(bool isModal);

		Array<var> modalPages;
		Array<var> pages;
		Array<var> elementData;
		
		LambdaBroadcaster<Backdrop::MessageType> backdropBroadcaster;

		bool isDialogVisible() const
		{
			return backdropBroadcaster.getLastValue<0>() != Backdrop::MessageType::Hide;
		}

		var dialogData;

		void clearState()
		{
			if(multiState != nullptr)
			{
				multiState->stopThread(1000);

				MessageManagerLock mm;

				Array<WeakReference<multipage::Dialog>> dialogCopy;
				dialogCopy.addArray(multiState->currentDialogs);

				for(auto d: dialogCopy)
				{
					if(d != nullptr)
						d->onStateDestroy(sendNotificationSync);
				}
			}

			multiState = nullptr;
		}

		struct Wrapper;

		multipage::State::Ptr multiState;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptMultipageDialog);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptMultipageDialog);

		// ========================================================================================================
	};

	using ScreenshotListener = hise::ScreenshotListener;

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

    struct TextInputDataBase: public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<TextInputDataBase>;
        
        TextInputDataBase(const String& parentId_):
          parentId(parentId_)
        {};
        
        virtual ~TextInputDataBase() {};
        
        virtual void show(Component* parentComponent) = 0;
        
        bool done = false;
        String parentId;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextInputDataBase);
    };
    
    LambdaBroadcaster<TextInputDataBase::Ptr> textInputBroadcaster;

	LambdaBroadcaster<int, int> interfaceSizeBroadcaster;
    
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

	/** Adds a web view. */
	ScriptWebView* addWebView(Identifier webviewName, int x, int y);

	/** Adds a floating layout component. */
	ScriptFloatingTile* addFloatingTile(Identifier floatingTileName, int x, int y);

	/** Adds a multipage dialog component. */
	ScriptMultipageDialog* addMultipageDialog(Identifier dialogId, int x, int y);

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

    /** Creates an SVG object from the converted Base64 String. */
    var createSVG(const String& base64String);
    
	/** Creates a MarkdownRenderer. */
	var createMarkdownRenderer();

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

    /** Opens a text input box with the given properties and executes the callback when finished. */
    void showModalTextInput(var properties, var callback);
    
    /** Sets this script as main interface with the given size. */
    void makeFrontInterface(int width, int height);
    
	/** Sets this script as main interface with the given device resolution (only works with mobile devices). */
	void makeFullScreenInterface();

	/** Returns the total bounds of the main display. */
	var getScreenBounds(bool getTotalArea);

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
	
	/** Checks whether the CTRL key's flag is set. */
	bool isCtrlDown();

	/** Creates a look and feel that you can attach manually to certain components. */
	var createLocalLookAndFeel();

	/** Returns 1 if the left mouse button is clicked somewhere on the interface and 2 if the right button is clicked. */
	int isMouseDown();

	/** Returns the name of the component that is currently hovered. */
	String getComponentUnderMouse();

	/** Calls a function after a delay. This is not accurate and only useful for UI purposes!. */
	void callAfterDelay(int milliSeconds, var function, var thisObject);

	/** Calls the paint function of the drag operation again to refresh the image. */
	bool refreshDragImage();

	/** Returns the ID of the component under the mouse. */
	String getComponentUnderDrag();

	/** Sets a callback that will be notified whenever the UI timers are suspended. */
	void setSuspendTimerCallback(var suspendFunction);

	/** Adds a callback that will be performed asynchronously when the key is pressed. */
	void setKeyPressCallback(const var& keyPress, var keyPressCallback);

	// ================================================================================================================

	static var createKeyboardCallbackObject(const KeyPress& k)
	{
		auto obj = new DynamicObject();
		var args(obj);

		obj->setProperty("isFocusChange", false);

		auto c = k.getTextCharacter();

		auto printable    = CharacterFunctions::isPrintable(c);
		auto isWhitespace = CharacterFunctions::isWhitespace(c);
		auto isLetter     = CharacterFunctions::isLetter(c);
		auto isDigit      = CharacterFunctions::isDigit(c);
		
		obj->setProperty("character", printable ? String::charToString(c) : "");
		obj->setProperty("specialKey", !printable);
		obj->setProperty("isWhitespace", isWhitespace);
		obj->setProperty("isLetter", isLetter);
		obj->setProperty("isDigit", isDigit);
		obj->setProperty("keyCode", k.getKeyCode());
		obj->setProperty("description", k.getTextDescription());
		obj->setProperty("shift", k.getModifiers().isShiftDown());
		obj->setProperty("cmd", k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown());
		obj->setProperty("alt", k.getModifiers().isAltDown());

		return args;
	}

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

	StringArray getMacroNames();

	bool hasComponent(const ScriptComponent* sc) const { return components.indexOf(sc) != -1; };

	int getContentHeight() const { return height; }
	int getContentWidth() const { return width; }

	void recompileAndThrowAtDefinition(ScriptComponent* sc);

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

        static String createLocalLookAndFeelForComponents(ReferenceCountedArray<ScriptComponent> selection);
        
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
		auto undoManager = nullptr; //&getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster()->getUndoManager();
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

		updateParameterSlots();

		return newComponent;
	}

	

	bool interfaceCreationAllowed() const;

	bool asyncFunctionsAllowed() const
	{
		return allowAsyncFunctions;
	}

	void addPanelPopup(ScriptPanel* panel, bool closeOther);

	void setIsRebuilding(bool isCurrentlyRebuilding);

	void sendDragAction(RebuildListener::DragAction a, ScriptComponent* sc, var& data);

	void suspendPanelTimers(bool shouldBeSuspended);

	Array<VisualGuide> guides;

	bool hasKeyPressCallbacks() const { return !registeredKeyPresses.isEmpty(); }

	bool handleKeyPress(const KeyPress& k)
	{
		auto k1 = k.getKeyCode();
		auto m1 = k.getModifiers();

		for(auto& rkp: registeredKeyPresses)
		{
			auto k2 = rkp.first.getKeyCode();
			auto m2 = rkp.first.getModifiers();
			
			if(k1 == k2 && m1 == m2)
			{
				auto obj = createKeyboardCallbackObject(k);
				WeakCallbackHolder f(getScriptProcessor(), nullptr, rkp.second, 1);
				f.call1(obj);
				return true;
			}
		}

		return false;
	}

private:

	WeakCallbackHolder dragCallback;
	WeakCallbackHolder suspendCallback;

	Array<std::pair<KeyPress, var>> registeredKeyPresses;

	struct AsyncRebuildMessageBroadcaster : public AsyncUpdater
	{
		AsyncRebuildMessageBroadcaster(Content& parent_);;

		~AsyncRebuildMessageBroadcaster();

		Content& parent;

		void notify();

	private:

		void handleAsyncUpdate() override;
	};

	Array<Identifier> modifiers;

	Array<WeakReference<ScreenshotListener>> screenshotListeners;

	static void initNumberProperties();

    bool isRebuilding = false;
    
	UpdateDispatcher updateDispatcher;

	ReferenceCountedArray<ScriptPanel> popupPanels;

	bool allowAsyncFunctions = false;

	void sendRebuildMessage();

	Array<WeakReference<RebuildListener>> rebuildListeners;

	WeakReference<ScriptComponent> componentToThrowAtDefinition;

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

		updateParameterSlots();
		
		restoreSavedValue(name);
		
		return t;
	}

	void updateParameterSlots();

	void restoreSavedValue(const Identifier& id);

	void rebuildComponentListFromValueTree();

	friend class ScriptContentComponent;
	friend class WeakReference<ScriptingApi::Content>;
	WeakReference<ScriptingApi::Content>::Master masterReference;

	bool useDoubleResolution = false;

	ValueTree contentPropertyData;

	ScopedPointer<ValueTreeUpdateWatcher> updateWatcher;

	bool allowGuiCreation;
	int width = 0;
	int height = 0;
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
	static void removeFromParent(ValueTree& v);

	static Result setNewParent(ValueTree& newParent, ValueTree& child);

	static void updatePosition(ValueTree& v, Point<int> localPoint, Point<int> oldParentPosition);

	static Point<int> getLocalPosition(const ValueTree& v);

	static bool isShowing(const ValueTree& v);

	static bool getAbsolutePosition(const ValueTree& v, Point<int>& offset);
};

struct MapItemWithScriptComponentConnection : public Component,
	public ComponentWithPreferredSize,
	public PooledUIUpdater::SimpleTimer
{
	MapItemWithScriptComponentConnection(ScriptComponent* c, int width, int height);;

	int getPreferredWidth() const override { return w; }
	int getPreferredHeight() const override { return h; }

	int w = 0;
	int h = 0;

	WeakReference<ScriptComponent> sc;
};



struct SimpleVarBody : public ComponentWithPreferredSize,
	public Component
{
	SimpleVarBody(const var& v);

	void paint(Graphics& g) override;

	static ComponentWithPreferredSize* create(Component* root, const var& v)
	{
		return new SimpleVarBody(v);
	};

	void mouseDown(const MouseEvent& e);



	String getSensibleStringRepresentation() const;

	int getPreferredWidth() const override { return 128; }
	int getPreferredHeight() const override { return 32; };

	var value;
	String s;
};

struct LiveUpdateVarBody : public SimpleVarBody,
	public PooledUIUpdater::SimpleTimer
{
	enum DisplayType
	{
		Text,
		Bool,
		Colour,
		numDisplayType
	};

	static DisplayType getDisplayType(const Identifier& id);

	LiveUpdateVarBody(PooledUIUpdater* updater, const Identifier& id_, const std::function<var()>& f);;

	void timerCallback() override;

	String getTextToDisplay() const;

	int getPreferredWidth() const override { return 35 + GLOBAL_MONOSPACE_FONT().getStringWidth(getTextToDisplay()); }

	void paint(Graphics& g) override;

	ModValue alpha;
	const Identifier id;
	std::function<var()> valueFunction;
	const DisplayType displayType;
};

struct PrimitiveArrayDisplay : public SimpleVarBody,
	public PooledUIUpdater::SimpleTimer
{
	PrimitiveArrayDisplay(Processor* jp, const var& obj);;

	void timerCallback() override;

	int h;
	int w;

	String id;

	int getPreferredWidth() const override { return w; };
	int getPreferredHeight() const override { return h; }

	void paint(Graphics& g) override;

	static ComponentWithPreferredSize* create(Component* r, const var& obj);

	Array<var> lastValues;
};


} // namespace hise
#endif  // SCRIPTINGAPICONTENT_H_INCLUDED
