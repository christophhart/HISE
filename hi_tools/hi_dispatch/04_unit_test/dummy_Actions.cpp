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

inline uint32 Helpers::normalisedToTimestamp(float normalised)
{
	return (uint32)roundToInt(normalised * (float)(NumTotalSeconds * 1000));
}

void Helpers::busyWait(float milliseconds)
{
	auto t2 = roundToInt(milliseconds * 1000.0f * 1000.0f);

	std::chrono::nanoseconds t{t2};
	auto end = std::chrono::steady_clock::now() + t - Action::overhead;
	while(std::chrono::steady_clock::now() < end);
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

Action::List RandomActionBuilder::createActions(const var& jsonData)
{
	Random r;
	Action::List list;

	if(jsonData.size() > 0)
	{
		jassertfalse;
	}
	else
	{
		for(int i = 0; i < r.nextInt(500); i++)
		{
			auto ts = r.nextInt(NumTotalSeconds * 1000);
			auto type = r.nextFloat() > 0.5 ? ThreadType::AudioThread : ThreadType::UIThread;

			auto na = new BusyWaitAction(getMainController());

			na->setTimestamp(ts);
			na->setPreferredThread(type);
			list.add(na);
		}
	}
	
	return list;
}
} // dummy
} // dispatch
} // hise