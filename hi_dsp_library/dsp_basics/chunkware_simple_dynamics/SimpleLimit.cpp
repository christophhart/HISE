/*
 *	Simple Limiter (source)
 *
 *  File		: SimpleLimit.cpp
 *	Library		: SimpleSource
 *  Version		: 1.12
 *  Implements	: SimpleLimit
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


#include "SimpleLimit.h"

 // include inlined process function
#include "SimpleLimitProcess.inl"

namespace chunkware_simple
{
	//-------------------------------------------------------------
	SimpleLimit::SimpleLimit()
		: threshdB_( 0.0 )
		, thresh_( 1.0 )
		, peakHold_( 0 )
		, peakTimer_( 0 )
		, maxPeak_( 1.0 )
		, att_( 1.0 )
		, rel_( 10.0 )
		, env_( 1.0 )
		, mask_( BUFFER_SIZE - 1 )
		, cur_( 0 )
	{
		setAttack( 1.0 );
		outBuffer_[ 0 ].resize( BUFFER_SIZE, 0.0 );
		outBuffer_[ 1 ].resize( BUFFER_SIZE, 0.0 );
	}

	//-------------------------------------------------------------
	void SimpleLimit::setThresh( SimpleDataType dB )
	{
		threshdB_ = dB;
		thresh_ = dB2lin( dB );
	}

	//-------------------------------------------------------------
	void SimpleLimit::setAttack( SimpleDataType ms )
	{
		ms = jmax<SimpleDataType>(SimpleDataType(0.02), ms);

		unsigned int samp = jmin<int>(BUFFER_SIZE-1, int( 0.001 * ms * att_.getSampleRate() ));

		assert( samp < BUFFER_SIZE );

		peakHold_ = samp;
		att_.setTc( ms );
	}

	//-------------------------------------------------------------
	void SimpleLimit::setRelease( SimpleDataType ms )
	{
		rel_.setTc( ms );
	}

	//-------------------------------------------------------------
	void SimpleLimit::setSampleRate( SimpleDataType sampleRate )
	{
		att_.setSampleRate( sampleRate );
		rel_.setSampleRate( sampleRate );
	}

	//-------------------------------------------------------------
	void SimpleLimit::initRuntime( void )
	{
		peakTimer_ = 0;
		maxPeak_ = thresh_;
		env_ = thresh_;
		cur_ = 0;
		outBuffer_[ 0 ].assign( BUFFER_SIZE, 0.0 );
		outBuffer_[ 1 ].assign( BUFFER_SIZE, 0.0 );
	}

	void SimpleLimit::process(SimpleDataType &in1, SimpleDataType &in2)
	{
		// create sidechain

		SimpleDataType rect1 = fabs(in1);	// rectify input
		SimpleDataType rect2 = fabs(in2);

		SimpleDataType keyLink = jmax(rect1, rect2);	// link channels with greater of 2

		// threshold
		// we always want to feed the sidechain AT LEATS the threshold value
		if (keyLink < thresh_)
			keyLink = thresh_;

		// test:
		// a) whether peak timer has "expired"
		// b) whether new peak is greater than previous max peak
		if ((++peakTimer_ >= peakHold_) || (keyLink > maxPeak_)) {
			// if either condition is met:
			peakTimer_ = 0;		// reset peak timer
			maxPeak_ = keyLink;	// assign new peak to max peak
		}

		/* REGARDING THE MAX PEAK: This method assumes that the only important
		 * sample in a look-ahead buffer would be the highest peak. As such,
		 * instead of storing all samples in a look-ahead buffer, it only stores
		 * the max peak, and compares all incoming samples to that one.
		 * The max peak has a hold time equal to what the look-ahead buffer
		 * would have been, which is tracked by a timer (counter). When this
		 * timer expires, the sample would have exited from the buffer. Therefore,
		 * a new sample must be assigned to the max peak. We assume that the next
		 * highest sample in our theoretical buffer is the current input sample.
		 * In reality, we know this is probably NOT the case, and that there has
		 * been another sample, slightly lower than the one before it, that has
		 * passed the input. If we do not account for this possibility, our gain
		 * reduction could be insufficient, resulting in an "over" at the output.
		 * To remedy this, we simply apply a suitably long release stage in the
		 * envelope follower.
		 */

		 // attack/release
		if (maxPeak_ > env_)
			att_.run(maxPeak_, env_);		// run attack phase
		else
			rel_.run(maxPeak_, env_);		// run release phase

		/* REGARDING THE ATTACK: This limiter achieves "look-ahead" detection
		 * by allowing the envelope follower to attack the max peak, which is
		 * held for the duration of the attack phase -- unless a new, higher
		 * peak is detected. The output signal is buffered so that the gain
		 * reduction is applied in advance of the "offending" sample.
		 */

		 /* NOTE: a DC offset is not necessary for the envelope follower,
		  * as neither the max peak nor envelope should fall below the
		  * threshold (which is assumed to be around 1.0 linear).
		  */

		  // gain reduction
		gR = thresh_ / env_;

		// unload current buffer index
		// ( cur_ - delay ) & mask_ gets sample from [delay] samples ago
		// mask_ variable wraps index
		unsigned int delayIndex = (cur_ - peakHold_) & mask_;
		SimpleDataType delay1 = outBuffer_[0][delayIndex];
		SimpleDataType delay2 = outBuffer_[1][delayIndex];

		// load current buffer index and advance current index
		// mask_ wraps cur_ index
		outBuffer_[0][cur_] = in1;
		outBuffer_[1][cur_] = in2;
		++cur_ &= mask_;

		// output gain
		in1 = delay1 * gR;	// apply gain reduction to input
		in2 = delay2 * gR;

		/* REGARDING THE GAIN REDUCTION: Due to the logarithmic nature
		 * of the attack phase, the sidechain will never achieve "full"
		 * attack. (Actually, it is only guaranteed to achieve 99% of
		 * the input value over the given time constant.) As such, the
		 * limiter cannot achieve "brick-wall" limiting. There are 2
		 * workarounds:
		 *
		 * 1) Set the threshold slightly lower than the desired threshold.
		 *		i.e. 0.0dB -> -0.1dB or even -0.5dB
		 *
		 * 2) Clip the output at the threshold, as such:
		 *
		 *		if ( in1 > thresh_ )		in1 = thresh_;
		 *		else if ( in1 < -thresh_ )	in1 = -thresh_;
		 *
		 *		if ( in2 > thresh_ )		in2 = thresh_;
		 *		else if ( in2 < -thresh_ )	in2 = -thresh_;
		 *
		 *		(... or replace with your favorite branchless clipper ...)
		 */
	}

	//-------------------------------------------------------------
	void SimpleLimit::FastEnvelope::setCoef( void )
	{
		// rises to 99% of in value over duration of time constant
		coef_ = pow( 0.01, (1000.0 / (ms_ * sampleRate_) ) );
	}

}	// end namespace chunkware_simple
