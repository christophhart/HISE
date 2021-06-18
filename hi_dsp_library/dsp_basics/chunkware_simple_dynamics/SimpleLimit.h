/*
 *	Simple Limiter (header)
 *
 *  File		: SimpleLimit.h
 *	Library		: SimpleSource
 *  Version		: 1.12
 *  Class		: SimpleLimit
 *
 *	� 2006, ChunkWare Music Software, OPEN-SOURCE
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


#ifndef __SIMPLE_LIMIT_H__
#define __SIMPLE_LIMIT_H__



namespace chunkware_simple
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
		
        void process( SimpleDataType &in1, SimpleDataType &in2 );

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

}	// end namespace chunkware_simple


#endif	// end __SIMPLE_LIMIT_H__
