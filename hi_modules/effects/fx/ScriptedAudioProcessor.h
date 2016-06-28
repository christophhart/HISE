/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licences for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licencing:
 *
 *   http://www.hartinstruments.net/hise/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#ifndef SCRIPTEDAUDIOPROCESSOR_H_INCLUDED
#define SCRIPTEDAUDIOPROCESSOR_H_INCLUDED



#define SET_MODULE_NAME(x) static Identifier getName() {static const Identifier id(x); return id; };

class ScriptingDsp
{
public:
    
    class BaseObject : public DynamicObject
    {
    public:
        
        SET_MODULE_NAME("Base")
        
    private:
        
    };
    
    struct Buffer : public BaseObject
    {
        Buffer(AudioSampleBuffer *externalBuffer=nullptr)
        {
            if (externalBuffer != nullptr)
            {
                buffer.setDataToReferTo(externalBuffer->getArrayOfWritePointers(), externalBuffer->getNumChannels(), externalBuffer->getNumSamples());
            }
            
            initMethods();
        }
        
        
        
        Buffer(int channels, int samples)
        {
            setSize(channels, samples);
            
            buffer.clear();
            
            initMethods();
        }
        
        void initMethods()
        {
            
            setMethod("setSize", Wrappers::setSize);
            setMethod("copyFrom", Wrappers::copyFrom);
            setMethod("add", Wrappers::add);
            setMethod("getSample", Wrappers::getSample);
            setMethod("setSample", Wrappers::setSample);
            setMethod("getNumSamples", Wrappers::getNumSamples);
        }
        
        SET_MODULE_NAME("buffer")
        
        void referToBuffer(AudioSampleBuffer &b)
        {
            buffer.setDataToReferTo(b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
        }
        
        /** Resizes the buffer and clears it. */
        void setSize(int numChannels, int numSamples)
        {
            buffer.setSize(numChannels, numSamples);
            buffer.clear();
        }
        
        
        var getSample(int channelIndex, int sampleIndex)
        {
            return buffer.getSample(channelIndex, sampleIndex);
        }
        
        void setSample(int channelIndex, int sampleIndex, float newValue)
        {
            buffer.setSample(channelIndex, sampleIndex, newValue);
        }
        
        var getNumSamples() const
        {
            return buffer.getNumSamples();
        }
        
        void copyFrom(Buffer &otherBuffer)
        {
            buffer.copyFrom(0, 0, otherBuffer.buffer, 0, 0, buffer.getNumSamples());
            buffer.copyFrom(1, 0, otherBuffer.buffer, 1, 0, buffer.getNumSamples());
        }
        
        void add(Buffer &otherBuffer)
        {
            buffer.addFrom(0, 0, otherBuffer.buffer, 0, 0, buffer.getNumSamples());
            buffer.addFrom(1, 0, otherBuffer.buffer, 1, 0, buffer.getNumSamples());
        }
        
        AudioSampleBuffer buffer;
        
        struct Wrappers
        {
            static var setSize(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    thisObject->setSize(args.arguments[0], args.arguments[1]);
                }
                return var::undefined();
            }
            
            static var add(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    Buffer *other = dynamic_cast<Buffer*>(args.arguments[0].getObject());
                    
                    if (other != nullptr)
                    {
                        thisObject->add(*other);
                    }
                }
                return var::undefined();
            }
            
            static var copyFrom(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    Buffer *other = dynamic_cast<Buffer*>(args.arguments[0].getObject());
                    
                    if (other != nullptr)
                    {
                        thisObject->copyFrom(*other);
                    }
                }
                return var::undefined();
            }
            
