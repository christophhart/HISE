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

#ifndef MACROCONTROLLEDCOMPONENTS_H_INCLUDED
#define MACROCONTROLLEDCOMPONENTS_H_INCLUDED

namespace hise { using namespace juce;

#include <limits>

class Processor;

class TouchAndHoldComponent
{
public:

	TouchAndHoldComponent():
		updateTimer(this)
	{

	}
	
	virtual ~TouchAndHoldComponent()
	{
		abortTouch();
	}
	
	virtual void touchAndHold(Point<int> downPosition) = 0;
	
	void startTouch(Point<int> downPosition)
	{
		if (isTouchEnabled())
		{
			updateTimer.startTouch(downPosition);
		}
	}

	void setDragDistance(float newDistance)
	{
		updateTimer.setDragDistance(newDistance);
	}

	void abortTouch()
	{
		updateTimer.stopTimer();
	}

	bool isTouchEnabled() const
	{
		return touchEnabled && HiseDeviceSimulator::isMobileDevice();
	}

	void setTouchEnabled(bool shouldBeEnabled)
	{
		touchEnabled = shouldBeEnabled;
	}

private:

	struct UpdateTimer : public Timer
	{
		UpdateTimer(TouchAndHoldComponent* parent_):
			parent(parent_),
			dragDistance(0.0f)
		{}

		void startTouch(Point<int>& newDownPosition)
		{
			downPosition = newDownPosition;
			startTimer(1000);
		}

		void setDragDistance(float newDistance)
		{
			dragDistance = newDistance;
		}

		void timerCallback()
		{
			stopTimer();

			if (dragDistance < 8.0f)
			{
				parent->touchAndHold(downPosition);
			}
		}

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


/** A base class for all control Components that can be controlled by a MacroControlBroadcaster.
*	@ingroup macroControl
*
*	Subclass your component from this class, and set the Processor's Attribute in its specified callback.
*	
*/
class MacroControlledObject: public MacroControlBroadcaster::MacroConnectionListener,
						     public Learnable
{
public:
	
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
	MacroControlledObject():
        parameter(-1),
		processor(nullptr),
		macroIndex(-1),
		name(""),
		numberTag(new NumberTag(3, 14.0f, Colour(SIGNAL_COLOUR))),
		macroControlledComponentEnabled(true)
	{};
    
    virtual ~MacroControlledObject();;

	/** returns the name. */
	const String getName() const noexcept { return name; };

	void setAttributeWithUndo(float newValue, bool useCustomOldValue=false, float customOldValue=-1.0f);

	void setCanBeMidiLearned(bool shouldBe)
	{
		midiLearnEnabled = shouldBe;
	}

	void macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded) override;

	bool canBeMidiLearned() const;

	bool isConnectedToModulator() const;

	void setUseUndoManagerForEvents(bool shouldUseUndo) { useUndoManagerForEvents = shouldUseUndo; }

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

	virtual void addToMacroController(int newMacroIndex)
	{ 
		numberTag->setNumber(newMacroIndex+1);
		numberTag->setVisible(true);
		macroIndex = newMacroIndex; 
	};

	virtual void removeFromMacroController() 
	{ 
		numberTag->setNumber(0);
		numberTag->setVisible(false);
		macroIndex = -1;	
	}

	/** overwrite this method and update the element to display the current value of the controlled attribute. */
	virtual void updateValue(NotificationType sendAttributeChange = sendNotification) = 0;

	/** overwrite this method and return the range that the parameter can have. */
	virtual NormalisableRange<double> getRange() const = 0;

	bool isLocked();

	/** Since the original setEnabled() is overwritten in the updateValue, use this method instead to enable / disable MacroControlledComponents. */
	void enableMacroControlledComponent(bool shouldBeEnabled) noexcept
	{
		macroControlledComponentEnabled = shouldBeEnabled;
	}

	bool isReadOnly();

	int getMacroIndex() const;
	
	int getParameter() const { return parameter; };

protected:

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
	
	Processor *getProcessor() {return processor.get(); };

	const Processor *getProcessor() const {return processor.get(); };

	int parameter;

	void enableMidiLearnWithPopup();

	ScopedPointer<NumberTag> numberTag;

private:

