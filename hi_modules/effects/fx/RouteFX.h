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

#ifndef ROUTEFX_H_INCLUDED
#define ROUTEFX_H_INCLUDED

 
namespace hise { using namespace juce;


/** A signal chain tool that allows to duplicate and send the signal to other channels to build AUX signal paths.
	@ingroup effectTypes.

*/
class RouteEffect : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("RouteFX", "Routing Matrix", "A signal chain tool that allows to duplicate and send the signal to other channels to build AUX signal paths.");

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

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	}

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;
	

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override;

private:

	

};


} // namespace hise

#endif  // ROUTEFX_H_INCLUDED
