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



namespace hise {
namespace dispatch {	
using namespace juce;

LoggerBase::Ptr ObjectUnderGlobalManager::getCurrentLogger()
{
	return getGlobalManager()->currentLogger;
}

String DebugLogger::toString(Event e)
{
	String s;

	auto c = e.getEventCategory();

	switch(c)
	{
	case Category::AllCategories: s << "global   "; break;
	case Category::ManagerEvent:  s << " manager "; break;
	case Category::SourceEvent:   s << "  source "; break;
	case Category::SlotEvent:	  s << "   slot  "; break;
	}

	auto a = e.getEventActionType();

	switch(a)
	{
	case EventAction::Nothing:		s << "empty   "; break;
	case EventAction::Change:		s << "change  "; break;
	case EventAction::Add:			s << "add     "; break;
	case EventAction::FlushAsync:	s << "ui      "; break;
	case EventAction::Remove:		s << "rem     "; break;
	case EventAction::ListenerAdd:	s << "ls_add  "; break; 
	case EventAction::ListenerRem:	s << "ls_rem  "; break; 
	case EventAction::Defer:		s << "<defer  "; break; 
	case EventAction::PostDefer:	s << "defer>  "; break; 
	case EventAction::PreAll:		s << "<all    "; break; 
	case EventAction::AllChange:	s << "all>    "; break;
	case EventAction::AllEvents: break;
	}

	s << String(e.eventManager) << " | " << String(e.source) << " | " << String(e.slot);
	return s;
}

void DebugLogger::logListenerEvent(NotificationType n, void* obj, uint8* values, size_t numValues)
{
	String s;

	if(n == sendNotificationSync)
		s << "** sync  ";
	else
		s << "** async ";

	s << String::toHexString(reinterpret_cast<uint64_t>(obj));

	s << " [ ";

	for(int i = 0; i < numValues; i++)
	{
		if(values[i] != 0)
		{
			s << String(i);

			if(i != (numValues-1))
				s << ", ";
		}
	}

	s << " ]";

	DBG(s);
}

String DebugLogger::getByteString(manager_int b)
{
	if(b < 1024)
		return String(b) + (b > 1 ? " bytes" : " byte");
	else
		return String(b / 1024) + "kB";
}

void DebugLogger::logReallocateEvent(manager_int before, manager_int now)
{
	String s;

	s << "realloc " << getByteString(before) << " -> " << getByteString(now);
	DBG(s);
}

inline void DebugLogger::logEvent(const Event& e)
{
	bool isDeferred = false;

	if(defferedManagers.contains(e.eventManager) &&
		deferredSources.contains(e.source) )
	{
		isDeferred = true;
	}

	if(e.isAnyDelayEvent())
	{
		if(e.isAnyPreEvent())
		{
			defferedManagers.add(e.eventManager);
			deferredSources.add(e.source);
		}
		else
		{
			defferedManagers.remove(e.eventManager);
			deferredSources.remove(e.source);
		}

		isDeferred = false;
	}
		
	auto s = toString(e);

	if(isDeferred)
	DBG("// " + s + " ( ignored )");
	else
	DBG(s);
}









#if 0

	TestClass::TestClass()
{
		
}

struct MyListener: public Listener
{
	MyListener(Processor* p):
	  Listener(&p->source)
	{
		addListener();
	};

