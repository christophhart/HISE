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
using namespace juce;


namespace dummy
{

#define DECLARE_ID(x) static const Identifier x(#x);
namespace ActionIds
{
    // Generic Properties
    DECLARE_ID(description);        // some kind of string for describing the object or test
    DECLARE_ID(id);                 // ID for any object (eg. processors) that is used for connections between objects
    DECLARE_ID(ts);                 // timestamp, either a double between 0...1 or a millisecond value
    DECLARE_ID(duration);           // an artificial duration that the simulated task will take in milliseconds
    DECLARE_ID(notify);             // the notification type for the message / listener, etc. Must be one of ["none", "sync", "async_high", "async_low"];
    DECLARE_ID(thread);             // thread ID, one of the following strings: "audio", "ui", "script", "loading"
    DECLARE_ID(value);              // some kind of value that is set by an event (eg. bypass state)

    // Global Properties
    DECLARE_ID(samplerate);         // the simulated samplerate
    DECLARE_ID(buffersize);         // the simulated buffersize

    // Layout Properties

    DECLARE_ID(objects);            // A list of objects. Must be an array with JSON objects that have a `type` and a `events` property
    DECLARE_ID(type);               // the type of object. This ID will be used to pick the factory that creates the events so it must match
    DECLARE_ID(events);             // a list of actions that a given object will perform. The child

    // Random Event Object
    DECLARE_ID(randomevents);       // the type identifier for a random action that takes a certain amount of time

    // Processor Object
    DECLARE_ID(processor);          // the type identifier for the processor object
    DECLARE_ID(index);              // any kind of integer index (eg attribute number).
    DECLARE_ID(num_parameters);

    // Listener Object
    DECLARE_ID(processor_listener); // the type identidier for a listener object for processor events. Generic attributes:
    DECLARE_ID(source);             // the connection to a source. the source object must exist before and after creation (otherwise the test will fail)
    DECLARE_ID(slots);              // the slots that the listener wants to connect to. Must be a combination of ["attributes", "bypassed", "id"]
    DECLARE_ID(attributes);          // the attribute index that this processor will listen to. Can be either a single number or an array of integer indexes.
    DECLARE_ID(bypassed);
    
    
}

namespace ActionTypes
{
    DECLARE_ID(busywait);       // a random busywait event
    DECLARE_ID(sleep);          // a random sleep event
    DECLARE_ID(add);            // adds the object
    DECLARE_ID(rem);            // removes the object
    DECLARE_ID(count);          // counts the number of events and fails the test at a mismatch
    DECLARE_ID(set_attribute);
    DECLARE_ID(set_bypassed);
}
#undef DECLARE_ID

enum class ThreadType
{
    Undefined,
	AudioThread,
    UIThread,
    numThreadTypes,
    SampleLoadingThread,
    ScriptingThread
};

struct MainController;

struct ControlledObject
{
    ControlledObject(MainController* mc_): mc(mc_) {};
    virtual ~ControlledObject() {};

	const MainController* getMainController() const { return mc; }
	MainController* getMainController() { return mc; }

private:
    MainController* mc;
};

//static constexpr int NumTotalSeconds = 3;
static constexpr int TimeoutMilliseconds = 200;

struct Helpers
{
    enum class ErrorType
    {
        DataError,
	    RuntimeError,
        numErrorTypes
    };

	static HashedCharPtr getThreadName(ThreadType t)
    {
	    switch(t)
	    {
	    case ThreadType::AudioThread:         return "audio";
	    case ThreadType::UIThread:            return "ui";
	    case ThreadType::ScriptingThread:     return "scripting";
	    case ThreadType::SampleLoadingThread: return "loading";
	    case ThreadType::Undefined:           
	    default:                              return "undefined";
	    }
    }

    static void expectOrThrow(bool ok, const String& errorMessage)
    {
        if(!ok)
            throw String(errorMessage);
    }

    template <ThreadType Exclusive=ThreadType::Undefined> static void forEachThread(const std::function<void(ThreadType)>& f)
	{
        if constexpr(Exclusive != ThreadType::Undefined)
        {
	        f(Exclusive);
            return;
        }
        else
        {
	        f(ThreadType::UIThread);
			f(ThreadType::AudioThread);
        }
	}

