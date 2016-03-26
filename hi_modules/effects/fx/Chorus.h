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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef CHORUS_H_INCLUDED
#define CHORUS_H_INCLUDED

#define BUFMAX   2048

/** 
*/
class ChorusEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Chorus", "Chorus");

	/** The parameters */
	enum Parameters
	{
		Rate,
		Width,
		Feedback,
		Delay,
		numEffectParameters
	};

	ChorusEffect(MainController *mc, const String &id);;

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
	void processReplacing(const float** inputs, float** outputs, int sampleFrames);

	void calculateInternalValues();

private:

	AudioSampleBuffer tempBuffer;

	///global internal variables
	float rat, dep, wet, dry, fb, dem; //rate, depth, wet & dry mix, feedback, mindepth
	float phi, fb1, fb2, deps;         //lfo & feedback buffers, depth change smoothing 
	float *buffer, *buffer2;           //maybe use 2D buffer?
	uint32 size, bufpos;

	float parameterRate;
	float parameterDepth;
	float parameterMix;
	float parameterFeedback;
	float parameterDelay;
};







#endif  
