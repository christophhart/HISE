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


// get a rough estimate of how much overhead there is in calling buzy_sleep()
std::chrono::nanoseconds calc_overhead() {
    using namespace std::chrono;
    constexpr size_t tests = 1001;
    constexpr auto timer = 200us;

    auto init = [&timer]() {
        auto end = steady_clock::now() + timer;
        while(steady_clock::now() < end);
    };

    time_point<steady_clock> start;
    nanoseconds dur[tests];

    for(auto& d : dur) {
        start = steady_clock::now();
        init();
        d = steady_clock::now() - start - timer;
    }
    std::sort(std::begin(dur), std::end(dur));
    // get the median value or something a little less as in this example:
    return dur[tests / 3];
}

const std::chrono::nanoseconds dummy::Action::overhead = calc_overhead();

ThreadType Helpers::getThreadFromName(HashedCharPtr c)
{
	for(int i = 0; i < (int)ThreadType::numThreadTypes; i++)
	{
		if(getThreadName((ThreadType)i) == c)
			return (ThreadType)i;
	}

	expectOrThrow(c.length() == 0, "Can't find thread name " + c.toString());
	return ThreadType::Undefined;
}

DispatchType Helpers::getDispatchFromJSON(const var& obj, DispatchType defaultValue)
{
	if(!obj.hasProperty(ActionIds::notify))
		return defaultValue;

	auto d = obj[ActionIds::notify].toString();

	static const StringArray types = { "sync", "async_low", "async_high", "none" };

	auto idx = types.indexOf(d);

	expectOrThrow(idx != -1, "Can't find notification type: " + d);

	if(idx == 0)
		return DispatchType::sendNotificationSync;
	if(idx == 1)
		return DispatchType::sendNotificationAsync;
	if(idx == 2)
		return DispatchType::sendNotificationAsyncHiPriority;
	if(idx == 3)
		return DispatchType::dontSendNotification;

	jassertfalse;
	return DispatchType::dontSendNotification;
}

inline uint32 Helpers::normalisedToTimestamp(float normalised, float NumTotalSeconds)
{
	if(normalised > 1.0)
		return (uint32)roundToInt(normalised);
	
	return (uint32)roundToInt(normalised * (NumTotalSeconds * 1000.0f));
}

void Helpers::busyWait(float milliseconds)
{
	auto t2 = roundToInt(milliseconds * 1000.0f * 1000.0f);

	std::chrono::nanoseconds t{t2};
	auto end = std::chrono::steady_clock::now() + t - Action::overhead;
	while(std::chrono::steady_clock::now() < end);
}

Action::Builder::Builder(MainController* mc):
	ControlledObject(mc)
{}

Action::List Action::Builder::createActions(const var& jsonData)
{
	Action::List list;

	if(jsonData.isArray())
	{
		for(const auto& obj: *jsonData.getArray())
		{
			Identifier type(obj[ActionIds::type].toString());
			auto newObject = createAction(type);
			newObject->fromJSON(obj);

			if(list.isEmpty())
				addAction = newObject;

			list.add(newObject);
		}
	}

	Helpers::expectOrThrow(list.size() >= 2, "must have at least an add and a rem event");
	Helpers::expectOrThrow(list.getFirst()->getType() == ActionTypes::add, "first action must be add action");
	Helpers::expectOrThrow(list.getLast()->getType() == ActionTypes::rem, "last action must be rem action");

	return list;
}

int Action::Sorter::compareElements(Action* first, Action* second)
{
	if(first->timestampMilliseconds < second->timestampMilliseconds)
		return -1;
	if(first->timestampMilliseconds > second->timestampMilliseconds)
		return 1;

	return 0;
}

Action::Action(MainController* mc, const Identifier& type_):
	ControlledObject(mc),
    type(type_)
{}

void Action::setPreferredThread(ThreadType t)
{
	preferredThread = t;
}

