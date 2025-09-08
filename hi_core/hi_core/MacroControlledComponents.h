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

#pragma once

namespace hise { using namespace juce;



class Processor;

class TouchAndHoldComponent
{
public:

	TouchAndHoldComponent();

	virtual ~TouchAndHoldComponent();

	virtual void touchAndHold(Point<int> downPosition) = 0;
	
	void startTouch(Point<int> downPosition);

	void setDragDistance(float newDistance);

	void abortTouch();

	bool isTouchEnabled() const;

	void setTouchEnabled(bool shouldBeEnabled);

private:

	struct UpdateTimer : public Timer
	{
		UpdateTimer(TouchAndHoldComponent* parent_);

		void startTouch(Point<int>& newDownPosition);

		void setDragDistance(float newDistance);

		void timerCallback();

	private:

		Point<int> downPosition;

		float dragDistance;

		TouchAndHoldComponent* parent;
	};

	bool touchEnabled = true;

	UpdateTimer updateTimer;
};

struct Learnable
{
	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	};
	struct LearnData
	{
		String processorId;
		String parameterId;
		float value;
		String name;
		NormalisableRange<double> range;
		String mode;
		StringArray items;
	};

	Component* asComponent() { return dynamic_cast<Component*>(this); };
	const Component* asComponent() const { return dynamic_cast<const Component*>(this); };

	virtual ~Learnable() {};

	void setLearnable(bool shouldBeLearnable)
	{
		learnable = shouldBeLearnable;
	}

	bool isLearnable() const
	{
		return learnable;
	}

private:

	bool learnable = true;
};


struct HisePluginParameterBase: public ControlledObject,
								public dispatch::ListenerOwner,
							    public AsyncUpdater

{
	enum class Type
	{
		Macro,
		CustomAutomation,
		ScriptControl,
		NKSWrapper,
		numTypes
	};

	HisePluginParameterBase(MainController* mc, int index):
	  ControlledObject(mc),
	  autoListener(getMainController()->getRootDispatcher(), *this, BIND_MEMBER_FUNCTION_2(HisePluginParameterBase::onUpdate)),
	  parameterIndex(index)
	{}

	virtual ~HisePluginParameterBase()
	{
		// If this happens you haven't called cleanup before destroying this plugin parameter!
		jassert(cleanupCalled);
	}

	int getHiseParameterIndex() { return parameterIndex; }

	static int defaultSort(HisePluginParameterBase* p1, HisePluginParameterBase* p2)
	{
		auto i1 = p1->getHiseParameterIndex();
		auto i2 = p2->getHiseParameterIndex();

		if(i1 < i2)
			return -1;
		if (i1 > i2)
			return 1;

		return 0;
	}

	virtual Type getType() const = 0;
	virtual NormalisableRange<float> getNormalisableRange() const = 0;
	virtual int getSlotIndex() const = 0;
	virtual ValueToTextConverter getValueToTextConverter() const = 0;
	virtual String getHisePluginParameterName() const = 0;
	virtual String getHisePluginParameterGroupName() const = 0;
	virtual float getHisePluginParameterNormalisedValue() const = 0;

#if USE_BACKEND
	virtual HisePluginParameterBase* getWrappedParameter() { return this; }
#else
	HisePluginParameterBase* getWrappedParameter() { return this; }
#endif

	bool matchesIndex(int slotIndex) const { return getSlotIndex() == slotIndex; }

	void onUpdate(int index, float v)
	{
		FloatSanitizers::sanitizeFloatNumber(v);
		v = getNormalisableRange().convertTo0to1(v);

		if(v != parameterValueToSend)
		{
			parameterValueToSend = v;

			if(sendToHost)
				refreshParameterValue();
		}
	}

	void refreshParameterValue()
	{
		auto mc = getMainController();
		auto t = mc->getKillStateHandler().getCurrentThread();
		auto defer = mc->getDeferNotifyHostFlag() || t != MainController::KillStateHandler::TargetThread::MessageThread;

		if(defer)
			triggerAsyncUpdate();
		else
			handleAsyncUpdate();
	}

	void handleAsyncUpdate() override
	{
		auto mc = getMainController();
		
		ScopedValueSetter<bool> svs(recursive, true);
		ScopedValueSetter<bool> setter(mc->getPluginParameterUpdateState(), false, true);

		getWrappedParameter()->asJuceParameter()->setValueNotifyingHost(parameterValueToSend);
	}

    virtual void cleanup() { cleanupCalled = true; }
    
	int parameterIndex = -1;
	float parameterValueToSend = 0.0f;
	bool recursive = false;

	dispatch::library::CustomAutomationSource::Listener autoListener;

	juce::AudioProcessorParameter* asJuceParameter()
	{
		auto typed = dynamic_cast<juce::AudioProcessorParameter*>(this);
		jassert(typed != nullptr);
		return typed;
	}

protected:

	bool sendToHost = true;

private:

	bool cleanupCalled = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HisePluginParameterBase);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HisePluginParameterBase);
};

