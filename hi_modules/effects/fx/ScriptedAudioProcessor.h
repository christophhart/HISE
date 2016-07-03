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



class ScriptingAudioProcessor: public AudioProcessor
{
public:
    
    ScriptingAudioProcessor();
    
    void compileScript()
    {
        ScopedLock sl(compileLock);
        
        scriptingEngine = new HiseJavascriptEngine();
        
		

        processBlockScope = new DynamicObject();
        
		DspFactory *f = new DspFactory();

		scriptingEngine->registerNativeObject("Modules", f);

		scriptingEngine->getRootObject()->setMethod("print", print);

		scriptingEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));

		scriptingEngine->registerApiClass(new MathFunctions());

        callbackResult = scriptingEngine->execute(doc.getAllContent());
        
		if (callbackResult.failed())
		{
			Logger::writeToLog("!" + callbackResult.getErrorMessage());
		}
		else
		{
			prepareToPlay(sampleRate, samplesPerBlock);
		}
    };
    
    void prepareToPlay(double sampleRate_, int samplesPerBlock_)
    {
		

		if (sampleRate_ > 0)
		{
			sampleRate = sampleRate_;

			leftChannel = new VariantBuffer(samplesPerBlock_);
			rightChannel = new VariantBuffer(samplesPerBlock_);

			ScopedLock sl(compileLock);

			samplesPerBlock = samplesPerBlock_;

			const var arguments[2] = { sampleRate, samplesPerBlock };

			var::NativeFunctionArgs args(this, arguments, 2);

			scriptingEngine->callFunction("prepareToPlay", args, &callbackResult);

			if (callbackResult.failed())
			{
				Logger::writeToLog("!" + callbackResult.getErrorMessage());
			}
		}

    }
    
    void releaseResources() {};
    
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        if (callbackResult.wasOk())
        {
			ScopedLock sl(compileLock);
            
			leftChannel->referToData(buffer.getWritePointer(0), buffer.getNumSamples());
			rightChannel->referToData(buffer.getWritePointer(1), buffer.getNumSamples());

			var channels[2] = { var(leftChannel), var(rightChannel) };
			Array<var> c(channels, 2);
			const var arguments[1] = { var(c) };
			var::NativeFunctionArgs args(this, arguments, 1);

            static const Identifier id("processBlock");
            
            scriptingEngine->executeWithoutAllocation(id, args, &callbackResult, processBlockScope);
            
			if (callbackResult.failed())
			{
				Logger::writeToLog("!" + callbackResult.getErrorMessage());
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
    
	static var print(const var::NativeFunctionArgs &args)
	{
		if (args.numArguments > 0)
		{
			Logger::writeToLog(args.arguments[0].toString());
		}

		return var::undefined();
	}

    CodeDocument &getDocument()
    {
        return doc;
    }
    
private:
    
    MainController *mc;
    
    Result callbackResult = Result::ok();
    
    DynamicObject::Ptr processBlockScope = nullptr;
    
    int samplesPerBlock = 0;
    
	double sampleRate = 0.0;

	VariantBuffer::Ptr leftChannel;
	VariantBuffer::Ptr rightChannel;

	Array<var> channelData;

    CriticalSection compileLock;
    
    CodeDocument doc;
    
    ScopedPointer<HiseJavascriptEngine> scriptingEngine;

	Factory<ScriptingDsp::BaseObject> objectFactory;
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
    
	Factory<ScriptingDsp::BaseObject> factory;

    ScopedPointer<GenericEditor> controls;
    
    ScopedPointer<JavascriptTokeniser> tokenizer;
    
    ScopedPointer<CodeEditorComponent> codeEditor;
    
    ScopedPointer<TextButton> compileButton;
};


#endif  // SCRIPTEDAUDIOPROCESSOR_H_INCLUDED
