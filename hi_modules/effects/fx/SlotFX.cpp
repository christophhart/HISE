/*
  ==============================================================================

    SlotFX.cpp
    Created: 28 Jun 2017 2:51:50pm
    Author:  Christoph

  ==============================================================================
*/

#include "SlotFX.h"

SlotFX::SlotFX(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	updater(this)
{
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

void SlotFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (dynamic_cast<EmptyFX*>(wrappedEffect.get()) == nullptr && !wrappedEffect->isBypassed())
	{
        ScopedLock callbackLock(getMainController()->getLock());
        ScopedReadLock sl(getMainController()->getCompileLock());
        
        
        wrappedEffect->renderAllChains(0, buffer.getNumSamples());
        wrappedEffect->renderWholeBuffer(buffer);
	}
}

bool SlotFX::setEffect(const String& typeName)
{
	int index = effectList.indexOf(typeName);

	if (index != -1)
	{
		ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);

		f->setConstrainer(new Constrainer());

		currentIndex = index;

		if (wrappedEffect != nullptr)
			wrappedEffect->sendDeleteMessage();


		auto p = dynamic_cast<MasterEffectProcessor*>(f->createProcessor(f->getProcessorTypeIndex(typeName), typeName));

		if (p != nullptr && getSampleRate() > 0)
			p->prepareToPlay(getSampleRate(), getBlockSize());

        bool thisIsScriptFX = false;
        
        if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(p))
        {
            thisIsScriptFX = true;
            sp->compileScript();
        }
        
		if(p != nullptr)
		{
            ScopedLock callbackLock(getMainController()->getLock());
            ScopedReadLock sl(getMainController()->getCompileLock());
            
			wrappedEffect = p;
            hasScriptFX = thisIsScriptFX;
		}

        

        
		updater.triggerAsyncUpdate();
        


        
		

		return true;
	}
	else
	{
		//jassertfalse;
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



void SlotFX::Updater::handleAsyncUpdate()
{
    auto p = fx->wrappedEffect.get();

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumInternalChains(); i++)
		{
			dynamic_cast<ModulatorChain*>(p->getChildProcessor(i))->setColour(p->getColour());
		}

		fx->sendRebuildMessage(true);
	}
}
