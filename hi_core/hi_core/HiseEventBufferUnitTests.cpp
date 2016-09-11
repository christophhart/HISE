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

#include "JuceHeader.h"

class HiseEventUnitTest : public UnitTest
{
public:

	HiseEventUnitTest() :
		UnitTest("Testing Hise Event structure")
	{

	}

	void runTest() override
	{
		testConstructors();

		testNoteOn();

		testPitchWheel();

		testEventBuffer();

		testEventBufferCopyMethods();

		testMidiBufferCopyMethods();

		testEventBufferMoveOperations();
	}

	

private:

	void testConstructors()
	{
		beginTest("Testing empty constructor");

		HiseEvent empty;

		expect(empty.isNoteOn(), "Correct type (NoteOn)");
		expectEquals<int>(empty.getNoteNumber(), 0, "Number value");
		expectEquals<int>(empty.getVelocity(),0, "Velocity");
		expectEquals<int>(empty.getChannel(), 0, "Channel");
		expect(!empty.isArtificial(), "Artificial state");
		expect(!empty.isIgnored(), "Ignored state");
		expectEquals<int>(empty.getGain(), 0, "Gain");
		expectEquals<int>(empty.getCoarseDetune(), 0, "Coarse Detune");
		expectEquals<int>(empty.getFineDetune(), 0, "Fine Detune");

		expect(empty.getEventId() == 0, "Event ID");
		expect(empty.getTransposeAmount() == 0, "Transpose Amount");
		expect(empty.getPitchFactorForEvent() == 1.0, "Pitch Factor");
		
		expectEquals<int>(sizeof(HiseEvent), 16, "Byte size of HiseEvent structure");

		beginTest("Testing initialisation with zeroes.");

		HiseEvent empty2(empty);

		expect(empty2 == empty, "Copy constructor & Equal Operator");

		HiseEvent::clear(&empty);

		expect(empty.isNoteOn(), "Correct type (NoteOn)");
		expectEquals<int>(empty.getNoteNumber(), 0, "Number value");
		expectEquals<int>(empty.getVelocity(), 0, "Velocity");
		expectEquals<int>(empty.getChannel(), 0, "Channel");
		expect(!empty.isArtificial(), "Artificial state");
		expect(!empty.isIgnored(), "Ignored state");
		expectEquals<int>(empty.getGain(), 0, "Gain");
		expectEquals<int>(empty.getCoarseDetune(), 0, "Coarse Detune");
		expectEquals<int>(empty.getFineDetune(), 0, "Fine Detune");

		expect(empty.getEventId() == 0, "Event ID");
		expect(empty.getTransposeAmount() == 0, "Transpose Amount");
		expect(empty.getPitchFactorForEvent() == 1.0, "Pitch Factor");
	}

	void testNoteOn()
	{
		beginTest("Testing constructor with note on message");

		const uint8 noteNumber = (uint8)r.nextInt(128);
		const uint8 velocity = (uint8)r.nextInt(128);
		const uint8 channel = r.nextInt(Range<int>(1, 17));

		MidiMessage no = MidiMessage::noteOn(channel, noteNumber, (uint8)velocity);

		HiseEvent noe = HiseEvent(no);

		expect(noe.isNoteOn(), "Type (Note on)");
		expectEquals<int>(noe.getNoteNumber(), noteNumber, "Note Number");
		expectEquals<int>(noe.getVelocity(), velocity, "Velocity");
		expectEquals<int>(noe.getChannel(), channel, "Channel");
		expect(!noe.isArtificial(), "Artificial state");
		expect(!noe.isIgnored(), "Ignored state");
		expectEquals<int>(noe.getGain(), 0, "Gain");
		expectEquals<int>(noe.getCoarseDetune(), 0, "Coarse Detune");
		expectEquals<int>(noe.getFineDetune(), 0, "Fine Detune");

		const int timeStamp = r.nextInt(4096);

		noe.setTimeStamp(timeStamp);

		expectEquals<int>(noe.getTimeStamp(), timeStamp, "Timestamp");

		expect(noe.getEventId() == 0, "Event ID");
		expect(noe.getTransposeAmount() == 0, "Transpose Amount");
		expect(noe.getPitchFactorForEvent() == 1.0, "Pitch Factor");

		HiseEvent noe2(noe);

		expect(noe == noe2, "Copy constructor of initialised event");

		expect(noe2.isNoteOn(), "Type (Note on)");
		expectEquals<int>(noe2.getNoteNumber(), noteNumber, "Note Number");
		expectEquals<int>(noe2.getVelocity(), velocity, "Velocity");
		expectEquals<int>(noe2.getChannel(), channel, "Channel");
		expect(!noe2.isArtificial(), "Artificial state");
		expect(!noe2.isIgnored(), "Ignored state");
		expectEquals<int>(noe2.getGain(), 0, "Gain");
		expectEquals<int>(noe2.getCoarseDetune(), 0, "Coarse Detune");
		expectEquals<int>(noe2.getFineDetune(), 0, "Fine Detune");
		expectEquals<int>(noe2.getTimeStamp(), timeStamp, "Timestamp of copied message");
	}

