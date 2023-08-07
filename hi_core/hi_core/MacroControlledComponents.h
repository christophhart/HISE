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
	
	struct ModulationPopupData: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ModulationPopupData>;

		operator bool() const noexcept;

		String modulationId;
		StringArray sources;
		std::function<bool(int, bool)> queryFunction;
		std::function<void(int, bool)> toggleFunction;
		std::function<void(double)> valueCallback;
		std::function<void(String)> editCallback;
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

	bool isLocked();

	/** Since the original setEnabled() is overwritten in the updateValue, use this method instead to enable / disable MacroControlledComponents. */
	void enableMacroControlledComponent(bool shouldBeEnabled) noexcept;

	bool isReadOnly();

	int getMacroIndex() const;
	
	int getParameter() const;;

	void setModulationData(ModulationPopupData::Ptr modData);

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
	
};

/** A combobox which can be controlled by the macro system. */
class HiComboBox: public ComboBox,
				  public ComboBox::Listener,
				  public MacroControlledObject,
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
                      public TouchAndHoldComponent
{
public:

	HiToggleButton(const String &name);;

    ~HiToggleButton();

	void setup(Processor *p, int parameter, const String &name) override;

	void updateValue(NotificationType sendAttributeChange = sendNotification) override;

	void buttonClicked(Button *b) override;

	void setNotificationType(NotificationType notify);


	void setPopupData(const var& newPopupData, Rectangle<int>& newPopupPosition);

	void setLookAndFeelOwned(LookAndFeel *fslaf);

    void mouseDown(const MouseEvent &e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override;
	

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

	bool enableShiftTextInput = true;

protected:

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
				public TouchAndHoldComponent
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

	static void setRangeSkewFactorFromMidPoint(NormalisableRange<double>& range, const double midPoint);

	static double getMidPointFromRangeSkewFactor(const NormalisableRange<double>& range);

	static NormalisableRange<double> getRangeForMode(HiSlider::Mode m);;

	/** Creates a Slider. The name will be displayed. 
	*
	*	The slider must be initialized using setup().
	*/
	HiSlider(const String &name);;

    ~HiSlider() override;

	static String getFrequencyString(float input);

	static double getFrequencyFromTextString(const String& t);

	void mouseDown(const MouseEvent &e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent&) override;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

	void touchAndHold(Point<int> downPosition) override;

	void onTextValueChange(double newValue) override;

	void resized() override;

	String getModeId() const;

	void setMode(Mode m);


	/** sets the mode. */
	void setMode(Mode m, double min, double max, double mid=DBL_MAX, double stepSize=DBL_MAX);;

	Mode getMode() const;


	/* initialises the slider. You must call this after creation before you use this component! */
	void setup(Processor *p, int parameter, const String &name) override;

	void sliderValueChanged(Slider *s) override;

	void sliderDragStarted(Slider* s) override;

	void sliderDragEnded(Slider* s) override;

	/** If the slider represents a modulated attribute (eg. LFO Frequency), this can be used to set the displayed value. 
	*
	*	In order to use this functionality, add a timer callback to your editor and update the value using the ModulatorChain's getOutputValue().
	*/
	void setDisplayValue(float newDisplayValue);

	bool isUsingModulatedRing() const noexcept;;

	void setIsUsingModulatedRing(bool shouldUseModulatedRing);;

	float getDisplayValue() const;

	void updateValue(NotificationType sendAttributeChange=sendNotification) override;

	/** Overrides the slider method to display the tempo names for the TempoSync mode. */
	String getTextFromValue(double value) override;;

	/** Overrides the slider method to set the value from the Tempo names */
	double getValueFromText(const String &text) override;;

	void setLookAndFeelOwned(LookAndFeel *fslaf);

	NormalisableRange<double> getRange() const override;;

	static double getSkewFactorFromMidPoint(double minimum, double maximum, double midPoint);

	static String getSuffixForMode(HiSlider::Mode mode, float panValue);

private:

	String getModeSuffix() const;;

	void setModeRange(double min, double max, double mid, double stepSize);;
	
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
