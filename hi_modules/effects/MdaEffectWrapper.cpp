/*
  ==============================================================================

    MdaEffectWrapper.cpp
    Created: 2 Aug 2014 10:39:44pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "MdaEffectWrapper.h"


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

	jassertfalse;

	return nullptr;

#endif
};