	void testPitchWheel()
	{

		beginTest("Testing pitch wheel messages");

		HiseEvent pitch(HiseEvent::Type::PitchBend, 0, 0, 1);

		const int pitchWheelValue = r.nextInt(16384);

		pitch.setPitchWheelValue(pitchWheelValue);

		expectEquals<int>(pitch.getPitchWheelValue(), pitchWheelValue, "Pitch Value");

		HiseEvent pitch2(pitch);

		expectEquals<int>(pitch.getPitchWheelValue(), pitch2.getPitchWheelValue(), "Pitch Value of copied message");

		expect(pitch.isPitchWheel(), "Correct Type");
		expect(!pitch.isController(), "Not a controller");
	}

	void testEventBuffer()
	{
		beginTest("Testing HiseEventBuffer");

		HiseEventBuffer b;

		expect(b.isEmpty(), "isEmpty()");
		expectEquals<int>(b.getNumUsed(), 0, "getNumUsed()");

		const int numToFill = r.nextInt(HISE_EVENT_BUFFER_SIZE);

		for (int i = 0; i < numToFill; i++)
		{
			b.addEvent(generateRandomHiseEvent());
		}

		expectEquals<int>(b.getNumUsed(), numToFill, "Correct size after filling buffer");

		HiseEventBuffer::Iterator iter(b);

		int index = 0;

		int timestamp = 0;


		while (HiseEvent* e = iter.getNextEventPointer(false))
		{
			index++;

			const int thisTimestamp = e->getTimeStamp();

			expect(thisTimestamp >= timestamp, "Testing correct sorted order");

		}

		expectEquals<int>(index, numToFill, "Iterator index");


		b.clear();

		expectEquals<int>(b.getNumUsed(), 0, "cleared buffer");

		const int equalTimestamp = r.nextInt(16384);

		HiseEvent firstEvent(generateRandomHiseEvent());
		HiseEvent secondEvent(generateRandomHiseEvent());

		firstEvent.setTimeStamp(equalTimestamp);
		secondEvent.setTimeStamp(equalTimestamp);

		b.addEvent(firstEvent);
		b.addEvent(secondEvent);

		HiseEventBuffer::Iterator iter2(b);

		expect(*iter2.getNextEventPointer(false) == firstEvent);
		expect(*iter2.getNextEventPointer(false) == secondEvent, "Correct order of insertion");

	}

	void testEventBufferCopyMethods()
	{
		beginTest("Testing HiseEventBuffer copy methods");

		HiseEventBuffer b1;
		HiseEventBuffer b2;

		const int numToFill = r.nextInt(HISE_EVENT_BUFFER_SIZE);

		for (int i = 0; i < numToFill; i++)
		{
			b1.addEvent(generateRandomHiseEvent());
		}

		b2.copyFrom(b1);

		expectEquals(b1.getNumUsed(), b2.getNumUsed(), "buffer sizes");


		HiseEventBuffer::Iterator iter1(b1);
		HiseEventBuffer::Iterator iter2(b2);

		while (HiseEvent* e1 = iter1.getNextEventPointer())
		{
			HiseEvent* e2 = iter2.getNextEventPointer();

			expect(*e1 == *e2, "Equal Events");
		}

		expect(b1 == b2, "Equal Operator");

	}

