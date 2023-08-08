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

namespace hise { using namespace juce;

MdaLimiterEffect::MdaLimiterEffect(MainController *mc, const String &id):
	MdaEffectWrapper(mc, id)
{
	effect = new mdaLimiter();

	finaliseModChains();

	parameterNames.add("Threshhold");
	parameterNames.add("Output");
	parameterNames.add("Attack");
	parameterNames.add("Release");
	parameterNames.add("Knee");
};

ProcessorEditorBody *MdaLimiterEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MdaLimiterEditor(parentEditor);

	
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};

MdaDegradeEffect::MdaDegradeEffect(MainController *mc, const String &id):
	MdaEffectWrapper(mc, id),
	dryWet(1.0f)
{
	modChains += {this, "FX Modulation"};

	finaliseModChains();

	dryWetChain = modChains[InternalChains::DryWetChain].getChain();
	modChains[InternalChains::DryWetChain].setExpandToAudioRate(true);
	modChains[InternalChains::DryWetChain].setAllowModificationOfVoiceValues(true);

	parameterNames.add("Headroom");
	parameterNames.add("Quant");
	parameterNames.add("Rate");
	parameterNames.add("PostFilt");
	parameterNames.add("NonLin");
	parameterNames.add("DryWet");

	editorStateIdentifiers.add("DryWetChainShown");

	effect = new mdaDegrade();
};

ProcessorEditorBody *MdaDegradeEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MdaDegradeEditor(parentEditor);


#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};

void MdaDegradeEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	FloatVectorOperations::copy(inputBuffer.getWritePointer(0, startSample), buffer.getReadPointer(0, startSample), numSamples);
	FloatVectorOperations::copy(inputBuffer.getWritePointer(1, startSample), buffer.getReadPointer(1, startSample), numSamples);

	float *inputData[2];

	inputData[0] = inputBuffer.getWritePointer(0, startSample);
	inputData[1] = inputBuffer.getWritePointer(1, startSample);

	float *outputData[2];

	outputData[0] = buffer.getWritePointer(0, startSample);
	outputData[1] = buffer.getWritePointer(1, startSample);

	effect->processReplacing(inputData, outputData, numSamples);

	

	if (auto dryWetModValues = modChains[InternalChains::DryWetChain].getWritePointerForVoiceValues(startSample))
	{
		FloatVectorOperations::multiply(dryWetModValues, dryWet, numSamples);

		FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), dryWetModValues, numSamples);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), dryWetModValues, numSamples);

		FloatVectorOperations::multiply(dryWetModValues, -1.0f, numSamples);
		FloatVectorOperations::add(dryWetModValues, 1.0f, numSamples);

		FloatVectorOperations::multiply(inputData[0], dryWetModValues, numSamples);
		FloatVectorOperations::multiply(inputData[1], dryWetModValues, numSamples);

		FloatVectorOperations::add(buffer.getWritePointer(0, startSample), inputData[0], numSamples);
		FloatVectorOperations::add(buffer.getWritePointer(1, startSample), inputData[1], numSamples);
	}
	else
	{
		const float modValue = dryWet * modChains[DryWetChain].getConstantModulationValue();

		FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), modValue, numSamples);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startSample), modValue, numSamples);
		FloatVectorOperations::multiply(inputData[0], 1.0f - modValue, numSamples);
		FloatVectorOperations::multiply(inputData[1], 1.0f - modValue, numSamples);
		FloatVectorOperations::add(buffer.getWritePointer(0, startSample), inputData[0], numSamples);
		FloatVectorOperations::add(buffer.getWritePointer(1, startSample), inputData[1], numSamples);
	}
	
}

} // namespace hise
