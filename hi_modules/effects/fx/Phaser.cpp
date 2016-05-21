

#define SR (44100.f)  //sample rate
#define F_PI (3.14159f)

#if 0
class Phaser
{
public:
    Phaser()
    : feedback( .7f )
    , lfoPhase( 0.f )
    , depth( 1.f )
    , _zm1( 0.f )
    {
        setRange( 440.f, 1600.f );
        setRate( .5f );
    }
    
    void setRange( float fMin, float fMax ){ // Hz
        _dmin = fMin / (SR/2.f);
        _dmax = fMax / (SR/2.f);
    }
    
    void setRate( float rate ){ // cps
        lfoDelta= 2.f * F_PI * (rate / SR);
    }
    
    void setFeedback( float fb ){ // 0 -> <1.
        feedback = fb;
    }
    
    void Depth( float depth ){  // 0 -> 1.
        _depth = depth;
    }
    
    float Update( float inSamp ){
        //calculate and update phaser sweep lfo...
        float d  = _dmin + (_dmax-_dmin) * ((sin( _lfoPhase ) +
                                             1.f)/2.f);
        _lfoPhase += _lfoInc;
        if( _lfoPhase >= F_PI * 2.f )
            _lfoPhase -= F_PI * 2.f;
        
        //update filter coeffs
        for( int i=0; i<6; i++ )
            _alps[i].Delay( d );
        
        //calculate output
        float y = 	_alps[0].Update(
                                    _alps[1].Update(
                                                    _alps[2].Update(
                                                                    _alps[3].Update(
                                                                                    _alps[4].Update(
                                                                                                    _alps[5].Update( inSamp + _zm1 * _fb ))))));
        _zm1 = y;
        
        return inSamp + y * _depth;
    }
private:
    class AllpassDelay{
    public:
        AllpassDelay()
        : _a1( 0.f )
        , _zm1( 0.f )
        {}
        
        void Delay( float delay ){ //sample delay time
            _a1 = (1.f - delay) / (1.f + delay);
        }
        
        float Update( float inSamp ){
            float y = inSamp * -_a1 + _zm1;
            _zm1 = y * _a1 + inSamp;
            
            return y;
        }
    private:
        float _a1, _zm1;
    };
    
    AllpassDelay _alps[6];
    
    float rangeMin, rangeMax;
    float feedback;
    float lfoPhase;
    float lfoDelta;
    float depth;
    
    float _zm1;
};

#endif

PhaserEffect::PhaserEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id)
{
    parameterNames.add("Speed");
    parameterNames.add("Range");
    parameterNames.add("Feedback");
    parameterNames.add("Mix");
    
}

float PhaserEffect::getAttribute(int parameterIndex) const
{
    switch (parameterIndex)
    {
        case Speed:			return 0.0f;
        case Range:			return 0.0f;
        case Feedback:		return 0.0f;
        case Mix:			return 0.0f;
        default:			jassertfalse; return 1.0f;
    }
}

void PhaserEffect::setInternalAttribute(int parameterIndex, float value)
{
    switch (parameterIndex)
    {
        case Speed:			break;
        case Range:			break;
        case Feedback:		break;
        case Mix:			break;
        default:			jassertfalse; break;
    }
}


void PhaserEffect::restoreFromValueTree(const ValueTree &v)
{
    MasterEffectProcessor::restoreFromValueTree(v);
    
    loadAttribute(Speed, "Speed");
    loadAttribute(Range, "Range");
    loadAttribute(Feedback, "Feedback");
    loadAttribute(Mix, "Mix");
}

ValueTree PhaserEffect::exportAsValueTree() const
{
    ValueTree v = MasterEffectProcessor::exportAsValueTree();
    
    saveAttribute(Speed, "Speed");
    saveAttribute(Range, "Range");
    saveAttribute(Feedback, "Feedback");
    saveAttribute(Mix, "Mix");

    return v;
}

void PhaserEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
}

void PhaserEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
}

ProcessorEditorBody *PhaserEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND
    
    return new PhaserEditor(parentEditor);
    
#else 
    
    ignoreUnused(parentEditor);
    jassertfalse;
    return nullptr;
    
#endif
}


