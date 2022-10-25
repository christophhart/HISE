/*
 *	Simple Envelope Detectors (header)
 *
 *  File		: SimpleEnvelope.h
 *	Library		: SimpleSource
 *  Version		: 1.12
 *  Class		: EnvelopeDetector, AttRelEnvelope
 *
 *	© 2006, ChunkWare Music Software, OPEN-SOURCE
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *	copy of this software and associated documentation files (the "Software"),
 *	to deal in the Software without restriction, including without limitation
 *	the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *	and/or sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 */


#ifndef __SIMPLE_ENVELOPE_H__
#define __SIMPLE_ENVELOPE_H__

namespace chunkware_simple
{
	//-------------------------------------------------------------
	// DC offset (to prevent denormal)
	//-------------------------------------------------------------

	// USE:
	// 1. init envelope state to DC_OFFSET before processing
	// 2. add to input before envelope runtime function
	static const SimpleDataType DC_OFFSET = 1.0E-25;

	//-------------------------------------------------------------
	// envelope detector
	//-------------------------------------------------------------
	class EnvelopeDetector
	{
	public:
		EnvelopeDetector(
			SimpleDataType ms = 1.0
			, SimpleDataType sampleRate = 44100.0
			);
		virtual ~EnvelopeDetector() {}

		// time constant
		virtual void   setTc(SimpleDataType ms );
		virtual SimpleDataType getTc( void ) const { return ms_; }

		// sample rate
		virtual void   setSampleRate(SimpleDataType sampleRate );
		virtual SimpleDataType getSampleRate( void ) const { return sampleRate_; }

		// runtime function
		INLINE void run(SimpleDataType in, SimpleDataType &state ) {
			state = in + coef_ * ( state - in );
		}

	protected:
			
		SimpleDataType sampleRate_;		// sample rate
		SimpleDataType ms_;				// time constant in ms
		SimpleDataType coef_;			// runtime coefficient
		virtual void setCoef( void );	// coef calculation

	};	// end SimpleComp class

	//-------------------------------------------------------------
	// attack/release envelope
	//-------------------------------------------------------------
	class AttRelEnvelope
	{
	public:
		AttRelEnvelope(
			SimpleDataType att_ms = 10.0
			, SimpleDataType rel_ms = 100.0
			, SimpleDataType sampleRate = 44100.0
			);
		virtual ~AttRelEnvelope() {}

		// attack time constant
		virtual void   setAttack(SimpleDataType ms );
		virtual SimpleDataType getAttack( void ) const { return att_.getTc(); }

		// release time constant
		virtual void   setRelease(SimpleDataType ms );
		virtual SimpleDataType getRelease( void ) const { return rel_.getTc(); }

		// sample rate dependencies
		virtual void   setSampleRate(SimpleDataType sampleRate );
		virtual SimpleDataType getSampleRate( void ) const { return att_.getSampleRate(); }

		// runtime function
		void run(SimpleDataType in, SimpleDataType &state );

	private:
			
		EnvelopeDetector att_;
		EnvelopeDetector rel_;
		
	};	// end AttRelEnvelope class

}	// end namespace chunkware_simple

#endif	// end __SIMPLE_ENVELOPE_H__
