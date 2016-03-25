
ChorusEffect::ChorusEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id)
{
	parameterNames.add("Rate");
	parameterNames.add("Width");
	parameterNames.add("Feedback");
	parameterNames.add("Delay");

	bufpos = 0;
	buffer = new float[BUFMAX];
	buffer2 = new float[BUFMAX];
	phi = fb = fb1 = fb2 = deps = 0.0f;

	parameterRate = 0.30f; 
	parameterDepth = 0.43f;
	parameterMix = 0.47f;  
	parameterFeedback = 0.30f;
	parameterDelay = 1.00f;

	if (buffer) memset(buffer, 0, BUFMAX * sizeof(float));
	if (buffer2) memset(buffer2, 0, BUFMAX * sizeof(float));

}

float ChorusEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Rate:			return parameterRate;
	case Width:			return parameterDepth;
	case Feedback:		return parameterFeedback;
	case Delay:			return parameterDelay;
	default:			jassertfalse; return 1.0f;
	}
}

void ChorusEffect::setInternalAttribute(int parameterIndex, float value)
{
	switch (parameterIndex)
	{
	case Rate:			parameterRate = value; break;
	case Width:			parameterDepth = value; break;
	case Feedback:		parameterFeedback = value; 
						phi = 0.0f; //reset cycle
						break;
	case Delay:			parameterDelay = value; break;
	default:			jassertfalse; break;
	}

	calculateInternalValues();
}


void ChorusEffect::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(Rate, "Rate");
	loadAttribute(Width, "Width");
	loadAttribute(Feedback, "Feedback");
	loadAttribute(Delay, "Delay");
}

ValueTree ChorusEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Rate, "Rate");
	saveAttribute(Width, "Width");
	saveAttribute(Feedback, "Feedback");
	saveAttribute(Delay, "Delay");

	return v;
}

void ChorusEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	tempBuffer = AudioSampleBuffer(2, samplesPerBlock);

	calculateInternalValues();
}

void ChorusEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	FloatVectorOperations::copy(tempBuffer.getWritePointer(0, 0), buffer.getReadPointer(0, startSample), numSamples);
	FloatVectorOperations::copy(tempBuffer.getWritePointer(1, 0), buffer.getReadPointer(1, startSample), numSamples);

	const float *inputL = tempBuffer.getReadPointer(0, 0);
	const float *inputR = tempBuffer.getReadPointer(1, 0);

	const float *inputs[2] = { inputL, inputR };
	
	float *outputL = buffer.getWritePointer(0, startSample);
	float *outputR = buffer.getWritePointer(1, startSample);

	float *outputs[2] = { outputL, outputR };

	processReplacing(inputs, outputs, numSamples);
}

//
//								processReplacing
//
void ChorusEffect::processReplacing(const float** inputs, float** outputs, int sampleFrames)
{
	const float *in1 = inputs[0];
	const float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, f = fb, f1 = fb1, f2 = fb2, ph = phi;
	float ra = rat, de = dep, we = wet, dr = dry, ds = deps, dm = dem;
	int  tmp, tmpi, bp = bufpos;
	float tmpf, dpt;

	--in1;
	--in2;
	--out1;
	--out2;
	while (--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

		ph += ra;
		if (ph > 1.0f) ph -= 2.0f;

		bp--; bp &= 0x7FF;
		*(buffer + bp) = a + f * f1;
		*(buffer2 + bp) = b + f * f2;

		//ds = 0.995f * (ds - de) + de;          //smoothed depth change ...try inc not mult
		dpt = tmpf = dm + de * (1.0f - ph * ph); //delay mod shape
		tmp = int(tmpf);
		tmpf -= tmp;
		tmp = (tmp + bp) & 0x7FF;
		tmpi = (tmp + 1) & 0x7FF;

		f1 = *(buffer + tmp);  //try adding a constant to reduce denormalling
		f2 = *(buffer2 + tmp);
		f1 = tmpf * (*(buffer + tmpi) - f1) + f1; //linear interpolation
		f2 = tmpf * (*(buffer2 + tmpi) - f2) + f2;

		a = a * dr - f1 * we;
		b = b * dr - f2 * we;

		*++out1 = a;
		*++out2 = b;
	}
	if (fabs(f1) > 1.0e-10) { fb1 = f1; fb2 = f2; }
	else fb1 = fb2 = 0.0f; //catch denormals
	phi = ph;
	deps = ds;
	bufpos = bp;
}


void ChorusEffect::calculateInternalValues()
{

	rat = (float)(pow(10.0f, 3.f * parameterRate - 2.f) * 2.f / getSampleRate());
	dep = 2000.0f * parameterDepth * parameterDepth;
	dem = dep - dep * parameterDelay;
	dep -= dem;

	wet = parameterMix;
	dry = 1.f - wet;
	if (parameterRate < 0.01f) { rat = 0.0f; phi = (float)0.0f; }
	fb = 1.9f * parameterFeedback - 0.95f;
}

ProcessorEditorBody *ChorusEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ChorusEditor(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}

