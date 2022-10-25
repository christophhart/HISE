/*
 *	Simple Gate (header)
 *
 *  File		: SimpleGate.h
 *	Library		: SimpleSource
 *  Version		: 1.12
 *  Class		: SimpleGate, SimpleGateRms
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


#ifndef __SIMPLE_GATE_H__
#define __SIMPLE_GATE_H__

namespace chunkware_simple
{
	//-------------------------------------------------------------
	// simple gate
	//-------------------------------------------------------------
	class SimpleGate : public AttRelEnvelope
	{
	public:
		SimpleGate();
		virtual ~SimpleGate() {}

		// parameters
		virtual void   setThresh( SimpleDataType dB );
		virtual SimpleDataType getThresh( void ) const { return threshdB_; }
		
		// runtime
		virtual void initRuntime( void );			// call before runtime (in resume())
		
        void process( SimpleDataType &in1, SimpleDataType &in2 );
        
        //-------------------------------------------------------------
        void process( SimpleDataType &in1, SimpleDataType &in2, SimpleDataType keyLinked );
        
		void setRatio(SimpleDataType ) {}
		SimpleDataType getRatio() const { return SimpleDataType(1); }

		SimpleDataType getGainReduction() const { return gR; }

	private:
		
		SimpleDataType gR;

		// transfer function
		SimpleDataType threshdB_;	// threshold (dB)
		SimpleDataType thresh_;		// threshold (linear)
		
		// runtime variables
		SimpleDataType env_;		// over-threshold envelope (linear)
		
	};	// end SimpleGate class

	//-------------------------------------------------------------
	// simple gate with RMS detection
	//-------------------------------------------------------------
	class SimpleGateRms : public SimpleGate
	{
	public:
		SimpleGateRms();
		virtual ~SimpleGateRms() {}

		// sample rate
		virtual void setSampleRate( SimpleDataType sampleRate );

		// RMS window
		virtual void setWindow( SimpleDataType ms );
		virtual SimpleDataType getWindow( void ) const { return ave_.getTc(); }

		// runtime process
		virtual void initRuntime( void );			// call before runtime (in resume())
		
        //-------------------------------------------------------------
        void process( SimpleDataType &in1, SimpleDataType &in2 );

	private:

		EnvelopeDetector ave_;	// averager
		SimpleDataType aveOfSqrs_;		// average of squares

	};	// end SimpleGateRms class

}	// end namespace chunkware_simple



#endif	// end __SIMPLE_GATE_H__