    template <DispatchType Exclusive=DispatchType::dontSendNotification> static void forEachDispatchType(const std::function<void(DispatchType n)>& f)
	{
        if constexpr (Exclusive != DispatchType::dontSendNotification)
        {
	        f(Exclusive);
        }
        else
        {
	        f(DispatchType::sendNotificationSync);
	        f(DispatchType::sendNotificationAsyncHiPriority);
	        f(DispatchType::sendNotificationAsync);
        }
	}

    static void expectOrThrow(bool ok, ErrorType t, const StringBuilder& b, const String& errorMessage)
	{
		if(!ok)
		{
			String e;
            e << (t == ErrorType::DataError ? "DATA ERROR: " : "RUNTIME ERROR: ");
            e << b.toString() << " - " << errorMessage;
            throw e;
		}
	}

    static ThreadType getThreadFromName(HashedCharPtr c);

    static DispatchType getDispatchFromJSON(const var& obj, DispatchType defaultValue=DispatchType::sendNotificationAsync);

    static String getDispatchName(DispatchType n)
    {
	    static const StringArray ids =
        {
			"none",
			"unused",
			"sync",
			"async_low",
	    	"async_high"
		};

        return ids[(int)n];
    }

	static void writeDispatchString(DispatchType n, var& obj)
	{
        if(n == DispatchType::sendNotification)
            return;

        obj.getDynamicObject()->setProperty(ActionIds::notify, getDispatchName(n));
	}

	static uint32 normalisedToTimestamp(float normalised, float NumTotalSeconds);

    /** Use this method whenever you want to simulate a task that
     *  takes a certain amount of time.
     *
     */
    static void busyWait(float milliseconds);
};

/** A base class for every action that can be performed by the
 *  thread simulator unit test.
 *
 */
struct Action: public ReferenceCountedObject,
	           public ControlledObject
{
    static const std::chrono::nanoseconds overhead;
    using Ptr = ReferenceCountedObjectPtr<Action>;
    using List = ReferenceCountedArray<Action>;

    struct Builder: public ControlledObject,
				    public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<Builder>;

        Builder(MainController* mc);;

        virtual ~Builder() {};

	    virtual Action::List createActions(const var& jsonData);

        virtual Action::Ptr createAction(const Identifier& id) { jassertfalse; return nullptr; }
        virtual Identifier getId() const = 0;

        

        Action::Ptr addAction;
    };

    struct Sorter
    {
        static int compareElements(Action* first, Action* second);
    };

    Action(MainController* mc, const Identifier& type);;

    ~Action() override {};
    virtual void perform() = 0;
    StringBuilder& getActionDescription() { return b; }

    void setPreferredThread(ThreadType t);

    void setTimestamp(uint32 newTimestampMilliseconds);

    void setTimestampNormalised(float normalised);

    void expectOrThrowRuntimeError(bool condition, const String& message);

    void expectAfter(Action::Ptr addAction, bool expectSameThread);

    virtual void fromJSON(const var& obj);;

    virtual var toJSON() const;

    ThreadType getPreferredThread() const { return preferredThread; }
    uint32 getTimestamp() const noexcept { return timestampMilliseconds; }
    Identifier getType() const { return type; }

protected:

    const Identifier type;
    StringBuilder b;
    uint32 timestampMilliseconds = 0;
    ThreadType preferredThread = ThreadType::Undefined;
};

struct BusyWaitAction: public Action
{
	BusyWaitAction(MainController* mc);

    void perform() override;

	void fromJSON(const var& obj) override;

	var toJSON() const override;

	float duration;
};

/** Waits and does nothing, but calls the OS threading function
 *  to yield the thread. The timing is not as accurate as the
 *  busy sleep action, but it doesn't burn 100% CPU.
 */
struct SleepWait: public Action
{
	SleepWait(MainController* mc);
    void perform() override;

    void fromJSON(const var& obj) override;

	var toJSON() const override;

	StringBuilder b;
	int duration;
};

struct RandomActionBuilder: public Action::Builder
{
    RandomActionBuilder(MainController* mc):
      Action::Builder(mc)
    {};

    Identifier getId() const override { return ActionIds::randomevents; };
    
	Action::List createActions(const var& jsonData) override;
};


} // dummy
} // dispatch
} // hise