void Action::setTimestamp(uint32 newTimestampMilliseconds)
{
	timestampMilliseconds = newTimestampMilliseconds;
}

void Action::setTimestampNormalised(float normalised)
{
	timestampMilliseconds = Helpers::normalisedToTimestamp(normalised, (float)getMainController()->totalDurationMilliseconds * 0.001f);
}

void Action::expectOrThrowRuntimeError(bool condition, const String& message)
{
	Helpers::expectOrThrow(condition, Helpers::ErrorType::RuntimeError, getActionDescription(), message);
}

void Action::expectAfter(Action::Ptr addAction, bool expectSameThread)
{
	auto thisTime = getTimestamp();
	auto addTime = addAction->getTimestamp();

	auto d1 = getActionDescription().toString();
	auto d2 = addAction->getActionDescription().toString();
        
	Helpers::expectOrThrow(thisTime > addTime, Helpers::ErrorType::DataError, getActionDescription(), "timestamp < add event");

	if(expectSameThread)
		Helpers::expectOrThrow(getPreferredThread() == addAction->getPreferredThread(), Helpers::ErrorType::DataError, getActionDescription(), "thread mismatch to add event");
}

void Action::fromJSON(const var& obj)
{
	getActionDescription() << obj[ActionIds::id].toString();
	jassert(obj[ActionIds::type].toString() == type.toString());

	auto v = obj[ActionIds::ts];

	if(v.isDouble() && ((double)v < 1.0))
		setTimestampNormalised((float)v);
	else
		setTimestamp((uint32)(int64)v);

	preferredThread = Helpers::getThreadFromName(HashedCharPtr(obj[ActionIds::thread].toString()));
}

var Action::toJSON() const
{
	auto obj = new DynamicObject();
	obj->setProperty(ActionIds::type, type.toString());
	obj->setProperty(ActionIds::id, b.toString());
	obj->setProperty(ActionIds::ts, (int64)timestampMilliseconds);
	obj->setProperty(ActionIds::thread, Helpers::getThreadName(preferredThread).toString());
	return var(obj);
}

BusyWaitAction::BusyWaitAction(MainController* mc):
	Action(mc, ActionTypes::busywait),
	duration(0)
{
	getActionDescription() << "busywait ";
}

void BusyWaitAction::perform()
{
	Helpers::busyWait(duration);
}

void BusyWaitAction::fromJSON(const var& obj)
{
	Action::fromJSON(obj);
	duration = (float)obj[ActionIds::duration];
}

var BusyWaitAction::toJSON() const
{
	auto obj = Action::toJSON();
	obj.getDynamicObject()->setProperty(ActionIds::duration, duration);
	return obj;
}

SleepWait::SleepWait(MainController* mc):
	Action(mc, ActionTypes::sleep),
	duration(0)
{
	getActionDescription() << "sleep ";
}

void SleepWait::perform()
{
	Thread::getCurrentThread()->wait(duration);
}

void SleepWait::fromJSON(const var& obj)
{
	Action::fromJSON(obj);
	duration = (int)obj[ActionIds::duration];
}

var SleepWait::toJSON() const
{
	auto obj = Action::toJSON();
	obj.getDynamicObject()->setProperty(ActionIds::duration, duration);
	return obj;
}

Action::List RandomActionBuilder::createActions(const var& jsonData)
{
	Random r;
	Action::List list;

	// Create a function that will update the current

	if(jsonData.size() > 0)
	{
		jassertfalse;
	}
	else
	{
		jassertfalse;
#if 0
		for(int i = 0; i < r.nextInt(500); i++)
		{
			auto ts = r.nextInt(NumTotalSeconds * 1000);
			auto type = r.nextFloat() > 0.5 ? ThreadType::AudioThread : ThreadType::UIThread;

			auto na = new BusyWaitAction(getMainController());

			

			na->setTimestamp(ts);
			na->setPreferredThread(type);
			list.add(na);
		}
#endif
	}
	
	return list;
}
} // dummy
} // dispatch
} // hise