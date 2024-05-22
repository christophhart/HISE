
#ifndef __SIMPLE_ENVELOPE_H__
#define __SIMPLE_ENVELOPE_H__

namespace dynamics_module
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

}	// end namespace dynamics_module

#endif	// end __SIMPLE_ENVELOPE_H__