            static var getSample(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    return thisObject->getSample(args.arguments[0], args.arguments[1]);
                }
                return var::undefined();
            }
            
            static var setSample(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    thisObject->setSample(args.arguments[0], args.arguments[1], args.arguments[2]);
                }
                return var::undefined();
            }
            
            static var getNumSamples(const var::NativeFunctionArgs& args)
            {
                if (Buffer* thisObject = dynamic_cast<Buffer*>(args.thisObject.getObject()))
                {
                    return thisObject->getNumSamples();
                }
                return var::undefined();
            }
        };
    };
    
    class DspObject : public BaseObject
    {
    public:
        
        DspObject()
        {
            setMethod("processBlock", Wrappers::processBlock);
            setMethod("prepareToPlay", Wrappers::prepareToPlay);
            setMethod("getBuffer", Wrappers::getBuffer);
        }
        
        /** Call this to setup the module*/
        virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
        
        /** Call this to process the given buffer.
         *
         *	If you don't want inplace processing (eg. for delays), store it in a internal buffer and return this buffer using getBuffer().
         */
        virtual void processBlock(Buffer &buffer) = 0;
        
        /** Return the internal buffer of the object.
         *
         *	This should be only used by non inplace calculations.
         */
        virtual var getBuffer() { return var::undefined(); }
        
        struct Wrappers
        {
            static var processBlock(const var::NativeFunctionArgs& args)
            {
                if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
                {
                    Buffer *buffer = dynamic_cast<Buffer*>(args.arguments[0].getObject());
                    
                    thisObject->processBlock(*buffer);
                }
                return var::undefined();
            }
            
            static var prepareToPlay(const var::NativeFunctionArgs& args)
            {
                if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
                {
                    thisObject->prepareToPlay(args.arguments[0], args.arguments[1]);
                }
                return var::undefined();
            }
            
            static var getBuffer(const var::NativeFunctionArgs& args)
            {
                if (DspObject* thisObject = dynamic_cast<DspObject*>(args.thisObject.getObject()))
                {
                    return thisObject->getBuffer();
                }
                return var::undefined();
            }
        };
    };
    
    class Gain : public DspObject
    {
    public:
        
        Gain():
        DspObject()
        {
            setMethod("setGain", Wrappers::setGain);
        }
        
        SET_MODULE_NAME("gain")
        
        void prepareToPlay(double sampleRate, int samplesPerBlock) override {};
        
        void processBlock(Buffer &buffer) override
        {
            float **out = buffer.buffer.getArrayOfWritePointers();
            
            float *l = out[0];
            float *r = out[1];
            
            const int numSamples = buffer.buffer.getNumSamples();
            
            FloatVectorOperations::multiply(l, gain, numSamples);
            FloatVectorOperations::multiply(r, gain, numSamples);
        }
        
        void setGain(float newGain) noexcept{ gain = newGain; };
        
        struct Wrappers
        {
            static var setGain(const var::NativeFunctionArgs& args)
            {
                if (Gain* thisObject = dynamic_cast<Gain*>(args.thisObject.getObject()))
                {
                    thisObject->setGain(args.arguments[0]);
                }
                return var::undefined();
            }
        };
        
    private:
        
        float gain = 1.0f;
        
    };
    
    class Delay : public DspObject
    {
    public:
        
        Delay() :
        DspObject()
        {
            setMethod("setDelayTime", Wrappers::setDelayTime);
        }
        
        SET_MODULE_NAME("delay")
        
        void setDelayTime(int newDelayInSamples)
        {
            delayL.setDelayTimeSamples(newDelayInSamples);
            delayR.setDelayTimeSamples(newDelayInSamples);
        }
        
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            delayedBuffer = new Buffer(2, samplesPerBlock);
            
            
            
            delayL.prepareToPlay(sampleRate);
            delayR.prepareToPlay(sampleRate);
        }
        
        void processBlock(Buffer &b) override
        {
            const float *inL = b.buffer.getReadPointer(0);
            const float *inR = b.buffer.getReadPointer(1);
            
            float *l = delayedBuffer->buffer.getWritePointer(0);
            float *r = delayedBuffer->buffer.getWritePointer(1);
            
            int numSamples = b.buffer.getNumSamples();
            
            while (--numSamples >= 0)
            {
                *l++ = delayL.getDelayedValue(*inL++);
                *r++ = delayL.getDelayedValue(*inR++);
            }
        }
        
        var getBuffer() override
        {
            return var(delayedBuffer);
        }
        
        struct Wrappers
        {
            static var setDelayTime(const var::NativeFunctionArgs& args)
            {
                if (Delay* thisObject = dynamic_cast<Delay*>(args.thisObject.getObject()))
                {
                    thisObject->setDelayTime(args.arguments[0]);
                }
                return var::undefined();
            }
        };
        
        
        
    private:
        
        DelayLine delayL;
        DelayLine delayR;
        
        ReferenceCountedObjectPtr<Buffer> delayedBuffer;
    };
    
    class Biquad: public DspObject
    {
    public:
        
        enum class Mode
        {
            LowPass = 0,
            HighPass,
            LowShelf,
            HighShelf,
            Peak,
            numModes
        };
        
        SET_MODULE_NAME("biquad")
        
        Biquad():
          DspObject()
        {
            coefficients = IIRCoefficients::makeLowPass(44100.0, 20000.0);
            
            setProperty("LowPass", LowPass);
            setProperty("HighPass", HighPass);
            setProperty("LowShelf", LowShelf);
            setProperty("HighShelf", HighShelf);
            setProperty("Peak", Peak);
            
            setMethod("setMode", Wrappers::setMode);
            setMethod("setFrequency", Wrappers::setFrequency);
            setMethod("setGain", Wrappers::setGain);
            setMethod("setQ", Wrappers::setQ);
        }
        
        void prepareToPlay(double sampleRate_, int samplesPerBlock) override
        {
            sampleRate = sampleRate_;
        }
        
        void setMode(Mode newMode)
        {
            m = newMode;
            calcCoefficients();
        }
        
        void setFrequency(double newFrequency)
        {
            frequency = newFrequency;
            calcCoefficients();
        }
        
        void setGain(double newGain)
        {
            gain = newGain;
            calcCoefficients();
        }
        
        void setQ(double newQ)
        {
            q = newQ;
            calcCoefficients();
        }
        
        void processBlock(Buffer &b) override
        {
            float *inL = b.buffer.getWritePointer(0);
            float *inR = b.buffer.getWritePointer(1);
            
            const int numSamples = b.buffer.getNumSamples();

            leftFilter.processSamples(inL, numSamples);
            rightFilter.processSamples(inR, numSamples);
        }
        
    private:
        
        struct Wrappers
        {
            static var setMode(const var::NativeFunctionArgs& args)
            {
                if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
                {
                    thisObject->setMode((Mode)(int)args.arguments[0]);
                }
                return var::undefined();
            }
            
            static var setFrequency(const var::NativeFunctionArgs& args)
            {
                if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
                {
                    thisObject->setFrequency((double)args.arguments[0]);
                }
                return var::undefined();
            }
            
            static var setGain(const var::NativeFunctionArgs& args)
            {
                if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
                {
                    thisObject->setGain((double)args.arguments[0]);
                }
                return var::undefined();
            }
            
            static var setQ(const var::NativeFunctionArgs& args)
            {
                if (Biquad* thisObject = dynamic_cast<Biquad*>(args.thisObject.getObject()))
                {
                    thisObject->setQ((double)args.arguments[0]);
                }
                return var::undefined();
            }
        };
        
        void calcCoefficients()
        {
            switch(m)
            {
                case Mode::LowPass: coefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
                case Mode::HighPass: coefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
                case Mode::LowShelf: coefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, gain); break;
                case Mode::HighShelf: coefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, gain); break;
                case Mode::Peak:      coefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, gain); break;
            }
            
            leftFilter.setCoefficients(coefficients);
            rightFilter.setCoefficients(coefficients);
        }
        
        double sampleRate = 44100.0;
        
        Mode m = Mode::LowPass;
        
        double gain = 0.0;
        double frequency = 20000.0;
        double q = 1.0;
        
        IIRFilter leftFilter;
        IIRFilter rightFilter;
        
        IIRCoefficients coefficients;
        
    };
    
    class Factory
    {
    public:
        
        Factory()
        {};
        
        static void registerModule(const Identifier &id)
        {
            moduleIds.add(id);
        }
        
        static var createModule(const var::NativeFunctionArgs& args)
        {
            if (args.numArguments == 1)
            {
                const Identifier id = Identifier(args.arguments[0].toString());
                
                if (id.isValid())
                {
                    if (id == Gain::getName())
                    {
                        Gain *b = new Gain();
                        return var(b);
                    }
                    else if (id == Delay::getName())
                    {
                        Delay *d = new Delay();
                        return var(d);
                    }
                    else if (id == Biquad::getName())
                    {
                        Biquad *b = new Biquad();
                        return var(b);
                    }
                }
            }
            
            return var::undefined();
        }
        
        
    private:
        
        static Array<Identifier> moduleIds;
    };
};

