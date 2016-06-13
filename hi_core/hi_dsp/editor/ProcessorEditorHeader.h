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
	ProcessorEditorHeader (ProcessorEditor *p);
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