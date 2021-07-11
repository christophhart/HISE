/*
 *	Simple Compressor (header)
 *
 *  File		: SimpleComp.h
 *	Library		: SimpleSource
 *  Version		: 1.12
 *  Class		: SimpleComp, SimpleCompRms
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


#ifndef __SIMPLE_COMP_H__
#define __SIMPLE_COMP_H__



namespace chunkware_simple
{
	//-------------------------------------------------------------
	// simple compressor
	//-------------------------------------------------------------
	class SimpleComp : public AttRelEnvelope
	{
	public:
		SimpleComp();
		virtual ~SimpleComp() {}

		// parameters
		virtual void setThresh( double dB );
		virtual void setRatio( double dB );

		virtual double getThresh( void ) const { return threshdB_; }
		virtual double getRatio( void )  const { return ratio_; }

		// runtime
		virtual void initRuntime( void );			// call before runtime (in resume())
		
        //-------------------------------------------------------------
        void process( SimpleDataType &in1, SimpleDataType &in2 );
        
        //-------------------------------------------------------------
        void process( SimpleDataType &in1, SimpleDataType &in2, SimpleDataType keyLinked );

        
		double getGainReduction() const { return gR; }

	private:

		double gR;

		// transfer function
		double threshdB_;		// threshold (dB)
		double ratio_;			// ratio (compression: < 1 ; expansion: > 1)

		// runtime variables
		double envdB_;			// over-threshold envelope (dB)

	};	// end SimpleComp class

	//-------------------------------------------------------------
	// simple compressor with RMS detection
	//-------------------------------------------------------------
	class SimpleCompRms : public SimpleComp
	{
	public:
		SimpleCompRms();
		virtual ~SimpleCompRms() {}

		// sample rate
		virtual void setSampleRate( double sampleRate );

		// RMS window
		virtual void setWindow( double ms );
		virtual double getWindow( void ) const { return ave_.getTc(); }

		// runtime process
		virtual void initRuntime( void );			// call before runtime (in resume())
		
        void process( SimpleDataType &in1, SimpleDataType &in2 );

	private:

		EnvelopeDetector ave_;	// averager
		double aveOfSqrs_;		// average of squares

	};	// end SimpleCompRms class

}	// end namespace chunkware_simple




#endif	// end __SIMPLE_COMP_H__