/** A base class for all control Components that can be controlled by a MacroControlBroadcaster.
*	@ingroup macroControl
*
*	Subclass your component from this class, and set the Processor's Attribute in its specified callback.
*	
*/
class MacroControlledObject: public MacroControlBroadcaster::MacroConnectionListener,
							 public dispatch::ListenerOwner,
						     public Learnable
{
public:
	
	struct ModulationPopupData: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ModulationPopupData>;

		ModulationPopupData(const String& targetId_):
		  targetId(targetId_)
		{};

		operator bool() const noexcept;

		/** Override this method and populate the popup menu for the given target. */
		virtual void addToPopupMenu(MacroControlledObject* parentComponent, PopupMenu& m) = 0;

		/** Override this method and perform the result if matching and return true if consumed. */
		virtual bool onPopupMenuResult(MacroControlledObject* parentComponent, int result) = 0;

		String targetId;
		
		//std::function<bool(int, bool)> queryFunction;
		//std::function<void(int, bool)> toggleFunction;
		//std::function<void(double)> valueCallback;
		//std::function<void(String)> editCallback;
	};

	class UndoableControlEvent: public UndoableAction
	{
	public:
		UndoableControlEvent(Processor* p_, int parameterIndex_, float oldValue_, float newValue_);

		bool perform() override;

		bool undo() override;

	private:

		WeakReference<Processor> processor;
		int parameterIndex;
		float newValue;
		float oldValue;
	};

	/** Creates a new MacroControlledObject. 
	*
	*	You have to call setup() before you use the object!
	*/
	MacroControlledObject();;
    
    virtual ~MacroControlledObject();;

	/** returns the name. */
	const String getName() const noexcept;;

	void setAttributeWithUndo(float newValue, bool useCustomOldValue=false, float customOldValue=-1.0f);

	void setCanBeMidiLearned(bool shouldBe);

	void macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded) override;

	bool canBeMidiLearned() const;

	bool isConnectedToModulator() const;

	void setUseUndoManagerForEvents(bool shouldUseUndo);

	/** Initializes the control element.
	*
	*	It connects to a Processor's parameter and automatically updates its value and changes the attribute. 
	*	@param p the Processor that is controlled by the element.
	*	@param parameter_ the parameter that is controlled by the element
	*	@param name_ the name of the element. This will also be the displayed name in the macro control panel.
	*/
	virtual void setup(Processor *p, int parameter_, const String &name_);

	void connectToCustomAutomation(const Identifier& newCustomId);

	int getAutomationIndex() const;

	void initMacroControl(NotificationType notify);

	virtual void addToMacroController(int newMacroIndex);;

	virtual void removeFromMacroController();

	/** overwrite this method and update the element to display the current value of the controlled attribute. */
	virtual void updateValue(NotificationType sendAttributeChange = sendNotification) = 0;

	/** overwrite this method and return the range that the parameter can have. */
	virtual NormalisableRange<double> getRange() const = 0;

	virtual ValueToTextConverter getValueToTextConverter() const = 0;

	bool isLocked();

	/** Since the original setEnabled() is overwritten in the updateValue, use this method instead to enable / disable MacroControlledComponents. */
	void enableMacroControlledComponent(bool shouldBeEnabled) noexcept;

	bool isReadOnly();

	int getMacroIndex() const;
	
	int getParameter() const;;

	void setModulationData(ModulationPopupData::Ptr modData);


	void onAttributeChange(dispatch::library::Processor* p, uint8 index);

	juce::AudioProcessorParameter* getConnectedPluginParameter() const;

	void rebuildPluginParameterConnection();

	bool skipHostDisplayUpdate = false;

	void checkMouseClickProfiler(bool isDown);