	~MyListener()
	{
		removeListener();
	}
};

void TestClass::run()
{
	ThreadManager tm;

	GlobalManager gm(tm);
	gm.setLogger(new DebugLogger());

	TypedSourceManager<Processor> pm(&gm);

	static constexpr int NumElements = 7600;
	int numIterations = 90000;

	CriticalSection processorLock, listenerLock;
	UnorderedStack<Processor*, NumElements> processors;
	UnorderedStack<MyListener*, NumElements> myListeners;

	auto p1 = new Processor(&pm);

	MyListener l(p1);

	

	Random r;

	int numOps = 0;

	auto addProcessor = [&](Processor* newProcessor)
	{
		ScopedLock sl(processorLock);
		processors.insertWithoutSearch(newProcessor);
		numOps++;
	};



	auto removeProcessor = [&]()
	{
		Processor* x;
		
		{
			ScopedLock sl(processorLock);

			if(processors.isEmpty())
				return;

			auto index = r.nextInt(processors.size());
			x = processors[index];
			jassert(x != nullptr);
			jassert(processors.removeElement(index));
			numOps++;
		}

		delete x;
	};

	auto addListener = [&](MyListener* l)
	{
		ScopedLock sl(processorLock);
		myListeners.insertWithoutSearch(l);
		numOps++;
	};

	

	auto removeListener = [&]()
	{
		MyListener* x;
		{
			ScopedLock sl(processorLock);

			if(myListeners.isEmpty())
				return;

			auto index = r.nextInt(myListeners.size());

			r.nextInt(myListeners.size());
			x = myListeners[index];
			jassert(x != nullptr);
			jassert(myListeners.removeElement(index));
			numOps++;
		}
		
		delete x;
	};

	auto setProcessorValue = [&]()
	{
		Processor* x = nullptr;
		{
			ScopedLock sl(processorLock);
			if(processors.isEmpty())
				return;

			x = processors[r.nextInt(processors.size())];
			numOps++;
		}
		
		if(x != nullptr)
			x->setAttribute(r.nextInt(16), r.nextFloat());
	};

#define LOG_EVENT(x) DBG(x);

	int finished = 0;

	auto funky = [&]()
	{
		PerformanceCounter pc("start", 1);

		for(int i = 0; i < numIterations; i++)
		{
			enum Action
			{
				AddProcessor,
				RemoveProcessor,
				AddListener,
				RemoveListener,
				SendMessage,
				numActions
			};

			auto thisAction = r.nextInt(numActions);

			if(thisAction != SendMessage)
				thisAction = r.nextInt(numActions);
					
			if(thisAction != SendMessage)
				thisAction = r.nextInt(numActions);

			switch(thisAction)
			{
			case AddProcessor:
				{
					addProcessor(new Processor(&pm));

					LOG_EVENT("Add processor");
					break;
				}
				
			case AddListener:
				if(processors.isEmpty())
					break;

				addListener(new MyListener(processors[r.nextInt(processors.size())]));
				LOG_EVENT("Add listener to processor");
				break;
			case RemoveListener:
				if(myListeners.isEmpty())
					break;

				removeListener();
				LOG_EVENT("Remove listener");
				break;
			case RemoveProcessor:
				if(processors.isEmpty())
					break;;
				removeProcessor();
				LOG_EVENT("Remove Processor");
				break;
			case SendMessage:
				if(processors.isEmpty())
					break;;

				setProcessorValue();
				
				LOG_EVENT("Send message"); break;
				break;
			}
		};

		pc.stop();

		finished++;
	};


	auto before = Time::getMillisecondCounter();

	#if 0
	struct Funky
	{
		void doSomething(int& z)
		{
			z += this->x + this->y;
			x = Random::getSystemRandom().nextInt(1243);
		}

		int x = 90;
		int y = 10;
	};

	Array<Funky> funkyItems;


	SimpleReadWriteLock testLock;

	auto funkyReader = [&]()
	{
		int num = 0;
		int numSkipped = 0;
		while(num++ < 1000000)
		{
			if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(testLock))
			{
				int x = 0;

				for(auto& f: funkyItems)
					f.doSomething(x);
			}
			else
			{
				numSkipped++;
			}
		}

		finished++;
	};

	auto funkyWriter = [&]()
	{
		int num = 0;

		while(num++ < 100000)
		{
			SimpleReadWriteLock::ScopedMultiWriteLock sl(testLock, true);

			funkyItems.clear();

			for(int i = 0; i < Random::getSystemRandom().nextInt(12333); i++)
				funkyItems.add({});
		}

		Thread::getCurrentThread()->wait(30);

		finished++;
	};
#endif

	Thread::launch(funky);
	Thread::launch(funky);
	//Thread::launch(funky);
	//Thread::launch(funky);
	

	/*
	Thread::launch([&]()
	{
		while(!finished)
		{
			{
				MessageManagerLock mm;
				auto e = gm.createEvent(EventAction::FlushAsync);
				gm.sendMessage(e);
			}
			
			Thread::getCurrentThread()->wait(30);
		}
	});
	*/

	while(finished < 4)
		;
	
	auto now = (double)(Time::getMillisecondCounter() - before) / 1000.0;
	numOps++;
	


	

	jassertfalse;
}
#endif

} // dispatch
} // hise