	void testMidiBufferCopyMethods()
	{
		beginTest("Testing MidiBuffer copy operations");

		HiseEventBuffer b1;
		HiseEventBuffer b2;

		MidiBuffer mb;

		const int numToFill = r.nextInt(HISE_EVENT_BUFFER_SIZE);

		for (int i = 0; i < numToFill; i++)
		{
			mb.addEvent(generateRandomMidiMessage(), r.nextInt(4096));
		}

		b1.addEvents(mb);

		MidiBuffer::Iterator mbIterator(mb);

		MidiMessage m;
		int samplePos;

		while (mbIterator.getNextEvent(m, samplePos))
		{
			b2.addEvent(m, samplePos);
		}
		
		expectEquals<int>(b1.getNumUsed(), b2.getNumUsed(), "Size");
		expectEquals<int>(b1.getNumUsed(), numToFill, "Size (numToFill)");

		HiseEventBuffer::Iterator b1iter(b1);
		HiseEventBuffer::Iterator b2iter(b2);

		int index = 0;

		while (HiseEvent* e1 = b1iter.getNextEventPointer(false))
		{
			HiseEvent* e2 = b2iter.getNextEventPointer(false);

			if (e2 != nullptr)
			{
				index++;
				expect(*e1 == *e2, "Checking equality of events");
			}
		}

		expectEquals(index, numToFill, "Checked all events");


	}
	
	

	void testEventBufferMoveOperations()
	{
		beginTest("Testing HiseEventBuffer moveEventsAbove method");

		HiseEventBuffer b1;
		HiseEventBuffer b2;
		HiseEventBuffer b3;


		const int numToFill = r.nextInt(HISE_EVENT_BUFFER_SIZE);

		for (int i = 0; i < numToFill; i++)
		{
			b1.addEvent(generateRandomHiseEvent());
		}

		b3.copyFrom(b1);

		b1.moveEventsAbove(b2, 512);

		HiseEventBuffer::Iterator iter1(b1);

		int index = 0;

		while (HiseEvent* e = iter1.getNextEventPointer())
		{
			expect(e->getTimeStamp() < 512);
			index++;
		}

		HiseEventBuffer::Iterator iter2(b2);

		while (HiseEvent* e = iter2.getNextEventPointer())
		{
			expect(e->getTimeStamp() >= 512);
			index++;
		}

		expectEquals<int>(index, numToFill, "All events tested");
		expectEquals<int>(numToFill, b1.getNumUsed() + b2.getNumUsed(), "buffer sizes");

		b1.moveEventsBelow(b2, 512);

		expectEquals<int>(b1.getNumUsed(), 0, "First buffer is empty");
		expectEquals<int>(b2.getNumUsed(), numToFill, "Second buffer contains all items");

		expect(b2 == b3, "Testing both methods");


	}

	MidiMessage generateRandomMidiMessage()
	{
		const int type = r.nextInt(5);

		switch (type)
		{
		case 0: return MidiMessage::noteOn(r.nextInt(Range<int>(1, 16)), r.nextInt(128), (uint8)r.nextInt(128));
		case 1: return MidiMessage::noteOff(r.nextInt(Range<int>(1, 16)), r.nextInt(128));
		case 2: return MidiMessage::noteOn(r.nextInt(Range<int>(1, 16)), r.nextInt(128), (uint8)r.nextInt(128));
		case 3: return MidiMessage::controllerEvent(r.nextInt(Range<int>(1, 16)), r.nextInt(128), (uint8)r.nextInt(128));
		default: return MidiMessage::pitchWheel(r.nextInt(Range<int>(1, 16)), r.nextInt(16384));
		}
	}

	HiseEvent generateRandomHiseEvent()
	{
		HiseEvent e((HiseEvent::Type)r.nextInt((int)HiseEvent::Type::numTypes),
			r.nextInt(128),
			r.nextInt(128),
			r.nextInt(256));

		e.setTimeStamp(r.nextInt(1024));
		e.setEventId(r.nextInt(UINT16_MAX));

		if(r.nextBool()) e.setArtificial();
		e.ignoreEvent(r.nextBool());

		e.setGain(r.nextInt(Range<int>(-100, 18)));
		e.setCoarseDetune(r.nextInt(Range<int>(-24, 24)));
		e.setFineDetune(r.nextInt(Range<int>(-100, 100)));

		e.setTransposeAmount(r.nextInt(Range<int>(-24, 24)));

		return e;
	}

	Random r;
};

static HiseEventUnitTest eventBufferTestInstance;