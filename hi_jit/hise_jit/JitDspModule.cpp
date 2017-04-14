


HiseJITDspModule::HiseJITDspModule(const HiseJITCompiler* compiler)
{
	scope = compiler->compileAndReturnScope();

	static const Identifier proc("process");
	static const Identifier prep("prepareToPlay");
	static const Identifier init_("init");

	compiledOk = compiler->wasCompiledOK();

	if (compiledOk)
	{
		pf = scope->getCompiledFunction<float, float>(proc);
		initf = scope->getCompiledFunction<void>(init_);
		pp = scope->getCompiledFunction<void, double, int>(prep);

		allFunctionsDefined = pf != nullptr && pp != nullptr && initf != nullptr;
	}
}

void HiseJITDspModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (allOK())
	{
		pp(sampleRate, samplesPerBlock);
	}
}

void HiseJITDspModule::init()
{
	if (allOK())
	{
		initf();
	}
}

HiseJITScope* HiseJITDspModule::getScope()
{
	return scope;
}

const HiseJITScope* HiseJITDspModule::getScope() const
{
	return scope;
}

void HiseJITDspModule::processBlock(float* data, int numSamples)
{

	if (allOK())
	{
		for (int i = 0; i < numSamples; i++)
		{
			data[i] = pf(data[i]);
		}

		if (overFlowCheckEnabled)
		{
			overflowIndex = -1;

			for (int i = 0; i < scope->getNumGlobalVariables(); i++)
			{
				overflowIndex = jmax<int>(overflowIndex, scope->isBufferOverflow(i));
				if (overflowIndex != -1)
				{
					throw String("Buffer overflow for " + scope->getGlobalVariableName(i) + " at index " + String(overflowIndex));
				}
			}
		}
	}

}

bool HiseJITDspModule::allOK() const
{
	return compiledOk && allFunctionsDefined;
}


void HiseJITDspModule::enableOverflowCheck(bool shouldCheckForOverflow)
{
	overFlowCheckEnabled = shouldCheckForOverflow;
	overflowIndex = -1;
}