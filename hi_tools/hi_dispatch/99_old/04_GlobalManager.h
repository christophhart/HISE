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

#pragma once
#include "04_GlobalManager.h"
#include "hi_tools/hi_tools/MiscToolClasses.h"
#include "hi_tools/hi_tools/MiscToolClasses.h"

// TODO: add setCurrentListener with enabled / disabled
// TODO: send value change notifications from Sender<T>
// TODO: fix redudnant update index calls
// TODO: optimize tree iterator
// TODO: cleanup

namespace hise {
namespace dispatch {	
using namespace juce;


//struct Processor;



// MainController => Global Manager + Sender<EventSourceManager
// ProcessorHandler => EventSourceManager + Sender<Processor>
// Processor => EventSource

struct EventSourceManager;

// Top level manager contains EventSourceManagers (eg. Processor module updater
struct GlobalManager: public ManagerBase,
					  public Queue<EventSourceManager>::Resolver,
					  public PooledUIUpdater::SimpleTimer
{
	GlobalManager(ThreadManager& tm_, PooledUIUpdater* updater):
	  SimpleTimer(updater),
	  ManagerBase(nullptr, this, Category::AllCategories),
	  tm(tm_)
	{
		start();
	};

	void setLogger(LoggerBase::Ptr l)
	{
		currentLogger = l;
	}

	manager_int getNumEventSources() const override
	{
		return getNumChildren();
	}

	void timerCallback() override
	{
		flushAsyncMessages();
	}

	void rebuildIndexValuesRecursive();

	void sendMessage(Event e)
	{
		performInternal(e);
	}

	manager_int getNumSlots(manager_int index) const override;

	EventSourceManager* resolve(manager_int index) override;

	LoggerBase::Ptr currentLogger;
	ThreadManager& tm;

	void flushAsyncMessages()
	{
		auto e = createEvent(EventAction::FlushAsync);
		sendMessage(e);
	}

private:
	
	friend class ManagerBase;
};


#if 0
struct Processor
{
	Processor(TypedSourceManager<Processor> * manager):
	  source(manager, *this)
	{};

	void setAttribute(int index, float value)
	{
		parameters[index] = value;
		source.sendChangeMessage(index);
	}

	int getNumSlots() const { return 16; }

	float parameters[16];

	TypedSourceManager<Processor>::TypedSource source;
};

struct TestClass
{
	TestClass();

	void run();
};

#endif


} // dispatch
} // hise