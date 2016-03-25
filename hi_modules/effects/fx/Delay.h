/*
  ==============================================================================

    Delay.h
    Created: 8 Aug 2014 11:33:54pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

#define DELAY_BUFFER_SIZE 65536
#define DELAY_BUFFER_MASK 65536-1

class DelayLine
{
public:
    
    DelayLine():
    readIndex(0),
    writeIndex(0),
    sampleRate(-1.0),
    currentDelayTime(0.0),
    fadeCounter(-1),
    fadeTimeSamples(1024)
    {
        FloatVectorOperations::clear(delayBuffer, DELAY_BUFFER_SIZE);
    }
    
    void prepareToPlay(double sampleRate_)
    {
        ScopedLock sl(lock);
        
        sampleRate = sampleRate_;
    }
    
    void setDelayTimeSeconds(double delayInSeconds)
    {
        setDelayTimeSamples((int)(delayInSeconds * sampleRate));
    }
    
    void setDelayTimeSamples(int delayInSamples)
    {
        ScopedLock sl(lock);
     
        delayInSamples = jmin<int>(delayInSamples, DELAY_BUFFER_SIZE - 1);
        
        if(fadeCounter != -1)
        {
            lastIgnoredDelayTime = delayInSamples;
            return;
        }
        else
        {
            lastIgnoredDelayTime = 0;
        }
        
        currentDelayTime = delayInSamples;
        
        oldReadIndex = readIndex;
        
        fadeCounter = 0;
        
        readIndex = (writeIndex - delayInSamples) & DELAY_BUFFER_MASK;
    }
    
    void setFadeTimeSamples(int newFadeTimeInSamples)
    {
        fadeTimeSamples = newFadeTimeInSamples;
    }
    
    float getDelayedValue(float inputValue)
    {
        
        delayBuffer[writeIndex++] = inputValue;
        
        if(fadeCounter < 0)
        {
            const float returnValue = delayBuffer[readIndex++];
            
            readIndex &= DELAY_BUFFER_MASK;
            writeIndex &= DELAY_BUFFER_MASK;
            return returnValue;
        }
        else
        {
            const float oldValue = delayBuffer[oldReadIndex++];
            const float newValue = delayBuffer[readIndex++];
            
            const float mix = (float)fadeCounter / (float)fadeTimeSamples;
            
            const float returnValue = newValue * mix + oldValue * (1.0f-mix);
            
            oldReadIndex &= DELAY_BUFFER_MASK;
            readIndex &= DELAY_BUFFER_MASK;
            writeIndex &= DELAY_BUFFER_MASK;
            
            fadeCounter++;
            
            if(fadeCounter >= fadeTimeSamples)
            {
                fadeCounter = -1;
                if(lastIgnoredDelayTime != 0)
                {
                    setDelayTimeSamples(lastIgnoredDelayTime);
                }
            }
            
            return returnValue;
        }
    }
    
private:
    
    CriticalSection lock;
    
    double maxDelayTime;
    int currentDelayTime;
    double sampleRate;
    
    int lastIgnoredDelayTime;
    
    float delayBuffer[DELAY_BUFFER_SIZE];
    
    int readIndex;
    int oldReadIndex;
    int writeIndex;
    
    int fadeCounter;
    
    int fadeTimeSamples;
};


/** A delay effect with time synching and dual channel processing.
*	@ingroup effectTypes
*/
class DelayEffect: public MasterEffectProcessor,
				   public TempoListener
{
public:

	SET_PROCESSOR_NAME("Delay", "Delay")

	/** The parameters*/
	enum Parameters
	{
		DelayTimeLeft = 0, ///< the left channel time in ms or TempoSync::Mode
		DelayTimeRight, ///< the right channel time in ms or TempoSync::Mode
		FeedbackLeft, ///< the left channel feedback in percent
		FeedbackRight, ///< the right channel feedback in percent
		LowPassFreq, ///< the frequency for the low pass
		HiPassFreq, ///< the frequency for the high pass
		Mix, ///< the wet amount
		TempoSync, ///< if enabled, the delay time will sync to the host tempo
		numEffectParameters
	};

	DelayEffect(MainController *mc, const String &id):
		MasterEffectProcessor(mc, id),
		delayTimeLeft(300.0f),
		delayTimeRight(250.0f),
		feedbackLeft(0.3f),
		feedbackRight(0.3f),
		lowPassFreq(20000.0f),
		hiPassFreq(40.0f),
		mix(0.5f),
		tempoSync(true),
		syncTimeLeft(TempoSyncer::QuarterTriplet),
		syncTimeRight(TempoSyncer::Quarter),
		skipFirstBuffer(true)
	{
		parameterNames.add("DelayTimeLeft");
		parameterNames.add("DelayTimeRight");
		parameterNames.add("FeedbackLeft");
		parameterNames.add("FeedbackRight");
		parameterNames.add("LowPassFreq");
		parameterNames.add("HiPassFreq");
		parameterNames.add("Mix");
		parameterNames.add("TempoSync");

		mc->addTempoListener(this);

		enableConsoleOutput(true);
	};

	~DelayEffect()
	{
		getMainController()->removeTempoListener(this);
	}

	void tempoChanged(double newTempo) override
	{
		if(tempoSync)
		{
			delayTimeLeft = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeLeft);
			delayTimeRight = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeLeft);

			calcDelayTimes();

		}
	}

	float getAttribute(int parameterIndex) const override
	{
		switch ( parameterIndex )
		{
		case DelayTimeLeft:		return tempoSync ? (float)syncTimeLeft :  delayTimeLeft;
		case DelayTimeRight:	return tempoSync ? (float)syncTimeRight : delayTimeRight;
		case FeedbackLeft:		return feedbackLeft;
		case FeedbackRight:		return feedbackRight;
		case LowPassFreq:		return lowPassFreq;
		case HiPassFreq:		return hiPassFreq;
		case Mix:				return mix;
		case TempoSync:			return tempoSync ? 1.0f : 0.0f;
		default:				jassertfalse; return 0.0f;
		}
	};

	void setInternalAttribute(int parameterIndex, float newValue) override 
	{
		switch ( parameterIndex )
		{
		case DelayTimeLeft:		if(tempoSync)
								{
									syncTimeLeft = (TempoSyncer::Tempo)(int)newValue;
								}
								else
								{
									delayTimeLeft = newValue;
								}
								calcDelayTimes();
								break;

		case DelayTimeRight:	if(tempoSync)
								{
									syncTimeRight = (TempoSyncer::Tempo)(int)newValue;
								}
								else
								{
									delayTimeRight = newValue;
								}
								calcDelayTimes();
								break;
        case FeedbackLeft:		feedbackLeft = newValue;
                                break;
        case FeedbackRight:		feedbackRight = newValue;
                                break;
		case LowPassFreq:		lowPassFreq = newValue; break;
		case HiPassFreq:		hiPassFreq = newValue; break;
		case Mix:				mix = newValue; break;
		case TempoSync:			tempoSync = (newValue == 1.0f); 
								
								calcDelayTimes();

								break;
		default:				jassertfalse; 
		}
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);

		loadAttribute(TempoSync, "TempoSync");
		loadAttribute(DelayTimeLeft, "DelayTimeLeft");
		loadAttribute(DelayTimeRight, "DelayTimeRight");
		loadAttribute(FeedbackLeft, "FeedbackLeft");
		loadAttribute(FeedbackRight, "FeedbackRight");
		loadAttribute(LowPassFreq, "LowPassFreq");
		loadAttribute(HiPassFreq, "HiPassFreq");
		loadAttribute(Mix, "Mix");
		
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();

		saveAttribute(DelayTimeLeft, "DelayTimeLeft");
		saveAttribute(DelayTimeRight, "DelayTimeRight");
		saveAttribute(FeedbackLeft, "FeedbackLeft");
		saveAttribute(FeedbackRight, "FeedbackRight");
		saveAttribute(LowPassFreq, "LowPassFreq");
		saveAttribute(HiPassFreq, "HiPassFreq");
		saveAttribute(Mix, "Mix");
		saveAttribute(TempoSync, "TempoSync");

		return v;

	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
        
        leftDelay.prepareToPlay(sampleRate);
        rightDelay.prepareToPlay(sampleRate);
        
		calcDelayTimes();

		leftDelayFrames = AudioSampleBuffer(1, samplesPerBlock);
		rightDelayFrames = AudioSampleBuffer(1, samplesPerBlock);

		leftDelayFrames.clear();
		rightDelayFrames.clear();
		
	};

	void calcDelayTimes()
	{
		const float actualLeftTime = tempoSync ? TempoSyncer::getTempoInMilliSeconds(getMainController()->getBpm(), syncTimeLeft) :
												 delayTimeLeft;

		const float actualRightTime = tempoSync ? TempoSyncer::getTempoInMilliSeconds(getMainController()->getBpm(), syncTimeRight) :
												 delayTimeRight;

        leftDelay.setDelayTimeSeconds(actualLeftTime * 0.001);
        rightDelay.setDelayTimeSeconds(actualRightTime * 0.001);
	}

	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		if(skipFirstBuffer)
		{
			skipFirstBuffer = false;
			return;
		}

		const int sampleIndex = startSample;
		const int samplesToCopy = numSamples;

		float *inputL = buffer.getWritePointer(0, 0);
		float *inputR = buffer.getWritePointer(1, 0);
        
        while(--numSamples >= 0)
        {
            leftDelayFrames.setSample(0, startSample, (float)leftDelay.getDelayedValue(inputL[startSample] + feedbackLeft * leftDelayFrames.getSample(0, startSample)));
            rightDelayFrames.setSample(0, startSample, (float)rightDelay.getDelayedValue(inputR[startSample] + feedbackRight * rightDelayFrames.getSample(0, startSample)));
            
            ++startSample;
        }

        const float dryMix = (mix < 0.5f) ? 1.0f : (2.0f - 2.0f * mix);
        const float wetMix = (mix > 0.5f) ? 1.0f : (2.0f * mix);
        
        FloatVectorOperations::multiply(buffer.getWritePointer(0, sampleIndex), dryMix, samplesToCopy);
        FloatVectorOperations::multiply(buffer.getWritePointer(1, sampleIndex), dryMix, samplesToCopy);

		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0, sampleIndex), leftDelayFrames.getReadPointer(0, sampleIndex), wetMix, samplesToCopy);
		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(1, sampleIndex), rightDelayFrames.getReadPointer(0, sampleIndex), wetMix, samplesToCopy);
	};

	bool hasTail() const override {return false; };

	int getNumChildProcessors() const override { return 0; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

private:

	// Unsynced
	float delayTimeLeft;
	float delayTimeRight;

	float timeLeft;
	float timeRight;

	TempoSyncer::Tempo syncTimeLeft;
	TempoSyncer::Tempo syncTimeRight;

	float feedbackLeft;
	float feedbackRight;
	float lowPassFreq;
	float hiPassFreq;
	float mix;
	bool tempoSync;

	AudioSampleBuffer leftDelayFrames;
	AudioSampleBuffer rightDelayFrames;
    
    DelayLine leftDelay;
    DelayLine rightDelay;

	bool skipFirstBuffer;
};






#endif  // DELAY_H_INCLUDED
