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
namespace dummy {
using namespace juce;

BuilderFactory::BuilderFactory(MainController* mc):
	ControlledObject(mc)
{}

Action::Builder* BuilderFactory::createBuilder(const Identifier& id)
{
	for(auto& i: items)
	{
		if(i.id == id)
			return i.f(getMainController());
	}

	return nullptr;
}

Action::List BuilderFactory::create(const var& jsonData)
{
	Action::List list;

	jassert(jsonData.isArray());

	if(auto ar = jsonData.getArray())
	{
		for(auto& nv: *ar)
		{
			auto t = Identifier(nv[ActionIds::type].toString());
			if(auto b = createBuilder(t))
			{
				list.addArray(b->createActions(nv[ActionIds::events]));
			}
			else
			{
				throw String("Can't find factory for type: " + t.toString());
			}
		}
	}

	return list;
}

void BuilderFactory::registerItem(const Identifier& id, const CreateFunction& f)
{
	items.add({id, f});
}

MainController::MainController():
	root(&updater),
#if ENABLE_QUEUE_AND_LOGGER
	logger(root, 1024),
#endif
	processorHandler(new library::ProcessorHandler(root)),
    factory(this)
{
	//root.setLogger(&logger);

	audioThread = threads.add(new AudioThread(this));
	uiSimThread = threads.add(new UISimulator(this));

	factory.registerItem(ActionIds::processor, [](MainController* mc)
	{
		return new Processor::Builder(mc);
	});
	factory.registerItem(ActionIds::randomevents, [](MainController* mc)
	{
		return new RandomActionBuilder(mc);
	});
	factory.registerItem(ActionIds::processor_listener, [](MainController* mc)
	{
		return new ProcessorListener::Builder(mc);
	});
}

MainController::~MainController()
{
#if ENABLE_QUEUE_AND_LOGGER
	logger.flush();
#endif
	root.setLogger(nullptr);
	processorHandler = nullptr;
	allActions.clear();
	threads.clear();

}

void MainController::setActions(const var& obj)
{
	allActions = factory.create(obj);

	for(auto a: allActions)
	{
		if(auto th = getThread(a->getPreferredThread()))
		{
			th->addAction(a);
		}
	}
}

void MainController::prepareToPlay(const var& obj)
{
    totalDurationMilliseconds = (int)obj[ActionIds::duration];

    if(totalDurationMilliseconds < 10)
        totalDurationMilliseconds = 1000;

    dynamic_cast<AudioThread*>(audioThread)->prepareToPlay((double)obj[ActionIds::samplerate], (int)obj[ActionIds::buffersize]);
}


void MainController::start()
{
	Random r;
    
	updater.startTimer(30);

	for(auto& t: threads)
		t->startSimulation();

	started = true;
}

bool MainController::isFinished() const
{
	return started && !audioThread->isThreadRunning();
}

JSONBuilder::Data JSONBuilder::Data::withThread(ThreadType nt) const
{
	auto copy = *this;
	copy.t = nt;
	return copy;
}

JSONBuilder::Data JSONBuilder::Data::withDispatchType(DispatchType nd) const
{
	auto copy = *this;
	copy.n = nd;
	return copy;
}

JSONBuilder::Data JSONBuilder::Data::withSingleTimestampWithin(double proportion) const
{
	auto s = lifetime.convertFrom0to1(proportion);

	Data copy = *this;
	copy.lifetime = { s, 1.0};
	return copy;
}

JSONBuilder::Data JSONBuilder::Data::withTimestampRangeWithin(double start, double end) const
{
	auto s = lifetime.convertFrom0to1(start);
	auto e = lifetime.convertFrom0to1(end);

	Data copy = *this;
	copy.lifetime = { s, e};
	return copy;
}

JSONBuilder::JSONBuilder(const String& name, int durationMilliseconds)
{
	obj = new DynamicObject();
	obj->setProperty(ActionIds::description, name);
	obj->setProperty(ActionIds::duration, durationMilliseconds);

	prepareToPlay(44100.0, 512);
}

void JSONBuilder::prepareToPlay(double sampleRate, int bufferSize)
{
	obj->setProperty(ActionIds::samplerate, sampleRate);
	obj->setProperty(ActionIds::buffersize, bufferSize);

	Array<var> no;
	obj->setProperty(ActionIds::objects, var(no));
}

DynamicObject::Ptr JSONBuilder::addListener(const Data& d, const Array<int> attributes, bool addBypassListener)
{
	auto addEvent = addObjectInternal(ActionIds::processor_listener, d.lifetime.getRange(), d.t);
	addEvent->setProperty(ActionIds::source, d.id);
	auto ne = var(addEvent.get());

	Array<var> va;

	for(auto& v: attributes)
		va.add(v);

	addEvent->setProperty(ActionIds::attributes, var(va));
	addEvent->setProperty(ActionIds::bypassed, addBypassListener);
	Helpers::writeDispatchString(d.n, ne);
	return addEvent;
}

DynamicObject::Ptr JSONBuilder::addProcessor(const Data& d, int numParameters)
{
	auto addEvent = addObjectInternal(ActionIds::processor, d.lifetime.getRange(), d.t);
	addEvent->setProperty(ActionIds::num_parameters, numParameters);
	addEvent->setProperty(ActionIds::id, d.id);
	return addEvent;
}

DynamicObject::Ptr JSONBuilder::addProcessorEvent(const Data& d, const Identifier& eventType)
{
	return addEventInternal(ActionIds::processor, ActionIds::id, d.id, eventType, d.lifetime.getRange().getStart(), d.t, d.n);
}

DynamicObject::Ptr JSONBuilder::addListenerEvent(const Data& d, const Identifier& eventType)
{
	if(eventType == ActionTypes::count)
		bumpCheckCounter();

	return addEventInternal(ActionIds::processor_listener, ActionIds::source, d.id, eventType, d.lifetime.getRange().getStart(), d.t, d.n);
}

void JSONBuilder::dump()
{
	DBG(JSON::toString(getJSON(), false));
}

var JSONBuilder::getJSON()
{
	return var(obj.get());
}

DynamicObject::Ptr JSONBuilder::addEventInternal(const Identifier& typeId, const Identifier& idId,
	const String& processorId, const Identifier& eventType, double normalisedTimestampWithinLifetime, ThreadType t,
	DispatchType n)
{
	if(auto l = getEventsForType(typeId, idId, processorId))
	{
		DynamicObject::Ptr newEvent = new DynamicObject();
		newEvent->setProperty(ActionIds::type, eventType.toString());
		newEvent->setProperty(ActionIds::thread, Helpers::getThreadName(t).toString());

		auto ts = normalisedTimestampWithinLifetime;

		newEvent->setProperty(ActionIds::ts, ts);

		var ne(newEvent.get());
		Helpers::writeDispatchString(n, ne);

		struct TimestampSorter
		{
			TimestampSorter(float totalDuration_):
				totalDuration(totalDuration_)
			{};

			int compareElements(const var& v1, const var& v2)
			{
				auto t1 = Helpers::normalisedToTimestamp((float)v1[ActionIds::ts], totalDuration);
				auto t2 = Helpers::normalisedToTimestamp((float)v2[ActionIds::ts], totalDuration);

				if(t1 < t2)
					return -1;
				if(t1 > t2)
					return 1;

				return 0;
			}

			const float totalDuration;
		};

		TimestampSorter sorter(obj->getProperty(ActionIds::duration));
		const_cast<Array<var>*>(l)->addSorted(sorter, ne);

		return newEvent;
	}

	return nullptr;
}

DynamicObject::Ptr JSONBuilder::addObjectInternal(const Identifier& type, Range<double> lifetime,
	ThreadType constructionThread)
{
	auto p = new DynamicObject();
	p->setProperty(ActionIds::type, type.toString());

	Array<var> events;
	auto threadName = Helpers::getThreadName(constructionThread).toString();

	DynamicObject::Ptr addEvent = new DynamicObject();
	addEvent->setProperty(ActionIds::type, ActionTypes::add.toString());
	addEvent->setProperty(ActionIds::ts, lifetime.getStart());
	addEvent->setProperty(ActionIds::thread, threadName);
		
	events.add(addEvent.get());

	auto remEvent = new DynamicObject();
	remEvent->setProperty(ActionIds::type, ActionTypes::rem.toString());
	remEvent->setProperty(ActionIds::ts, lifetime.getEnd());
	remEvent->setProperty(ActionIds::thread, threadName);
	events.add(remEvent);

	p->setProperty(ActionIds::events, var(events));
	getObjectArray()->add(p);
	return addEvent;
}

const Array<var>* JSONBuilder::getEventsForType(const Identifier& typeId, const Identifier& idId, const String& processorId) const
{
	for(const auto& o: *getObjectArray())
	{
		if(o[ActionIds::type].toString() != typeId.toString())
			continue;

		auto addEvent = o[ActionIds::events][0];
		jassert(addEvent.isObject());
		auto pid = addEvent[idId].toString();

		if(pid == processorId)
		{
			return o[ActionIds::events].getArray();
		}
	}

	return nullptr;
}
} // dummy
} // dispatch
} // hise