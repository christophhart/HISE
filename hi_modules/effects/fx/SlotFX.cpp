/*
  ==============================================================================

    SlotFX.cpp
    Created: 28 Jun 2017 2:51:50pm
    Author:  Christoph

  ==============================================================================
*/

namespace hise { using namespace juce;

SlotFX::SlotFX(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid)
{
	finaliseModChains();

	createList();

	reset();
}

ProcessorEditorBody * SlotFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new SlotFXEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif

	
}

void SlotFX::handleHiseEvent(const HiseEvent &m)
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->handleHiseEvent(m);
		}
	}
}

void SlotFX::startMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->startMonophonicVoice();
		}
	}
}

void SlotFX::stopMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->stopMonophonicVoice();
		}
	}
}

void SlotFX::resetMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->resetMonophonicVoice();
		}
	}
}

void SlotFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			wrappedEffect->renderAllChains(0, buffer.getNumSamples());
			wrappedEffect->renderWholeBuffer(buffer);
		}
	}
}

hise::MasterEffectProcessor* SlotFX::getCurrentEffect()
{
	if (wrappedEffect != nullptr)
		return wrappedEffect.get();


	jassertfalse;
	return nullptr;
}

bool SlotFX::setEffect(const String& typeName, bool synchronously)
{
	LockHelpers::freeToGo(getMainController());

	int index = effectList.indexOf(typeName);

	if (currentIndex == index)
		return true;

	if (index != -1)
	{
		ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);

		f->setConstrainer(new Constrainer());

		currentIndex = index;

		if (auto p = f->createProcessor(f->getProcessorTypeIndex(typeName), typeName))
		{
			if (getSampleRate() > 0)
				p->prepareToPlay(getSampleRate(), getLargestBlockSize());

			p->setParentProcessor(this);
			auto newId = getId() + "_" + p->getId();

			p->setId(newId);

			ScopedPointer<MasterEffectProcessor> pendingDelete;

			if (wrappedEffect != nullptr)
			{
				LOCK_PROCESSING_CHAIN(this);
				wrappedEffect.swapWith(pendingDelete);
			}

			if (pendingDelete != nullptr)
			{
				getMainController()->getGlobalAsyncModuleHandler().removeAsync(pendingDelete.release(), ProcessorFunction());
			}

			{
				LOCK_PROCESSING_CHAIN(this);

				wrappedEffect = dynamic_cast<MasterEffectProcessor*>(p);
				wrappedEffect->setIsOnAir(isOnAir());
				wrappedEffect->setKillBuffer(*(this->killBuffer));
				isClear = wrappedEffect == nullptr || dynamic_cast<EmptyFX*>(wrappedEffect.get()) != nullptr;;
			}

			if (auto sp = dynamic_cast<JavascriptProcessor*>(wrappedEffect.get()))
			{
				hasScriptFX = true;
				sp->compileScript();
			}

			return true;

#if CLEANUP_LOCK
			MasterEffectProcessor* fxToBeDeleted = nullptr;

			if (processorWaitingToBeInserted != nullptr)
			{
				// Nothing to do here...
				auto removeFunction = [](Processor*)
				{
					return SafeFunctionCall::OK;
				};

				getMainController()->getGlobalAsyncModuleHandler().removeAsync(processorWaitingToBeInserted.release(), removeFunction);
			}

			processorWaitingToBeInserted = dynamic_cast<MasterEffectProcessor*>(p);

			

			if (wrappedEffect != nullptr)
			{
				ScopedLock sl(getMainController()->getLock());

				fxToBeDeleted = wrappedEffect.release();
				wrappedEffect = nullptr;
				fxToBeDeleted->setIsOnAir(false);
			}

			processorWaitingToBeInserted->setParentProcessor(this);
			auto newId = getId() + "_" + processorWaitingToBeInserted->getId();

			processorWaitingToBeInserted->setId(newId);

			if (fxToBeDeleted)
			{
				getMainController()->getGlobalAsyncModuleHandler().removeAsync(fxToBeDeleted, ProcessorFunction());
			}

			auto addFunction = [this](Processor* p)
			{
				jassert(p == this->processorWaitingToBeInserted.get());
				jassert(wrappedEffect == nullptr);

				auto newFX = dynamic_cast<MasterEffectProcessor*>(p);

				newFX->setKillBuffer(*(this->killBuffer));

				if (getSampleRate() > 0)
					newFX->prepareToPlay(getSampleRate(), getLargestBlockSize());

				bool thisIsScriptFX = false;

				if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(p))
				{
					thisIsScriptFX = true;

					auto f = [](Processor* p)
					{
						dynamic_cast<JavascriptProcessor*>(p)->compileScript();
						return SafeFunctionCall::OK;
					};
					
					getMainController()->callFunctionWithGlobalLock(p, f, GlobalLock::Reason::ScriptCompilation);
					
				}

				if (isOnAir())
				{
					auto chain = dynamic_cast<EffectProcessorChain*>(getParentProcessor(false));

					if (chain->hasTailingMasterEffects())
					{
						newFX->setSoftBypass(true, false);
					}
				}

				{
					const bool newFXIsEmpty = processorWaitingToBeInserted == nullptr || dynamic_cast<EmptyFX*>(processorWaitingToBeInserted.get()) != nullptr;;

					ScopedLock sl(getMainController()->getLock());

					p->setIsOnAir(true);
					wrappedEffect.swapWith(processorWaitingToBeInserted);
					jassert(processorWaitingToBeInserted == nullptr);

					hasScriptFX = thisIsScriptFX;
					isClear = newFXIsEmpty;
				}

				return SafeFunctionCall::OK;
			};

			if (synchronously)
			{
				addFunction(p);
			}
			else
			{
				getMainController()->getGlobalAsyncModuleHandler().addAsync(p, addFunction);
			}

			return true;
#endif
		}
		else
		{
			// Should have been catched before...
			jassertfalse;

			reset();
			return true;
		}
	}
	else
	{
		jassertfalse;
		reset();
		return false;
	}
}

void SlotFX::createList()
{
	ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);

	f->setConstrainer(new Constrainer());

	auto l = f->getAllowedTypes();

	for (int i = 0; i < l.size(); i++)
	{
		effectList.add(l[i].type.toString());
	}

	f = nullptr;
}

} // namespace hise