#define REGISTER_DSP_SCRIPTING_MODULE(x) ScriptingDsp::Factory::registerModule(ScriptingDsp::x::getName());

class ScriptingAudioProcessor: public AudioProcessor
{
public:
    
    ScriptingAudioProcessor():
    samplesPerBlock(0),
    callbackResult(Result::ok()),
    mc(nullptr)
    {
        REGISTER_DSP_SCRIPTING_MODULE(Buffer);
        REGISTER_DSP_SCRIPTING_MODULE(Gain);
        REGISTER_DSP_SCRIPTING_MODULE(Delay);
        REGISTER_DSP_SCRIPTING_MODULE(Biquad);
        
        compileScript();
    }
    
    void compileScript()
    {
        ScopedLock sl(compileLock);
        
        scriptingEngine = new HiseJavascriptEngine();
        
        processBlockScope = new DynamicObject();
        
        currentBuffer = new ScriptingDsp::Buffer();
        
        scriptingEngine->getRootObject()->setMethod("createModule", ScriptingDsp::Factory::createModule);
        
        Result r = scriptingEngine->execute(doc.getAllContent());
        
        callbackResult = Result::ok();
        
        prepareToPlay(getSampleRate(), samplesPerBlock);
    };
    
    void prepareToPlay(double sampleRate, int samplesPerBlock_)
    {
        ScopedLock sl(compileLock);
        
        samplesPerBlock = samplesPerBlock_;
        
        const var arguments[2] = { sampleRate, samplesPerBlock };
        
        var::NativeFunctionArgs args(this, arguments, 2);
        
        
        
        scriptingEngine->callFunction("prepareToPlay", args, &callbackResult);
        
        if (mc != nullptr && callbackResult.failed())
        {
            mc->writeToConsole(callbackResult.getErrorMessage(), 1);
        }
        
        
    }
    