protected:

	ScopedPointer<dispatch::library::Processor::AttributeListener> valueListener;

    friend class SliderWithShiftTextBox;
    
	/** checks if the macro learn mode is active.
	*
	*	If it is active, it adds this control to the macro and returns true.
	*	You can use this in the element's callback like:
	*
	*		if(!checkLearnMode())
	*		{
	*			getProcessor()->setAttribute(parameter, value, dontSendNotification);
	*		}
	*
	*/
	bool checkLearnMode();
	
	Processor *getProcessor();;

	const Processor *getProcessor() const;;

	int parameter;

	void enableMidiLearnWithPopup();

	ScopedPointer<NumberTag> numberTag;

private:

	ModulationPopupData::Ptr modulationData;

	Identifier customId;

	ScopedPointer<LookAndFeel> slaf;

	bool midiLearnEnabled = true;

	bool useUndoManagerForEvents = true;

	WeakReference<Processor> processor;
	
	int macroIndex;
	String name;

	bool macroControlledComponentEnabled;

	WeakReference<HisePluginParameterBase> connectedPluginParameter;

};



/** A combobox which can be controlled by the macro system. */
class HiComboBox: public SubmenuComboBox,
				  public ComboBox::Listener,
				  public MacroControlledObject,
				  public ProfiledComponent,
                  public TouchAndHoldComponent
{
public:

	HiComboBox(const String &name);;

    ~HiComboBox();

	void setup(Processor *p, int parameter, const String &name) override;

	void updateValue(NotificationType sendAttributeChange = sendNotification) override;

	void comboBoxChanged(ComboBox *c) override;
               
    void mouseUp(const MouseEvent& e) override;

	void touchAndHold(Point<int> downPosition) override;
    
	void resized() override;
	
    void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent& e) override;

	paintAndProfileChildren(g);

	ValueToTextConverter getValueToTextConverter() const override;


	bool customPopup = false;
    
	NormalisableRange<double> getRange() const override;;
	
	Font font;
};

class MomentaryToggleButton: public ToggleButton
{
public:
    
    MomentaryToggleButton(const String& name);;
    
    void setIsMomentary(bool shouldBeMomentary);

    void mouseDown(const MouseEvent& e) override;

    void mouseUp(const MouseEvent& e) override;

private:
    
    bool isMomentary = false;
};

