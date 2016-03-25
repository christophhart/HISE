/*
  ==============================================================================

    SineSynth.cpp
    Created: 10 Jul 2014 12:40:14pm
    Author:  Chrisboy

  ==============================================================================
*/

ProcessorEditorBody* SineSynth::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new SineSynthBody(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}




float const * SineSynth::getSaturatedTableValues()
{
	

	for (int i = 0; i < 128; i++)
	{
		const float sinValue = sin((float)i / 64.0f * float_Pi);

		saturatedTableValues[i] = saturator.getSaturatedSample(sinValue);
	}

	return saturatedTableValues;
}

void SineSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getVoicePitchValues();

	const float *modValues = getVoiceGainValues(startSample, numSamples);

	float saturation = dynamic_cast<SineSynth*>(getOwnerSynth())->saturationAmount;

	float *leftValues = voiceBuffer.getWritePointer(0, startSample);
	
#if 0

	while (numSamples -4 >= 0)
	{
		numSamples -= 4;

		const float voiceUptimeFloat = (float)voiceUptime;
		const float uptimeDeltaFloat = (float)uptimeDelta;

		__m128 uptimeDeltas = _mm_mul_ps(_mm_load_ps(voicePitchValues + startSample), _mm_set_ps1(uptimeDeltaFloat));
		__m128 uptimes;

		uptimes.m128_f32[0] = voiceUptimeFloat + uptimeDeltas.m128_f32[0];
		uptimes.m128_f32[1] = uptimes.m128_f32[0] + uptimeDeltas.m128_f32[1];
		uptimes.m128_f32[2] = uptimes.m128_f32[1] + uptimeDeltas.m128_f32[2];
		uptimes.m128_f32[3] = uptimes.m128_f32[2] + uptimeDeltas.m128_f32[3];

		const float increase = uptimes.m128_f32[3];

		

		__m128i index = _mm_cvtps_epi32(_mm_sub_ps(uptimes, _mm_set_ps(0.5f, 0.5f, 0.5f, 0.5f)));
		__m128 alphas = _mm_mul_ps(_mm_set_ps(-1.0f, -1.0f, -1.0f, -1.0f), _mm_cvtepi32_ps(index));

		alphas = _mm_add_ps(alphas, uptimes);


		__m128i index2 = _mm_add_epi32(index, _mm_set_epi32(1, 1, 1, 1));
		__m128i mask = _mm_set_epi32(2047, 2047, 2047, 2047);
		index = _mm_and_si128(index, mask);
		index2 = _mm_and_si128(index2, mask);

		__m128 v1;
		
		v1.m128_f32[0] = sinTable[index.m128i_i32[0]];
		v1.m128_f32[1] = sinTable[index.m128i_i32[1]];
		v1.m128_f32[2] = sinTable[index.m128i_i32[2]];
		v1.m128_f32[3] = sinTable[index.m128i_i32[3]];

		__m128 v2;

		v2.m128_f32[0] = sinTable[index2.m128i_i32[0]];
		v2.m128_f32[1] = sinTable[index2.m128i_i32[1]];
		v2.m128_f32[2] = sinTable[index2.m128i_i32[2]];
		v2.m128_f32[3] = sinTable[index2.m128i_i32[3]];

		__m128 invAlpha = _mm_mul_ps(alphas, _mm_set_ps(-1.0f, -1.0f, -1.0f, -1.0f));
		invAlpha = _mm_add_ps(invAlpha, _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f));

		__m128 output = _mm_add_ps(_mm_mul_ps(v1, invAlpha), _mm_mul_ps(v2, alphas));
		_mm_store_ps(leftValues, output);

		leftValues += 4;
		voiceUptime = (double)uptimes.m128_f32[3];
	}

#else

	if (isPitchModulationActive())
	{
		voicePitchValues += startSample;

		while (--numSamples >= 0)
		{
			int index = (int)voiceUptime;

			float v1 = sinTable[index & 2047];
			float v2 = sinTable[(index + 1) & 2047];

			const float alpha = float(voiceUptime) - (float)index;
			const float invAlpha = 1.0f - alpha;

			const float currentSample = invAlpha * v1 + alpha * v2;

			*leftValues++ = currentSample;

			const double thisPitchValue = *voicePitchValues++;

			voiceUptime += (uptimeDelta * thisPitchValue);

		}
	}
	else
	{
		while (numSamples > 0)
		{
			for (int i = 0; i < 4; i++)
			{
				int index = (int)voiceUptime;

				float v1 = sinTable[index & 2047];
				float v2 = sinTable[(index + 1) & 2047];

				const float alpha = float(voiceUptime) - (float)index;
				const float invAlpha = 1.0f - alpha;

				const float currentSample = invAlpha * v1 + alpha * v2;

				*leftValues++ = currentSample;

				voiceUptime += uptimeDelta;
			}

			numSamples -= 4;
		}
	}

	

#endif

	if (saturation != 0.0f)
	{
		if (saturation == 1.0f) saturation = 0.99f; // 1.0f makes it silent, so this is the best bugfix in the world...

		const float saturationAmount = 2.0f * saturation / (1.0f - saturation);

		// Once from the top...

		numSamples = samplesToCopy;
		startSample = startIndex;

		leftValues = voiceBuffer.getWritePointer(0, startSample);

		for (int i = 0; i < numSamples; i++)
		{
			const float currentSample = leftValues[i];
			const float saturatedSample = (1.0f + saturationAmount) * currentSample / (1.0f + saturationAmount * fabsf(currentSample));

			leftValues[i] = saturatedSample;
		}
	}

	FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
}
