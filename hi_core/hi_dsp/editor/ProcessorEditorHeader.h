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

#ifndef PROCESSOREDITORHEADER_H_INCLUDED
#define PROCESSOREDITORHEADER_H_INCLUDED
#include "hi_core/hi_dsp/Processor.h"

namespace hise { using namespace juce;

class VuMeter;



//	===================================================================================================
/** A ProcessorEditorHeader is a bar that resides on top of every ProcessorEditor.
*	@ingroup dspEditor
*
*/
class ProcessorEditorHeader  : public ProcessorEditorChildComponent,
                               public SliderListener,
							   public LabelListener,
							   public Processor::BypassListener,
                               public ButtonListener,
							   public GlobalScriptCompileListener,
							   public Timer
{
public:

    //==============================================================================
	ProcessorEditorHeader (ProcessorEditor *p);
    ~ProcessorEditorHeader();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void update(bool force);
	void createProcessorFromPopup(Processor *insertBeforeSibling=nullptr);

    void updateIdAndColour(dispatch::library::Processor* p);

    void bypassStateChanged(Processor* p, bool state) override;

    void timerCallback() override;

	void refreshShapeButton(ShapeButton *b);

	void labelTextChanged(Label *l) override;;
    
	void scriptWasCompiled(JavascriptProcessor *processor) override;

	/** This sets the ProcessorEditorLookAndFeel for different ProcessorEditorTypes.
	*
	*	The type is detected automatically
	*/
	void setLookAndFeel();
	
	void displayFoldState(bool shouldBeFolded);;

	void displayBypassedChain(bool isBypassed);;

	/** If a header of a Chain, this disables the header (and only enables the add button) if the chain is empty. */
	void enableChainHeader();

	bool isHeaderOfModulator() const;

	bool isHeaderOfModulatorSynth() const;

	bool isHeaderOfMidiProcessor() const;
	
	bool isHeaderOfEffectProcessor() const;

	bool hasWorkspaceButton() const;

    static void updateModulationMode(ProcessorEditorHeader& h, int m);
    
	/** Returns the Modulation::Mode of the ModulatorChain. */
	int getModulatorMode() const;
	
	/** Checks if the header is a header of a EditorChain subclasse. */
	bool isHeaderOfChain() const;

	/** returns false if the header is a chain editor and the chain is empty. */
	bool isHeaderOfEmptyChain() const;

    void paint (Graphics& g) override;

    void paintOverChildren(Graphics& g) override;

    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;

	void sliderDragStarted(Slider* s) override;
	void sliderDragEnded(Slider* s) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

	void updateBipolarIcon(bool shouldBeBipolar);

	void updateRetriggerIcon(bool shouldBeBipolar);

	void updateMonoIcon(bool shouldBeMono);

	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent&) override;

	void setupButton(DrawableButton *b, ButtonShapes::Symbol s);

private:

	dispatch::library::ProcessorHandler::NameAndColourListener idAndNameListener;

	void addProcessor(Processor *processorToBeAdded, Processor *insertBeforeSibling);

	void setPlotButton(bool on);

	void checkFoldButton();

	void checkSoloLabel();

	bool isChainHeader;
	bool isPitchMode;

	BiPolarSliderLookAndFeel pitchModeLookAndFeel;

	AlertWindowLookAndFeel alertLaf;

	bool storedInPreset = false;
	bool isBypassedFlag;
	bool valueFlag;
	bool intensityFlag;
	bool isSoloHeader;
	bool bipolar = false;
	bool mono = false;
	bool retrigger = false;

	String parentName;
	
	double dragStartValue = 0.0;

	ScopedPointer<ProcessorEditorHeaderLookAndFeel> luf;
	PopupLookAndFeel plaf;
	VUSliderLookAndFeel vulaf;
	BalanceButtonLookAndFeel bbluf;
	BiPolarSliderLookAndFeel bpslaf;
	WeakReference<Processor> parentProcessor;

    //==============================================================================

	ScopedPointer<Slider> balanceSlider;

	ScopedPointer<ChainIcon> chainIcon;

    ScopedPointer<VuMeter> valueMeter;
    ScopedPointer<Label> idLabel;
    ScopedPointer<Label> typeLabel;
    ScopedPointer<HeaderButton> bypassButton;
    ScopedPointer<ShapeButton> foldButton;
    ScopedPointer<ShapeButton> deleteButton;
	ScopedPointer<ShapeButton> addButton;
	ScopedPointer<ShapeButton> workspaceButton;
	ScopedPointer<ShapeButton> monophonicButton;
	ScopedPointer<ShapeButton> retriggerButton;
    ScopedPointer<IntensitySlider> intensitySlider;
	ScopedPointer<ShapeButton> bipolarModButton;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditorHeader)
    JUCE_DECLARE_WEAK_REFERENCEABLE(ProcessorEditorHeader);
};

} // namespace hise

#endif