	Identifier customId;

	ScopedPointer<LookAndFeel> slaf;

	bool midiLearnEnabled = true;

	bool useUndoManagerForEvents = true;

	WeakReference<Processor> processor;
	
	int macroIndex;
	String name;

	bool macroControlledComponentEnabled;
	
};

/** A combobox which can be controlled by the macro system. */
class HiComboBox: public ComboBox,
				  public ComboBox::Listener,
				  public MacroControlledObject,
                  public TouchAndHoldComponent
{
public:

	HiComboBox(const String &name):
        ComboBox(name),
		MacroControlledObject()
	{
		addChildComponent(numberTag);
		font = GLOBAL_FONT();

		addListener(this);

        setWantsKeyboardFocus(false);
        
        setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
        setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
        setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
        setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
	};

    ~HiComboBox()
    {
        setLookAndFeel(nullptr);
    }
    
	void setup(Processor *p, int parameter, const String &name) override;

	void updateValue(NotificationType sendAttributeChange = sendNotification) override;

	void comboBoxChanged(ComboBox *c) override;
               
    void mouseUp(const MouseEvent& e) override
    {
        abortTouch();
        ComboBox::mouseUp(e);
    }
    
    void touchAndHold(Point<int> downPosition) override;
    
	void resized() override
	{
		ComboBox::resized();
		numberTag->setBounds(getLocalBounds());
	}

#if 0
	static void comboBoxPopupMenuFinishedCallback(int result, HiComboBox* combo)
	{
		if (combo != nullptr)
		{
            
			combo->hidePopup();

			if (result != 0)
				combo->setSelectedId(result);

			

			//combo->addItemsToMenu(*combo->getRootMenu());
		}
	}


	void showPopup() override
	{
		PopupMenu menu = *getRootMenu();

		//addItemsToMenu(menu);

		menu.setLookAndFeel(&getLookAndFeel());
		

		

		menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this)
			.withItemThatMustBeVisible(getSelectedId())
			.withMinimumWidth(getWidth())
			.withMaximumNumColumns(1)
			.withStandardItemHeight(28),
			ModalCallbackFunction::forComponent(comboBoxPopupMenuFinishedCallback, this));
	}
#endif

    void mouseDown(const MouseEvent &e);

	NormalisableRange<double> getRange() const override 
	{ 
		NormalisableRange<double> r(1.0, (double)getNumItems()); 

		r.interval = 1.0;

		return r;
	};
	
	Font font;
};

class HiToggleButton: public ToggleButton,
					  public Button::Listener,
				      public MacroControlledObject,
                      public TouchAndHoldComponent
{
public:

	HiToggleButton(const String &name):
		ToggleButton(name),
        MacroControlledObject(),
		notifyEditor(dontSendNotification)
	{
		addChildComponent(numberTag);
		addListener(this);
		setWantsKeyboardFocus(false);
        
        setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
        setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
        setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
	};

    ~HiToggleButton()
    {
        setLookAndFeel(nullptr);
    }

	void setup(Processor *p, int parameter, const String &name) override;

	void updateValue(NotificationType sendAttributeChange = sendNotification) override;

	void buttonClicked(Button *b) override;

	void setNotificationType(NotificationType notify)
	{
		notifyEditor = notify;
	}

	void setIsMomentary(bool shouldBeMomentary)
	{
		isMomentary = shouldBeMomentary;
	}

	void setPopupData(const var& newPopupData, Rectangle<int>& newPopupPosition)
	{
		popupData = newPopupData;
		popupPosition = newPopupPosition;
	}

	void setLookAndFeelOwned(LookAndFeel *fslaf);

    void mouseDown(const MouseEvent &e) override;

	void mouseUp(const MouseEvent& e) override;

    void touchAndHold(Point<int> downPosition) override;
    
	void resized() override
	{
		ToggleButton::resized();
		numberTag->setBounds(getLocalBounds());
	}

	NormalisableRange<double> getRange() const override 
	{ 
		NormalisableRange<double> r(0.0, 1.0); 
		r.interval = 1.0;

		return r;
	};
	
private:

	var popupData;
	Rectangle<int> popupPosition;

	Component::SafePointer<Component> currentPopup;

	bool isMomentary = false;

	NotificationType notifyEditor;
	ScopedPointer<LookAndFeel> laf;
};




