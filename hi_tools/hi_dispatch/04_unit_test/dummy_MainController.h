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

namespace hise {
namespace dispatch {
namespace dummy {
using namespace juce;

struct BuilderFactory: public ControlledObject
{
    BuilderFactory(MainController* mc);;

    using CreateFunction = std::function<Action::Builder*(MainController*)>;
    Action::Builder* createBuilder(const Identifier& id);
    Action::List create(const var& jsonData);
    void registerItem(const Identifier& id, const CreateFunction& f);

private:

    struct FactoryItem
    {
	    Identifier id;
        CreateFunction f;
    };

    Array<FactoryItem> items;
};

struct MainController
{
    CriticalSection audioLock;

    MainController();
    ~MainController();

    void setActions(const var& obj);
    void start();
	bool isFinished() const;

    void prepareToPlay(const var& obj);

    SimulatedThread* getThread(ThreadType t)
    {
        if(t == ThreadType::AudioThread)
            return audioThread;
        if(t == ThreadType::UIThread)
            return uiSimThread;

        jassertfalse;
        return nullptr;
    }

    int totalDurationMilliseconds = 1000;

    PooledUIUpdater updater;
	RootObject root;

#if ENABLE_QUEUE_AND_LOGGER
    Logger logger;
#endif

	ScopedPointer<library::ProcessorHandler> processorHandler;
    OwnedArray<SimulatedThread> threads;
    SimulatedThread* audioThread;
    SimulatedThread* uiSimThread;
    BuilderFactory factory;
    Action::List allActions;

    int numChecksPerformed = 0;

    UnitTest* currentTest = nullptr;

    bool started = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainController);
};

struct JSONBuilder
{
	struct Data
	{
		String id;
		NormalisableRange<double> lifetime = {0.0, 1.0};
		DispatchType n = DispatchType::sendNotificationAsync;
		ThreadType t = ThreadType::UIThread;

		Data withThread(ThreadType nt) const;
		Data withDispatchType(DispatchType nd) const;
		Data withSingleTimestampWithin(double proportion) const;

		Data withRawTimestamp(double ts) const
		{
            auto copy = *this;
            copy.lifetime = { ts, 1.0};
            return copy;
		}

		Data withTimestampRangeWithin(double start, double end) const;
	};

	JSONBuilder(const String& name, int durationMilliseconds);

	void prepareToPlay(double sampleRate, int bufferSize);
	
	DynamicObject::Ptr addListener(const Data& d, const Array<int> attributes, bool addBypassListener=false);
	DynamicObject::Ptr addProcessor(const Data& d, int numParameters);
	DynamicObject::Ptr addProcessorEvent(const Data& d, const Identifier& eventType);
	
	DynamicObject::Ptr addListenerEvent(const Data& d, const Identifier& eventType);

	void dump();
	var getJSON();

    Data getListenerData(const String& id) const
    {
	    return getDataObject(ActionIds::processor_listener, ActionIds::source, id);
    }

    Data getProcessorData(const String& id) const
    {
	    return getDataObject(ActionIds::processor, ActionIds::id, id);
    }

private:

    Data getDataObject(const Identifier& typeId, const Identifier& idId, const String& id) const
    {
	    if(auto l = getEventsForType(typeId, idId, id))
	    {
            Data d;
            d.id = id;
            d.lifetime = {(double)l->getFirst()[ActionIds::ts], (double)l->getLast()[ActionIds::ts] };
		    d.n = Helpers::getDispatchFromJSON(l->getFirst());

            auto t = l->getFirst()[ActionIds::thread].toString();
            d.t = Helpers::getThreadFromName(HashedCharPtr(t));
            return d;
	    }

        return {};
    }

	void bumpCheckCounter()
	{
        auto v = (int)obj->getProperty(ActionTypes::count);
        v++;
        obj->setProperty(ActionTypes::count, v);
	}

	DynamicObject::Ptr addEventInternal(const Identifier& typeId, const Identifier& idId, const String& processorId, const Identifier& eventType, double normalisedTimestampWithinLifetime, ThreadType t, DispatchType n=sendNotificationAsync);
	DynamicObject::Ptr addObjectInternal(const Identifier& type, Range<double> lifetime, ThreadType constructionThread);

	const Array<var>* getEventsForType(const Identifier& typeId, const Identifier& idId, const String& processorId) const;
	const Array<var>* getObjectArray() const { return obj->getProperty(ActionIds::objects).getArray(); }
    Array<var>* getObjectArray() { return obj->getProperty(ActionIds::objects).getArray(); }

	DynamicObject::Ptr obj;
};


} // dummy
} // dispatch
} // hise