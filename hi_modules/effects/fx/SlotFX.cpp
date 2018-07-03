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
		if (!w->isBypassed())
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
		if (!w->isBypassed())
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
		if (!w->isBypassed())
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
		if (!w->isBypassed())
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
		if (!w->isBypassed())
		{
			wrappedEffect->renderAllChains(0, buffer.getNumSamples());
			wrappedEffect->renderWholeBuffer(buffer);
		}
	}
}

bool SlotFX::setEffect(const String& typeName, bool synchronously)
{
	int index = effectList.indexOf(typeName);

	if (currentIndex == index)
		return true;

	if (index != -1)
	{
		ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);

		f->setConstrainer(new Constrainer());

		currentIndex = index;


		if (wrappedEffect != nullptr)
		{
			if (synchronously)
			{
				wrappedEffect->sendDeleteMessage();

			}
			else
			{
				ScopedLock sl(getMainController()->getLock());

				auto pendingDeleteEffect = wrappedEffect.release();

				auto df = [pendingDeleteEffect, this]()
				{
					pendingDeleteEffect->sendDeleteMessage();


					auto p = this->wrappedEffect.get();

					if (p != nullptr)
					{
						for (int i = 0; i < p->getNumInternalChains(); i++)
						{
							dynamic_cast<ModulatorChain*>(p->getChildProcessor(i))->setColour(p->getColour());
						}

						this->sendRebuildMessage(true);
					}

					ScopedLock sl(getMainController()->getLock());

					delete pendingDeleteEffect;
				};

				new DelayedFunctionCaller(df, 100);
			}

			
		}
			



		auto p = dynamic_cast<MasterEffectProcessor*>(f->createProcessor(f->getProcessorTypeIndex(typeName), typeName));

		if (p == nullptr)
		{
			reset();
			return true;
		}

		if (p != nullptr && getSampleRate() > 0)
			p->prepareToPlay(getSampleRate(), getLargestBlockSize());

        bool thisIsScriptFX = false;
        
        if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(p))
        {
            thisIsScriptFX = true;
            sp->compileScript();
        }
        
		if(p != nullptr)
		{
			auto newId = getId() + "_" + p->getId();

			p->setId(newId);

			ScopedLock sl(getMainController()->getLock());
            
            p->setIsOnAir(true);
            
			wrappedEffect = p;
            
            hasScriptFX = thisIsScriptFX;

			isClear = wrappedEffect == nullptr || dynamic_cast<EmptyFX*>(wrappedEffect.get()) != nullptr;
		}

		
		

		if (synchronously)
			sendRebuildMessage(true);

		return true;
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
