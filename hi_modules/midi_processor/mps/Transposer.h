/*
  ==============================================================================

    Transposer.h
    Created: 5 Jul 2014 6:01:57pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef TRANSPOSER_H_INCLUDED
#define TRANSPOSER_H_INCLUDED

/** Transposes all midi note messages by the specified amount.
*	@ingroup midiTypes
*
*	If the amount is changed, a all note off message is sent to prevent hanging notes.
*/
class Transposer: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("Transposer", "Transposer");

	Transposer(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		transposeAmount(36)
	{};

	enum SpecialParameters
	{
		TransposeAmount = 0
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MidiProcessor::exportAsValueTree(); 

		saveAttribute(TransposeAmount , "TransposeAmount");
		
		return v;
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		MidiProcessor::restoreFromValueTree(v);
    
		loadAttribute(TransposeAmount , "TransposeAmount");
	}

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	

	float getAttribute(int) const
	{
		return (float)transposeAmount;
	};

	void setInternalAttribute(int, float newAmount) override
	{
		transposeAmount = (int)newAmount;
	};

	void processMidiMessage(MidiMessage &m) noexcept override
	{
		if(m.isNoteOnOrOff())
		{
			const int noteNumber = m.getNoteNumber();
			m.setNoteNumber(noteNumber + transposeAmount);
		}
	}

private:

	int transposeAmount;

};


#endif  // TRANSPOSER_H_INCLUDED
