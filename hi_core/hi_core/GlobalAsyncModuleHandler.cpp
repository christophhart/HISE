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
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


void MainController::GlobalAsyncModuleHandler::removeAsync(Processor* p, const SafeFunctionCall::Function& removeFunction)
{
	bool synchronous = p->getMainController()->isBeingDeleted() || p->getMainController()->isFlakyThreadingAllowed();

	if (removeFunction)
	{
		auto f = [removeFunction](Processor* p)
		{
			LockHelpers::freeToGo(p->getMainController());

			auto result = removeFunction(p);

			p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Delete);

			return result;
		};

		if (synchronous)
			f(p);
		else
			mc->getKillStateHandler().killVoicesAndCall(p, f, KillStateHandler::SampleLoadingThread);
	}
	else
	{
		p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Delete);
	}
}

void MainController::GlobalAsyncModuleHandler::addAsync(Processor* p, const SafeFunctionCall::Function& addFunction)
{
	auto f = [addFunction](Processor* p)
	{
		auto result = addFunction(p);

		p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Add);

		return result;
	};

	if (mc->getKillStateHandler().getCurrentThread() == KillStateHandler::ScriptingThread)
	{
		LockHelpers::freeToGo(mc);

		f(p);
	}
	else
	{
		mc->getKillStateHandler().killVoicesAndCall(p, f, KillStateHandler::SampleLoadingThread);
	}

	
}


void MainController::GlobalAsyncModuleHandler::addPendingUIJob(Processor* p, What what)
{
	bool synchronous = p->getMainController()->isBeingDeleted();

	if (what == Add)
	{
		auto f = [](Dispatchable* obj)
		{
			auto p = static_cast<Processor*>(obj);
			auto parent = p->getParentProcessor(false, false);

			if (parent != nullptr)
				parent->sendRebuildMessage(true);

			return Dispatchable::Status::OK;
		};

		if (synchronous)
			f(p);
		else
			mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, f);
	}
	else
	{
		auto f = [](Dispatchable* obj)
		{
			auto p = static_cast<Processor*>(obj);
			p->sendDeleteMessage();

			auto parent = p->getParentProcessor(false, false);

			if (parent != nullptr)
				parent->sendRebuildMessage(true);

			delete p;

			return Dispatchable::Status::OK;
		};

		if (synchronous)
			f(p);
		else
		{
			p->setIsWaitingForDeletion();
			mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, f);
		}
	}
}

MainController::ProcessorChangeHandler::ProcessorChangeHandler(MainController* mc_):
	mc(mc_)
{}

MainController::ProcessorChangeHandler::~ProcessorChangeHandler()
{
	listeners.clear();
}

MainController::ProcessorChangeHandler::Listener::~Listener()
{
	masterReference.clear();
}

void MainController::ProcessorChangeHandler::sendProcessorChangeMessage(Processor* changedProcessor, EventType type,
	bool synchronous)
{
	tempProcessor = changedProcessor;
	tempType = type;

	if (synchronous)
		handleAsyncUpdate();
	else
		triggerAsyncUpdate();
}

void MainController::ProcessorChangeHandler::handleAsyncUpdate()
{
	if (tempProcessor == nullptr)
		return;

	{
		ScopedLock sl(listeners.getLock());

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
				listeners[i]->moduleListChanged(tempProcessor, tempType);
			else
				listeners.remove(i--);
		}
	}
			

	tempProcessor = nullptr;
	tempType = EventType::numEventTypes;
}

void MainController::ProcessorChangeHandler::addProcessorChangeListener(Listener* newListener)
{
	listeners.addIfNotAlreadyThere(newListener);
}

void MainController::ProcessorChangeHandler::removeProcessorChangeListener(Listener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}
} // namespace hise
