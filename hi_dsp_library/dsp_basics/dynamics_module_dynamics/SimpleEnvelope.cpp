
namespace dynamics_module
{
	//-------------------------------------------------------------
	// envelope detector
	//-------------------------------------------------------------
	EnvelopeDetector::EnvelopeDetector(SimpleDataType ms, SimpleDataType sampleRate )
	{
		assert( sampleRate > 0.0 );
		assert( ms > 0.0 );
		sampleRate_ = sampleRate;
		ms_ = ms;
		setCoef();
	}

	//-------------------------------------------------------------
	void EnvelopeDetector::setTc(SimpleDataType ms )
	{
		ms = jmax<SimpleDataType>(ms, (SimpleDataType)0.01);

		assert( ms > 0.0 );
		ms_ = ms;
		setCoef();
	}

	//-------------------------------------------------------------
	void EnvelopeDetector::setSampleRate(SimpleDataType sampleRate )
	{
		assert( sampleRate > 0.0 );
		sampleRate_ = sampleRate;
		setCoef();
	}

	//-------------------------------------------------------------
	void EnvelopeDetector::setCoef( void )
	{
		coef_ = exp( -1000.0 / ( ms_ * sampleRate_ ) );
	}

	//-------------------------------------------------------------
	// attack/release envelope
	//-------------------------------------------------------------
	AttRelEnvelope::AttRelEnvelope( SimpleDataType att_ms, SimpleDataType rel_ms, SimpleDataType sampleRate )
		: att_( att_ms, sampleRate )
		, rel_( rel_ms, sampleRate )
	{
	}

	//-------------------------------------------------------------
	void AttRelEnvelope::setAttack( SimpleDataType ms )
	{
		att_.setTc( ms );
	}

	//-------------------------------------------------------------
	void AttRelEnvelope::setRelease( SimpleDataType ms )
	{
		rel_.setTc( ms );
	}

	//-------------------------------------------------------------
	void AttRelEnvelope::setSampleRate( SimpleDataType sampleRate )
	{
		att_.setSampleRate( sampleRate );
		rel_.setSampleRate( sampleRate );
	}

	void AttRelEnvelope::run(SimpleDataType in, SimpleDataType &state)
	{
		/* assumes that:
		* positive delta = attack
		* negative delta = release
		* good for linear & log values
		*/

		if (in > state)
			att_.run(in, state);	// attack
		else
			rel_.run(in, state);	// release
	}

}	// end namespace dynamics_module
