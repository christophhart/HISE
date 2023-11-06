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
    DECLARE_ID(id);
    DECLARE_ID(thread);
    DECLARE_ID(ts);
    DECLARE_ID(duration);
    DECLARE_ID(processor);
    DECLARE_ID(randomevents);
    DECLARE_ID(description);
    DECLARE_ID(actions);
    DECLARE_ID(type);
    DECLARE_ID(wait);
    DECLARE_ID(index);
    DECLARE_ID(num_parameters);
}

namespace ActionTypes
{
    DECLARE_ID(busywait);
    DECLARE_ID(sleep);
    DECLARE_ID(add_processor);
    DECLARE_ID(rem_processor);
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

static constexpr int NumTotalSeconds = 3;
static constexpr int TimeoutMilliseconds = 200;

struct Helpers
{
	static HashedCharPtr getThreadName(ThreadType t)
    {
	    switch(t)
	    {
	    case ThreadType::AudioThread:         return "audio";
	    case ThreadType::UIThread:            return "ui";
	    case ThreadType::ScriptingThread:     return "scripting";
	    case ThreadType::SampleLoadingThread: return "loading";
	    case ThreadType::Undefined:           return "undefined";
	    }
    }

    static ThreadType getThreadFromName(HashedCharPtr c);

	static uint32 normalisedToTimestamp(float normalised);

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
	    virtual Action::List createActions(const var& jsonData) = 0;
        virtual Identifier getId() const = 0;
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