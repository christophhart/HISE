/*
  ==============================================================================

    pLagrangeInterpolator.h
    Created: 6 Jul 2014 3:12:08am
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"
#include "../LuaState.h"

PROTO_API exLagrangeInterpolator LagrangeInterpolator_create()
{ 
	LagrangeInterpolator li;
	exLagrangeInterpolator ret(*(exLagrangeInterpolator*)(&li)); // look away children
	return ret;
}

PROTO_API int LagrangeInterpolator_process(exLagrangeInterpolator ex,
					double speedRatio,
					const float* inputSamples,
					float* outputSamples,
					int numOutputSamplesToProduce)
{
	LagrangeInterpolator *li = (LagrangeInterpolator*)(&ex);
	return li->process(speedRatio, inputSamples, outputSamples, numOutputSamplesToProduce);
}
