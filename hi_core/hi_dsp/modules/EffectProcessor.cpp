void EffectProcessor::checkTailing(AudioSampleBuffer &b, int startSample, int numSamples)
{
	// Call this only on effects that produce a tail!
	jassert(hasTail());

	const float maxInL = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(0, startSample), numSamples);
	const float maxInR = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(1, startSample), numSamples);

	const float maxL = FloatVectorOperations::findMaximum(b.getReadPointer(0, startSample), numSamples);
	const float maxR = FloatVectorOperations::findMaximum(b.getReadPointer(1, startSample), numSamples);

	const float in = maxInL + maxInR;
	const float out = maxL + maxR;
		
	isTailing = (in == 0.0f && out >= 0.01f);
}