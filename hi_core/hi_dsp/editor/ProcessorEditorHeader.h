#ifndef PROCESSOREDITORHEADER_H_INCLUDED
#define PROCESSOREDITORHEADER_H_INCLUDED


//	===================================================================================================
/** A ProcessorEditorHeader is a bar that resides on top of every ProcessorEditor.
*	@ingroup dspEditor
*
*/
class ProcessorEditorHeader  : public ProcessorEditorChildComponent,
                               public SliderListener,
							   public LabelListener,
                               public ButtonListener,
							   public Timer
{
public:

    //==============================================================================
	ProcessorEditorHeader (BetterProcessorEditor *p);
    ~ProcessorEditorHeader();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void update();
	void createProcessorFromPopup(Processor *insertBeforeSibling=nullptr);

	void timerCallback() override;

	void refreshShapeButton(ShapeButton *b);

	void labelTextChanged(Label *l) override;;
    


	/** This sets the ProcessorEditorLookAndFeel for different ProcessorEditorTypes.
	*
	*	The type is detected automatically
	*/
	void setLookAndFeel();
	
	void displayFoldState(bool shouldBeFolded) { foldButton->setToggleState(shouldBeFolded, dontSendNotification); };

	void displayBypassedChain(bool isBypassed)
	{
		bypassButton->setEnabled(isBypassed);
		valueMeter->setEnabled(isBypassed);
	};

	/** If a header of a Chain, this disables the header (and only enables the add button) if the chain is empty. */
	void enableChainHeader();

	bool isHeaderOfModulator() const;

	bool isHeaderOfModulatorSynth() const;

	bool isHeaderOfMidiProcessor() const;
	
	bool isHeaderOfEffectProcessor() const;

	/** Returns the Modulation::Mode of the ModulatorChain. */
	int getModulatorMode() const;
	
	/** Checks if the header is a header of a EditorChain subclasse. */
	bool isHeaderOfChain() const;

	/** returns false if the header is a chain editor and the chain is empty. */
	bool isHeaderOfEmptyChain() const;

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;

	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent&) override;

	void setupButton(DrawableButton *b, ButtonShapes::Symbol s);

private:

	void addProcessor(Processor *processorToBeAdded, Processor *insertBeforeSibling);

	void setPlotButton(bool on);

	void checkFoldButton();

	void checkSoloLabel();

	bool isChainHeader;
	bool isPitchMode;

	BiPolarSliderLookAndFeel pitchModeLookAndFeel;

	AlertWindowLookAndFeel alertLaf;

	bool isBypassedFlag;
	bool valueFlag;
	bool intensityFlag;
	bool isSoloHeader;
	String parentName;
	

	ScopedPointer<ProcessorEditorHeaderLookAndFeel> luf;

	VUSliderLookAndFeel vulaf;

	BalanceButtonLookAndFeel bbluf;

	BiPolarSliderLookAndFeel bpslaf;

    //==============================================================================

	ScopedPointer<Slider> balanceSlider;

	ScopedPointer<ChainIcon> chainIcon;

    ScopedPointer<VuMeter> valueMeter;
    ScopedPointer<Label> idLabel;
    ScopedPointer<Label> typeLabel;
    ScopedPointer<TextButton> debugButton;
    ScopedPointer<DrawableButton> plotButton;
    ScopedPointer<HeaderButton> bypassButton;
    ScopedPointer<ShapeButton> foldButton;
    ScopedPointer<ShapeButton> deleteButton;
	ScopedPointer<ShapeButton> addButton;
	ScopedPointer<ShapeButton> routeButton;
    ScopedPointer<Slider> intensitySlider;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditorHeader)
};

#endif