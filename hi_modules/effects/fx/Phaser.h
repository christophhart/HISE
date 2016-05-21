/*
  ==============================================================================

    Phaser.h
    Created: 17 May 2016 8:47:28pm
    Author:  Christoph Hart

  ==============================================================================
*/

#ifndef PHASER_H_INCLUDED
#define PHASER_H_INCLUDED

class PhaserEffect: public MasterEffectProcessor
{
public:
    
    SET_PROCESSOR_NAME("Phaser", "Phaser")
    
    enum Attributes
    {
        Speed = 0,
        Range,
        Feedback,
        Mix,
        numAttributes
    };
    
    PhaserEffect(MainController *mc, const String &id);;
    
    float getAttribute(int parameterIndex) const override;;
    void setInternalAttribute(int parameterIndex, float newValue) override;;
    
    void restoreFromValueTree(const ValueTree &v) override;;
    ValueTree exportAsValueTree() const override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
    void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
    
    bool hasTail() const override { return false; };
    int getNumChildProcessors() const override { return 0; };
    Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
    const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };
    
    ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;
    
private:
    
    
    
};

#endif  // PHASER_H_INCLUDED