/** A custom Slider class that automatically sets up its properties according to the specified mode.
*
*	You can call setup() after you created the Slider and it will be skinned using the global HiLookAndFeel
*	and its range, skew factor and suffix are specified.
*/
class HiSlider: public juce::Slider,
				public MacroControlledObject,
				public SliderListener,
				public TouchAndHoldComponent,
				public TextEditor::Listener
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

	static void setRangeSkewFactorFromMidPoint(NormalisableRange<double>& range, const double midPoint)
	{
		const double length = range.end - range.start;

		if (range.end > range.start && range.getRange().contains(midPoint))
			range.skew = std::log(0.5) / std::log((midPoint - range.start)
				/ (length));
	}

	static double getMidPointFromRangeSkewFactor(const NormalisableRange<double>& range)
	{
		const double length = range.end - range.start;

		return std::pow(2.0, -1.0 / range.skew) * length + range.start;
	}

	static NormalisableRange<double> getRangeForMode(HiSlider::Mode m)
	{
		NormalisableRange<double> r;

		switch(m)
		{
		case Frequency:				r = NormalisableRange<double>(20.0, 20000.0, 1);
									setRangeSkewFactorFromMidPoint(r, 1500.0);
									break;
		case Decibel:				r = NormalisableRange<double>(-100.0, 0.0, 0.1);
									setRangeSkewFactorFromMidPoint(r, -18.0);
									break;
		case Time:					r = NormalisableRange<double>(0.0, 20000.0, 1);
									setRangeSkewFactorFromMidPoint(r, 1000.0);
									break;
		case TempoSync:				r = NormalisableRange<double>(0, TempoSyncer::numTempos-1, 1);
									break;
		case Pan:					r = NormalisableRange<double>(-100.0, 100.0, 1);
									break;
		case NormalizedPercentage:	r = NormalisableRange<double>(0.0, 1.0, 0.01);									
									break;
		case Linear:				r = NormalisableRange<double>(0.0, 1.0, 0.01); 
									break;
		case Discrete:				r = NormalisableRange<double>();
									r.interval = 1;
									break;
        case numModes: 
		default:					jassertfalse; 
									r = NormalisableRange<double>();
		}

		return r;
	};

	/** Creates a Slider. The name will be displayed. 
	*
	*	The slider must be initialized using setup().
	*/
	HiSlider(const String &name);;

    ~HiSlider()
    {
        setLookAndFeel(nullptr);
    }
    
	static String getFrequencyString(float input)
	{
		if (input < 30.0f)
		{
			return String(input, 1) + " Hz";
		}
		if (input < 1000.0f)
		{
			return String(roundToInt(input)) + " Hz";
		}
		else
		{
			return String(input / 1000.0, 1) + " kHz";
		}
	}
	
	static double getFrequencyFromTextString(const String& t)
	{
		if (t.contains("kHz"))
			return t.getDoubleValue() * 1000.0;
		else
			return t.getDoubleValue();
	}

	void mouseDown(const MouseEvent &e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent&) override;

	void touchAndHold(Point<int> downPosition) override;

	
	void updateValueFromLabel(bool updateValue);
	
	void textEditorFocusLost(TextEditor&) override;

	void textEditorReturnKeyPressed(TextEditor&) override;

	void textEditorEscapeKeyPressed(TextEditor&) override;

	void resized() override
	{
		Slider::resized();
		numberTag->setBounds(getLocalBounds());
	}

	String getModeId() const;

	void setMode(Mode m)
	{
		if (mode != m)
		{
			mode = m;

			normRange = getRangeForMode(m);

			setTextValueSuffix(getModeSuffix());

			setRange(normRange.start, normRange.end, normRange.interval);
			setSkewFactor(normRange.skew);

			setValue(modeValues[m], dontSendNotification);

			repaint();
		}
	}



	/** sets the mode. */
	void setMode(Mode m, double min, double max, double mid=DBL_MAX, double stepSize=DBL_MAX)
	{ 
		

		if(mode != m)
		{
			mode = m; 
			setModeRange(min, max, mid, stepSize);
			setTextValueSuffix(getModeSuffix());

			setValue(modeValues[m], dontSendNotification);

			repaint();
		}
		else
		{
			setModeRange(min, max, mid, stepSize);
		}
	};

	Mode getMode() const { return mode; }

	

	/* initialises the slider. You must call this after creation before you use this component! */
	void setup(Processor *p, int parameter, const String &name) override;

	void sliderValueChanged(Slider *s) override;

	void sliderDragStarted(Slider* s) override;

	void sliderDragEnded(Slider* s) override;

	/** If the slider represents a modulated attribute (eg. LFO Frequency), this can be used to set the displayed value. 
	*
	*	In order to use this functionality, add a timer callback to your editor and update the value using the ModulatorChain's getOutputValue().
	*/
	void setDisplayValue(float newDisplayValue)
	{
        if(newDisplayValue != displayValue)
        {
            displayValue = newDisplayValue;
            repaint();
        }
	}

	bool isUsingModulatedRing() const noexcept{ return useModulatedRing; };

	void setIsUsingModulatedRing(bool shouldUseModulatedRing) { useModulatedRing = shouldUseModulatedRing; };

	float getDisplayValue() const
	{
		return useModulatedRing ? displayValue : 1.0f;
	}

	void updateValue(NotificationType sendAttributeChange=sendNotification) override;

	/** Overrides the slider method to display the tempo names for the TempoSync mode. */
	String getTextFromValue(double value) override
	{
		if(mode == Pan) setTextValueSuffix(getModeSuffix());

		if (mode == Frequency) return getFrequencyString((float)value);
		if(mode == TempoSync) return TempoSyncer::getTempoName((int)(value));
		else if(mode == NormalizedPercentage) return String((int)(value * 100)) + "%";
		else				  return Slider::getTextFromValue(value);
	};

	/** Overrides the slider method to set the value from the Tempo names */
	double getValueFromText(const String &text) override
	{
		if (mode == Frequency) return getFrequencyFromTextString(text);
		if(mode == TempoSync) return TempoSyncer::getTempoIndex(text);
		else if (mode == NormalizedPercentage) return text.getDoubleValue() / 100.0;
		else				  return Slider::getValueFromText(text);
	};

	void setLookAndFeelOwned(LookAndFeel *fslaf);

	NormalisableRange<double> getRange() const override { return normRange; };

	static double getSkewFactorFromMidPoint(double minimum, double maximum, double midPoint)
	{
		if (maximum > minimum)
			return log(0.5) / log((midPoint - minimum) / (maximum - minimum));
		
		jassertfalse;
		return 1.0;
	}

	static String getSuffixForMode(HiSlider::Mode mode, float panValue)
	{
		jassert(mode != numModes);



		switch (mode)
		{
		case Frequency:		return " Hz";
		case Decibel:		return " dB";
		case Time:			return " ms";
		case Pan:			return panValue > 0.0 ? "R" : "L";
		case TempoSync:		return String();
		case Linear:		return String();
		case Discrete:		return String();
		case NormalizedPercentage:	return "%";
		default:			return String();
		}
	}

private:

	ScopedPointer<TextEditor> inputLabel;

	String getModeSuffix()
	{
		return getSuffixForMode(mode, (float)modeValues[Pan]);
	};

	void setModeRange(double min, double max, double mid, double stepSize)
	{
		jassert(mode != numModes);

		normRange = NormalisableRange<double>();

		normRange.start = min;
		normRange.end = max;
		
		normRange.interval = stepSize != DBL_MAX ? stepSize : 0.01;
			

		if(mid != DBL_MAX)
			setRangeSkewFactorFromMidPoint(normRange, mid);

		setRange(normRange.start, normRange.end, normRange.interval);
		setSkewFactor(normRange.skew);
	};
	
	Mode mode;

	bool useModulatedRing;

	double modeValues[numModes];

	double dragStartValue = 0.0f;

	float displayValue;

	NormalisableRange<double> normRange;
	ScopedPointer<LookAndFeel> laf;
	//ScopedPointer<Component> stupidComponent;

};


} // namespace hise

#endif  // MACROCONTROLLEDCOMPONENTS_H_INCLUDED
