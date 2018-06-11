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

MainController::GlobalAsyncModuleHandler::JobData::JobData(Processor* parent_, Processor* processor_, What what_) :
	what(what_),
	parent(parent_),
	processorToDelete(what_ == What::Delete ? processor_ : nullptr),
	processorToAdd(what_ == What::Add ? processor_ : nullptr)
{

}

MainController::GlobalAsyncModuleHandler::JobData::JobData() :
	what(numWhat),
	parent(nullptr),
	processorToDelete(nullptr),
	processorToAdd(nullptr)
{

}

bool MainController::GlobalAsyncModuleHandler::JobData::doit()
{
    // Use the parent for starters
    Processor* pToUse = parent.get();
    
    if(pToUse == nullptr)
        pToUse = processorToDelete.get();
    
    if(pToUse == nullptr)
        pToUse = processorToAdd.get();
    
    if(pToUse == nullptr)
        return true;
    
	if (auto lock = PresetLoadLock(pToUse->getMainController()))
	{
		if (what == What::Add)
		{
			if (parent.get() != nullptr)
				parent->sendRebuildMessage(true);
		}
		else
		{
			processorToDelete->sendDeleteMessage();

			if (parent.get() != nullptr)
				parent->sendRebuildMessage(true);

			delete processorToDelete;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void MainController::GlobalAsyncModuleHandler::addPendingUIJob(Processor* parent, Processor* p, JobData::What what)
{
	pendingJobs.push(JobData(parent, p, what));

	triggerAsyncUpdate();
}

void MainController::GlobalAsyncModuleHandler::handleAsyncUpdate()
{
	JobData d;

	while (pendingJobs.pop(d))
	{
		if (!d.doit())
		{
			lockedJobs.push(std::move(d));
		}
	}

	if (!lockedJobs.isEmpty())
		startTimer(200);
}

void MainController::GlobalAsyncModuleHandler::timerCallback()
{
	JobData d;

	LockfreeQueue<JobData> unfinishedJobs(1024);

	while (lockedJobs.pop(d))
	{
		if (!d.doit())
		{
			unfinishedJobs.push(std::move(d));
		}
	}

	while (unfinishedJobs.pop(d))
	{
		lockedJobs.push(std::move(d));
	}
	
	if (lockedJobs.isEmpty())
	{
		stopTimer();
	}
}



void MainController::GlobalAsyncModuleHandler::removeAsync(Processor* p, Component* /*rootWindow*/)
{
	if (auto parent = ProcessorHelpers::findParentProcessor(p, false))
	{
		WeakReference<Processor> processorToDelete = p;

		auto f = [processorToDelete](Processor* p)
		{
			if (processorToDelete == nullptr)
				return true;

			auto c = dynamic_cast<Chain*>(p);

			jassert(c != nullptr);

			if (c == nullptr)
				return true;

			c->getHandler()->remove(processorToDelete, false);
			p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, processorToDelete, JobData::What::Delete);

			return true;
		};

		p->getMainController()->getKillStateHandler().killVoicesAndCall(parent, f, KillStateHandler::SampleLoadingThread);
	}

	

	
}

void MainController::GlobalAsyncModuleHandler::addAsync(Chain* c, Processor* p, Component* /*rootWindow*/, const String& /*type*/, const String& /*id*/, int index)
{

	auto f = [c, index](Processor* p)
	{
		if (c == nullptr)
		{
			delete p; // Rather bad...
			return false;
		}

		if (index >= 0 && index < c->getHandler()->getNumProcessors())
		{
			Processor* sibling = c->getHandler()->getProcessor(index);
			c->getHandler()->add(p, sibling);
		}
		else
			c->getHandler()->add(p, nullptr);

		p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(dynamic_cast<Processor*>(c), p, JobData::What::Add);

		return true;
	};

	p->getMainController()->getKillStateHandler().killVoicesAndCall(p, f, KillStateHandler::SampleLoadingThread);
}


} // namespace hise
