
#ifndef __SIMPLE_LIMIT_H__
#define __SIMPLE_LIMIT_H__



namespace dynamics_module
{
	//-------------------------------------------------------------
	// simple limiter
	//-------------------------------------------------------------
	class SimpleLimit
	{
	public:
		SimpleLimit();
		virtual ~SimpleLimit() {}

		// parameters
		virtual void setThresh( SimpleDataType dB );
		virtual void setAttack( SimpleDataType ms );
		virtual void setRelease( SimpleDataType ms );

		virtual SimpleDataType getThresh( void )  const { return threshdB_; }
		virtual SimpleDataType getAttack( void )  const { return att_.getTc(); }
		virtual SimpleDataType getRelease( void ) const { return rel_.getTc(); }

		// latency
		virtual const unsigned int getLatency( void ) const { return peakHold_; }

		// sample rate dependencies
		virtual void   setSampleRate( SimpleDataType sampleRate );
		virtual SimpleDataType getSampleRate( void ) { return att_.getSampleRate(); }
		
		// runtime
		virtual void initRuntime( void );			// call before runtime (in resume())
		
        void process( SimpleDataType& in1, SimpleDataType& in2);
        
        void process( SimpleDataType& in1, SimpleDataType& in2, SimpleDataType keyIn);
        
		SimpleDataType getGainReduction() const { return gR; }

		void setRatio(SimpleDataType) {}

	protected:

		// class for faster attack/release
		class FastEnvelope : public EnvelopeDetector
		{
		public:
			FastEnvelope( SimpleDataType ms = 1.0, SimpleDataType sampleRate = 44100.0 )
				: EnvelopeDetector( ms, sampleRate )
			{}
			virtual ~FastEnvelope() {}

		protected:
			// override setCoef() - coefficient calculation
			virtual void setCoef( void );
		};
		
	private:
		
		// transfer function
		SimpleDataType threshdB_;	// threshold (dB)
		SimpleDataType thresh_;		// threshold (linear)

		// max peak
		unsigned int peakHold_;		// peak hold (samples)
		unsigned int peakTimer_;	// peak hold timer
		SimpleDataType maxPeak_;			// max peak

		SimpleDataType gR;

		// attack/release envelope
		FastEnvelope att_;			// attack
		FastEnvelope rel_;			// release
		SimpleDataType env_;				// over-threshold envelope (linear)

		// buffer
		// BUFFER_SIZE default can handle up to ~10ms at 96kHz
		// change this if you require more
		static const int BUFFER_SIZE = 4096;	// buffer size (always a power of 2!)
		unsigned int mask_;						// buffer mask
		unsigned int cur_;						// cursor
		std::vector< SimpleDataType > outBuffer_[ 2 ];	// output buffer
		
	};	// end SimpleLimit class

}	// end namespace dynamics_module


#endif	// end __SIMPLE_LIMIT_H__
