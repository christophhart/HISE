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

ProcessorEditorBody *MdaLimiterEffect::createEditor(BetterProcessorEditor *parentEditor)
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
	dryWetChain(new ModulatorChain(mc, "FX Modulation", 1, ModulatorChain::GainMode, this))
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

ProcessorEditorBody *MdaDegradeEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MdaDegradeEditor(parentEditor);


#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};