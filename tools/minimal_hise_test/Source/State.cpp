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



namespace hise
{
using namespace juce;

namespace multipage {
using namespace juce;

State::State(const var& obj):
	Thread("Tasks"),
	currentError(Result::ok())
{
	if(auto gs = obj[mpid::GlobalState].getDynamicObject())
		globalState = var(gs->clone());
	else
		globalState = var(new DynamicObject());
}

State::~State()
{
	stopThread(1000);
}

void State::run()
{
	for(int i = 0; i < jobs.size(); i++)
	{
		auto ok = jobs[i]->runJob();
            
		if(ok.failed())
			break;
            
		totalProgress = (double)i / (double)jobs.size();
	}
        
	jobs.clear();
        
	MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(State::onFinish));
}

State::Job::Job(State& rt, const var& obj):
	parent(rt),
	localObj(obj)
{
	parent.addJob(this);
}

State::Job::~Job()
{}

bool State::Job::matches(const var& obj) const
{
	return localObj[mpid::ID] == obj[mpid::ID];
}

double& State::Job::getProgress()
{ return progress; }

State::Job::Ptr State::getJob(const var& obj)
{
	for(auto j: jobs)
	{
		if(j->matches(obj))
			return j;
	}
        
	return nullptr;
}

var State::getGlobalSubState(const Identifier& id)
{
	if(globalState.hasProperty(id))
		return globalState[id];

	var no = new DynamicObject();
	globalState.getDynamicObject()->setProperty(id, no);
	return no;
}

void State::onFinish()
{
	if(currentDialog.get() != nullptr)
	{
		currentDialog->nextButton.setEnabled(currentDialog->currentErrorElement == nullptr);
		currentDialog->prevButton.setEnabled(true);
	}
}

Result State::Job::runJob()
{
	auto ok = run();
            
	if(auto p = parent.currentDialog)
	{
		MessageManager::callAsync([p, ok]()
		{
			p->repaint();
			p->errorComponent.setError(ok);
		});
	}
            
	return ok;
}

void State::addJob(Job::Ptr b, bool addFirst)
{
	if(addFirst)
		jobs.insert(0, b);
	else
		jobs.add(b);
        
	if(!isThreadRunning())
	{
		if(currentDialog != nullptr)
		{
			currentDialog->currentErrorElement = nullptr;
			currentDialog->repaint();
			currentDialog->errorComponent.setError(Result::ok());
			currentDialog->nextButton.setEnabled(false);
			currentDialog->prevButton.setEnabled(false);
		}
            
		startThread(6);
	}
}


}
}