class HiToggleButton: public MomentaryToggleButton,
					  public Button::Listener,
				      public MacroControlledObject,
                      public TouchAndHoldComponent,
					  public ProfiledComponent
{
public:

	HiToggleButton(const String &name);;

    ~HiToggleButton();

	void setup(Processor *p, int parameter, const String &name) override;

	void updateValue(NotificationType sendAttributeChange = sendNotification) override;

	void buttonClicked(Button *b) override;

	void setNotificationType(NotificationType notify);

	ValueToTextConverter getValueToTextConverter() const override
	{
		ValueToTextConverter c;
		c.active = true;
		c.itemList = { "Off", "On" };

		return c;
	}

	void setPopupData(const var& newPopupData, Rectangle<int>& newPopupPosition);

	void setLookAndFeelOwned(LookAndFeel *fslaf);

    void mouseDown(const MouseEvent &e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override;

	paintAndProfileChildren(g);

	void touchAndHold(Point<int> downPosition) override;
    
	void resized() override;

	NormalisableRange<double> getRange() const override;;
	
private:

	var popupData;
	Rectangle<int> popupPosition;
	Component::SafePointer<Component> currentPopup;
	NotificationType notifyEditor;
	ScopedPointer<LookAndFeel> laf;
};


class SliderWithShiftTextBox : public TextEditor::Listener
{
public:

    struct ModifierObject
    {
        static constexpr int doubleClickModifier = 512;
        static constexpr int noKeyModifier = 1024;
        
        enum class Action
        {
            NoAction,
            TextInput,
            FineTune,
            ResetToDefault,
            ContextMenu,
			ScaleModulation,
            numActions
        };

        ModifierKeys getModifiers(Action a)
        {
            auto f = mods1[(int)a];
            f &= ~doubleClickModifier;
            return ModifierKeys((int)f);
        }
        
        void setFromObject(const var& obj)
        {
            auto setFromIntOrArray = [&](Action a, const Identifier& id)
            {
                if(!obj.hasProperty(id))
                {
                    mods1[(int)a] = getDefaultFlags(a);
                    mods2[(int)a] = 0;
                    mods3[(int)a] = 0;
                    return;
                }
                
                auto t = obj.getProperty(id, 0);
                
                if(t.isArray())
                {
                    mods1[(int)a] = (uint64)(int64)t[0];
                    mods2[(int)a] = (uint64)(int64)t[1];
                    
                    if(t.size() > 2)
                        mods3[(int)a] = (uint64)(int64)t[2];
                    else
                        mods3[(int)a] = 0;
                }
                else
                {
                    mods1[(int)a] = (uint64)(int64)t;
                    mods2[(int)a] = 0;
                    mods3[(int)a] = 0;
                }
            };
            
            setFromIntOrArray(Action::TextInput, "TextInput");
            setFromIntOrArray(Action::ResetToDefault, "ResetToDefault");
            setFromIntOrArray(Action::FineTune, "FineTune");
            setFromIntOrArray(Action::ContextMenu, "ContextMenu");
			setFromIntOrArray(Action::ScaleModulation, "ScaleModulation");
        }
        
        static uint64 getDefaultFlags(Action a)
        {
            switch(a)
            {
                case Action::TextInput:
                    return ModifierKeys::shiftModifier;
            case Action::ResetToDefault:
                    return doubleClickModifier | ModifierKeys::altModifier;
                case Action::FineTune:
                    return  ModifierKeys::commandModifier |
                            ModifierKeys::ctrlModifier |
                            ModifierKeys::altModifier;
                case Action::ContextMenu:
                    return ModifierKeys::rightButtonModifier;
            case Action::ScaleModulation:
					return 0;
                default: return 0;
            }
        }
        
        Action getActionForModifier(ModifierKeys m, bool isDoubleClick) const
        {
            auto flags = (uint64)m.getRawFlags();

            if(isDoubleClick)
                flags |= doubleClickModifier;

            if(!m.isAnyModifierKeyDown())
                flags |= noKeyModifier;
            
            for(auto i = (int)Action::TextInput; i < (int)Action::numActions; i++)
            {
                auto firstMatch = (mods1[i] & flags) != 0;
                
                if(mods2[i])
                    firstMatch &= (flags & mods2[i]) != 0;
                
                if(mods3[i])
                    firstMatch &= (flags & mods3[i]) != 0;
                
                if(firstMatch)
                    return (Action)i;
            }
            
            return Action::NoAction;
        }

        ModifierObject()
        {
            for(int i = 0; i < (int)Action::numActions; i++)
            {
                mods1[i] = getDefaultFlags((Action)i);
                mods2[i] = 0;
                mods3[i] = 0;
            }
            
            // make sure it's only the no key modifier when double clicking...
            mods2[(int)Action::ResetToDefault] = noKeyModifier;
        }

        uint64 mods1[(int)Action::numActions];
        uint64 mods2[(int)Action::numActions];
        uint64 mods3[(int)Action::numActions];
        
    } modObject;
    
    void setModifierObject(const var& obj)
    {
        modObject.setFromObject(obj);
        
        auto fineMods = modObject.getModifiers(ModifierObject::Action::FineTune);
        
        auto swappable = asSlider()->getVelocityModeIsSwappable();
        auto threshold = asSlider()->getVelocityThreshold();
        auto sensitivity = asSlider()->getVelocitySensitivity();
        auto offset = asSlider()->getVelocityOffset();
        
        asSlider()->setVelocityModeParameters (sensitivity, threshold, offset, swappable, (ModifierKeys::Flags)fineMods.getRawFlags());
    }
    
	bool enableShiftTextInput = true;

    bool performModifierAction(const MouseEvent& e, bool isDoubleClick, bool isMouseDown = true)
    {
        auto a = modObject.getActionForModifier(e.mods, isDoubleClick);
        
        if(isMouseDown && a == ModifierObject::Action::TextInput)
        {
            onShiftClick(e);
			return true;
        }
        if(isMouseDown && a == ModifierObject::Action::ResetToDefault)
        {
            if(asSlider()->isDoubleClickReturnEnabled())
            {
                auto defaultValue = asSlider()->getDoubleClickReturnValue();
                asSlider()->setValue(defaultValue, sendNotificationSync);
                return true;
            }
        }
        if(isMouseDown && a == ModifierObject::Action::ContextMenu)
        {
            if(auto mco = dynamic_cast<MacroControlledObject*>(this))
				mco->enableMidiLearnWithPopup();
			else if(customPopupFunction)
				customPopupFunction(e);

            return true;
        }
		if(a == ModifierObject::Action::ScaleModulation)
		{
			if(scaleFunction)
			{
				float delta = ModulationDisplayValue::getDeltaForDragEvent(*asSlider(), e);
				
				return scaleFunction(isMouseDown, delta);
			}

			return true;
		}
        
        return false;
    }

protected:

	std::function<void(const MouseEvent&)> customPopupFunction;

	std::function<bool(bool, float)> scaleFunction;

	virtual ~SliderWithShiftTextBox();;

	void init();

	void cleanup();

	virtual void onTextValueChange(double newValue);

	void updateValueFromLabel(bool updateValue);

	void textEditorFocusLost(TextEditor&) override;

	void textEditorReturnKeyPressed(TextEditor&) override;

	void textEditorEscapeKeyPressed(TextEditor&) override;

	bool onShiftClick(const MouseEvent& e);

	
	ScopedPointer<TextEditor> inputLabel;

	Slider* asSlider();
	const Slider* asSlider() const;
};

/** A custom Slider class that automatically sets up its properties according to the specified mode.
*
*	You can call setup() after you created the Slider and it will be skinned using the global HiLookAndFeel
*	and its range, skew factor and suffix are specified.
*/
class HiSlider: public juce::Slider,
			    public SliderWithShiftTextBox,
				public MacroControlledObject,
				public SliderListener,
				public ProfiledComponent,
				public TouchAndHoldComponent,
			    public DragAndDropTarget
{
public:

	/** The HiSlider can be configured using one of these modes. */
	enum Mode
	{
		Frequency = 0, ///< Range 20 - 20 000, Suffix "Hz", Midpoint: 1.5kHz
		Decibel, ///< Range -100 - 0, Suffix "dB", Midpoint -18dB
		Time, ///< Range 0 - 20 000, Suffix "ms", Midpoint 1000ms
		TempoSync, ///< Range 0 - numTempos, @see TempoSyncer
		Linear, ///< Range min - max
		Discrete, ///< Range min - max, Stepsize integer
		Pan, ///< 100L - 100R, Stepsize integer
		NormalizedPercentage, ///< 0.0 - 1.0, Displayed as percentage
		numModes
	};

	struct HoverPopupLookandFeel
	{
		virtual ~HoverPopupLookandFeel() = default;

		struct DrawData
		{
			Rectangle<float> bounds;
			int sourceIndex = -1;
			String sourceName;
			String targetName;
			String labelText;
			Range<double> intensityRange;
			float intensityValue = 0.0f;
			scriptnode::modulation::TargetMode targetMode = scriptnode::modulation::TargetMode::Gain;

			bool isHover = false;
			bool isDown = false;
		};

		struct PositionData
		{
			operator bool() const { return draggers.getNumRectangles() > 0; }

			void fromVar(const var& data);
			var toVar() const;

			RectangleList<int> draggers;
			Rectangle<int> labelArea;
			int margin = 10;
			SliderStyle s = SliderStyle::RotaryHorizontalVerticalDrag;
			float sensitivity = 1.0f;
		};

		virtual PositionData getModulatorDragData(HiSlider& s, const StringArray& sourceList) const;
		virtual void drawModulationDragBackground(Graphics& g, HiSlider& s, const DrawData& dd, Rectangle<int> labelBounds);
		virtual void drawModulationDragger(Graphics& g, HiSlider& s, const DrawData& dd);
	};

	static NormalisableRange<double> getRangeForMode(HiSlider::Mode m);;

	/** Creates a Slider. The name will be displayed. 
	*
	*	The slider must be initialized using setup().
	*/
	HiSlider(const String &name);;

    ~HiSlider() override;

	

	static double getFrequencyFromTextString(const String& t);

	void mouseEnter(const MouseEvent& event) override;

	void mouseExit(const MouseEvent& e) override;

	void showModHoverPopup(bool shouldShow, bool closeOnExit);

	void mouseDown(const MouseEvent &e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent&) override;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
    
    void mouseDoubleClick(const MouseEvent& e) override;

	void touchAndHold(Point<int> downPosition) override;

	void onTextValueChange(double newValue) override;

	void setSendValueOnDrag(bool shouldSend) { sendValueOnDrag = shouldSend; }

	void resized() override;

	ValueToTextConverter getValueToTextConverter() const override;

	String getModeId() const;

	void setMode(Mode m);

	/** sets the mode. */
	void setMode(Mode m, NormalisableRange<double> nr);

	Mode getMode() const;

	/* initialises the slider. You must call this after creation before you use this component! */
	void setup(Processor *p, int parameter, const String &name) override;

	void sliderValueChanged(Slider *s) override;

	void sliderDragStarted(Slider* s) override;

	void sliderDragEnded(Slider* s) override;

	bool changePluginParameter(AudioProcessor* p, int parameterIndex);

	

	bool isUsingModulatedRing() const noexcept;;

	void setDisplayValue(float)
	{
		// forgot to set this up
		jassert(isUsingModulatedRing());
	}

	float getDisplayValue() const;

	NormalisableRange<double> getRange() const override;

	void focusLost(FocusChangeType cause) override
	{
		currentHoverPopup = nullptr;
		Slider::focusLost(cause);
	}

	void updateValue(NotificationType sendAttributeChange=sendNotification) override;

	/** Overrides the slider method to display the tempo names for the TempoSync mode. */
	String getTextFromValue(double value) override;;

	/** Overrides the slider method to set the value from the Tempo names */
	double getValueFromText(const String &text) override;;

	void setLookAndFeelOwned(LookAndFeel *fslaf);

	static double getSkewFactorFromMidPoint(double minimum, double maximum, double midPoint);

	static String getSuffixForMode(HiSlider::Mode mode, float panValue);

	paintAndProfileChildren(g);

	

	HoverPopupLookandFeel& getHoverPopupLookAndFeel();
	struct HoverPopup;

	ScopedPointer<Component> currentHoverPopup = nullptr;

	struct ModUpdater: public PooledUIUpdater::SimpleTimer
	{
		ModUpdater(HiSlider& s, MainController* mc):
		 SimpleTimer(mc->getGlobalUIUpdater(), false),
		 parent(s)
		{};

		void timerCallback() override;
		void setUpdateFunction(const ModulationDisplayValue::QueryFunction::Ptr f);;
		bool canBeDropped(const var& info) const;
		void onDrop(const var& info);

		ModulationDisplayValue::QueryFunction::Ptr modFunction;
		HiSlider& parent;
		ModulationDisplayValue lastValue;
	} ;

	bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override
	{
		return modUpdater != nullptr && modUpdater->canBeDropped(dragSourceDetails.description);
	}

    void itemDragEnter (const SourceDetails& d) override
	{
		if(isInterestedInDragSource(d))
		{
			getProperties().set("modulationDragState", 2);
			repaint();
		}
		
	}

    void itemDragExit (const SourceDetails& d) override
	{
		if(isInterestedInDragSource(d))
		{
			getProperties().set("modulationDragState", 1);
			repaint();
		}
	}

    void itemDropped (const SourceDetails& dragSourceDetails) override
	{
		if(modUpdater != nullptr)
		{
			modUpdater->onDrop(dragSourceDetails.description);
			showModHoverPopup(true, false);
		}
	};

private:

	ScopedPointer<ModUpdater> modUpdater;
	bool sendValueOnDrag = true;
	String getModeSuffix() const;;
	Mode mode;
	double modeValues[numModes];
	double dragStartValue = 0.0f;
	ScopedPointer<LookAndFeel> laf;
	HoverPopupLookandFeel fallback;
};


} // namespace hise

