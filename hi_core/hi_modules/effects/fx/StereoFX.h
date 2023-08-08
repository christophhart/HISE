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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef STEREOFX_H_INCLUDED
#define STEREOFX_H_INCLUDED

namespace hise { using namespace juce;

class MidSideDecoder
{
public:
    
    MidSideDecoder():
    width(1.0f)
    {};
    
    void calculateStereoValues(float &left, float &right);
    
    void setWidth(float newValue) noexcept;
    
    float getWidth() const noexcept;
    
private:
    
    float width;
};

/** A simple stereo panner which can be modulated using all types of Modulators
*	@ingroup effectTypes
*
*/
class StereoEffect: public VoiceEffectProcessor
{
public:

	SET_PROCESSOR_NAME("StereoFX", "Stereo FX", "A polyphonic stereo panner.");

	enum InternalChains
	{
		BalanceChain = 0,
		numInternalChains
	};

	/** The parameters */
	enum Parameters
	{
		Pan = 0, ///< the balance of the effect. If you want to modulate this parameter, set it to the biggest value and add Modulators to the Chain.
		Width ///< the stereo width using a M/S matrix
	};

	enum EditorStates
	{
		PanChainShown = Processor::numEditorStates,
		numEditorStates
	};

	StereoEffect(MainController *mc, const String &uid, int numVoices);;

	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;;
	
	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples);;

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int processorIndex) override { return modChains[processorIndex].getChain(); };
	const Processor *getChildProcessor(int processorIndex) const override { return modChains[processorIndex].getChain(); };
	int getNumChildProcessors() const override { return modChains.size(); };
	int getNumInternalChains() const override { return modChains.size(); };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

private:

    MidSideDecoder msDecoder;
    
	float pan;

	JUCE_DECLARE_WEAK_REFERENCEABLE(StereoEffect);
};






} // namespace hise

#endif  // STEREOFX_H_INCLUDED
