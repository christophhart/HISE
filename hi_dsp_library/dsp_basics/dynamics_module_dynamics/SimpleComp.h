

#ifndef __SIMPLE_COMP_H__
#define __SIMPLE_COMP_H__



namespace dynamics_module
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

}	// end namespace dynamics_module




#endif	// end __SIMPLE_COMP_H__