    void releaseResources() {};
    
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        if (callbackResult.wasOk())
        {
            ScopedLock sl(compileLock);
            
            currentBuffer->referToBuffer(buffer);
            
            const var arguments[3] = { var(currentBuffer), 0, buffer.getNumSamples() };
            
            var::NativeFunctionArgs args(this, arguments, 3);
            
            static const Identifier id("processBlock");
            
            scriptingEngine->executeWithoutAllocation(id, args, &callbackResult, processBlockScope);
            
            if (mc != nullptr && callbackResult.failed())
            {
                mc->writeToConsole(callbackResult.getErrorMessage(), 1);
            }
        }
    }
    
    void getStateInformation(MemoryBlock& destData) override
    {
        
    }
    
    void setMainController(MainController *mc_)
    {
        mc = mc_;
    }
    
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        
    }
    
    bool setPreferredBusArrangement(bool isInputBus, int busIndex,
                                    const AudioChannelSet& preferred) override
    {
        const int numChannels = preferred.size();
        
        // do not allow disabling channels
        if (numChannels == 0) return false;
        
        // always have the same channel layout on both input and output on the main bus
        if (!AudioProcessor::setPreferredBusArrangement(!isInputBus, busIndex, preferred))
            return false;
        
        return AudioProcessor::setPreferredBusArrangement(isInputBus, busIndex, preferred);
    }
    
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override               { return true; }
    
    //==============================================================================
    const String getName() const override               { return "Gain PlugIn"; }
    
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0; }
    
    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram(int) override                  {}
    const String getProgramName(int) override             { return String(); }
    void changeProgramName(int, const String&) override { }
    
    static AudioProcessor *create() { return new ScriptingAudioProcessor(); }
    
    CodeDocument &getDocument()
    {
        return doc;
    }
    
private:
    
    MainController *mc;
    
    Result callbackResult = Result::ok();
    
    DynamicObject::Ptr processBlockScope = nullptr;
    
    int samplesPerBlock = 0;
    
    ReferenceCountedObjectPtr<ScriptingDsp::Buffer> currentBuffer;
    
    CriticalSection compileLock;
    
    CodeDocument doc;
    
    ScopedPointer<HiseJavascriptEngine> scriptingEngine;
};


class ScriptingAudioProcessorEditor : public AudioProcessorEditor,
public ButtonListener
{
public:
    
    ScriptingAudioProcessorEditor(AudioProcessor *p):
    AudioProcessorEditor(p),
    tokenizer(new JavascriptTokeniser())
    {
        addAndMakeVisible(controls = new GenericEditor(*p));
        addAndMakeVisible(codeEditor = new CodeEditorComponent(dynamic_cast<ScriptingAudioProcessor*>(p)->getDocument(), tokenizer));
        addAndMakeVisible(compileButton = new TextButton("Compile"));
        
        codeEditor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
        codeEditor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
        codeEditor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
        codeEditor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
        codeEditor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
        codeEditor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
        codeEditor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
        
        compileButton->addListener(this);
        
        setSize(1000, controls->getHeight() + 600);
    }
    
    void resized()
    {
        controls->setBounds(0, 0, getWidth(), jmax<int>(controls->getHeight(), 50));
        codeEditor->setBounds(0, controls->getBottom(), getWidth(), getHeight() - controls->getBottom());
        compileButton->setBounds(0, 0, 80, 20);
    }
    
    void buttonClicked(Button *b) override
    {
        if (b == compileButton)
        {
            dynamic_cast<ScriptingAudioProcessor*>(getAudioProcessor())->compileScript();
        }
    }
    
private:
    
    ScopedPointer<GenericEditor> controls;
    
    ScopedPointer<JavascriptTokeniser> tokenizer;
    
    ScopedPointer<CodeEditorComponent> codeEditor;
    
    ScopedPointer<TextButton> compileButton;
};


#endif  // SCRIPTEDAUDIOPROCESSOR_H_INCLUDED
