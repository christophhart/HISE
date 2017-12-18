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
	dryWet(1.0f),
	dryWetChain(new ModulatorChain(mc, "FX Modulation", 1, ModulatorChain::GainMode, this)),
    dryWetBuffer(1, 0)
{
	parameterNames.add("Headroom");
	parameterNames.add("Quant");
	parameterNames.add("Rate");
	parameterNames.add("PostFilt");
	parameterNames.add("NonLin");
	parameterNames.add("DryWet");

	editorStateIdentifiers.add("DryWetChainShown");

	useStepSizeCalculation(false);
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

} // namespace hise
