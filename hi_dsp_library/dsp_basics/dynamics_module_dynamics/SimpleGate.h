
#ifndef __SIMPLE_GATE_H__
#define __SIMPLE_GATE_H__

namespace dynamics_module
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

}	// end namespace dynamics_module



#endif	// end __SIMPLE_GATE_H__
