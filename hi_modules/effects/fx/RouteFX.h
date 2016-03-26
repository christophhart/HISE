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

#ifndef ROUTEFX_H_INCLUDED
#define ROUTEFX_H_INCLUDED

 



/** A simple gain effect that allows time variant modulation. */
class RouteEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("RouteFX", "Routing Matrix");

	RouteEffect(MainController *mc, const String &uid);;

	float getAttribute(int ) const override { return 1.0f; };

	

	void setInternalAttribute(int , float ) override {};


	void restoreFromValueTree(const ValueTree &v) override
	{
		MasterEffectProcessor::restoreFromValueTree(v);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MasterEffectProcessor::exportAsValueTree();
		
		return v;
	}


	int getNumInternalChains() const override { return 0; };

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	int getNumChildProcessors() const override { return 0; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	}

	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
	

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override;

private:

	

};



#endif  // ROUTEFX_H_INCLUDED
