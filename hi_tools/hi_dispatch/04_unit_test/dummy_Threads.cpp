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

SimulatedThread::SimulatedThread(MainController* mc, const String& name):
	ControlledObject(mc),
	Thread(name),
	root(mc->root),
	audioLock(mc->audioLock)
{}

SimulatedThread::~SimulatedThread()
{
	stopThread(100);
}

void SimulatedThread::startSimulation()
{
	startTime = Time::getMillisecondCounter();
	actionIndex = 0;
}

void SimulatedThread::addAction(Action* a)
{
	Action::Sorter sorter;
	actions.addSorted(sorter, a);
}

Action::Ptr SimulatedThread::getNextAction()
{
	const auto deltaFromStart = Time::getMillisecondCounter() - startTime;

	if(isPositiveAndBelow(actionIndex, actions.size()) && 
		actions[actionIndex]->getTimestamp() < deltaFromStart )
	{
		return actions[actionIndex++];
	}

	return nullptr;
}

AudioThread::AudioThread(MainController* mc):
	SimulatedThread(mc, "Audio Thread")
{}

AudioThread::~AudioThread()
{
	stopThread(TimeoutMilliseconds);
}

void AudioThread::startSimulation()
{
	SimulatedThread::startSimulation();
	startThread(Thread::realtimeAudioPriority);
}

void AudioThread::audioCallback()
{
	TRACE_DISPATCH("audio processing");

	{
		ScopedLock sl(audioLock);
		TRACE_DISPATCH("locked");

		float microSeconds = Random::getSystemRandom().nextFloat();

		while(auto a = getNextAction())
		{
			StringBuilder b;
			b << "audio thread action: " << a->getActionDescription();
			TRACE_DYNAMIC_DISPATCH(b);
			a->perform();
		}

		auto t2 = microSeconds * 8.0;
            
		Helpers::busyWait(t2);
	}
}

void AudioThread::prepareToPlay(double newSampleRate, int newBufferSize)
{
    if(newSampleRate != 0.0)
        sampleRate = newSampleRate;

    if(newBufferSize != 0)
        bufferSize = newBufferSize;

    auto callbacksPerSecond = sampleRate / (double)bufferSize;

    auto durationSeconds = (double)getMainController()->totalDurationMilliseconds  * 0.001;
    numCallbacksToExecute = roundToInt(durationSeconds * callbacksPerSecond);
}


void AudioThread::simulatedAudioThread()
{
	TRACE_DSP();
        
	while(!threadShouldExit() && callbackCounter++ < numCallbacksToExecute)
	{
		StringBuilder b;
		b << "audio callback " << callbackCounter;
		TRACE_DYNAMIC_DISPATCH(b);
		const auto before = Time::getHighResolutionTicks();
		audioCallback();
		const auto delta = Time::getHighResolutionTicks() - before;
		const auto deltaSeconds = (double)delta / (double)Time::getHighResolutionTicksPerSecond();
		const auto deltaMilliseconds = deltaSeconds * 1000.0;

		auto bufferTime = roundToInt(1000.0 * (double)bufferSize / sampleRate);
		const auto waitTime = bufferTime - roundToInt(deltaMilliseconds);

		if(waitTime > 0)
			wait(waitTime);
	}
}

void AudioThread::run()
{
	try
	{
		simulatedAudioThread();
	}
	catch(String& e)
	{
		getMainController()->currentTest->expect(false, e);
	}
}

UISimulator::UISimulator(MainController* mc):
	SimulatedThread(mc, "UI Simulator Thread")
{}

void UISimulator::startSimulation()
{
	SimulatedThread::startSimulation();
	startThread(5);
}

UISimulator::~UISimulator()
{
	stopThread(100);
}

void UISimulator::simulateUIEvents()
{
	TRACE_COMPONENT();

	while(!threadShouldExit())
	{
		while(auto a = getNextAction())
		{
			StringBuilder b;
			b << "ui simulator action: " << a->getActionDescription();
			TRACE_DYNAMIC_DISPATCH(b);
			{
				MessageManagerLock mm;
				TRACE_DISPATCH("message lock");

				a->perform();

				{
					//ScopedLock sl(audioLock);
					//TRACE_DISPATCH("audio lock");
					
				}
			}
		}

		Thread::wait(15);
	}
}

void UISimulator::run()
{
	try
	{
		simulateUIEvents();
	}
	catch(String& e)
	{
		getMainController()->currentTest->expect(false, e);
	}
	
}
} // dummy
} // dispatch
} // hise