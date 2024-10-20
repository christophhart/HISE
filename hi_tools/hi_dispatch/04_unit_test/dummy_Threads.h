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


struct SimulatedThread: public Thread,
					    public ControlledObject
{
    SimulatedThread(MainController* mc, const String& name);;

    ~SimulatedThread() override;

    virtual void startSimulation();
    void addAction(Action* a);

protected:

    CriticalSection& audioLock;
    RootObject& root;
    Action::List actions;

    Action::Ptr getNextAction();

private:

    uint32 startTime = 0;
    int actionIndex = 0;
};

struct AudioThread: public SimulatedThread
{
    static constexpr int AudioCallbackLength = 10;

    AudioThread(MainController* mc);
    ~AudioThread() override;

    void startSimulation() override;
    void audioCallback();
    void simulatedAudioThread();
    void run() override;

    void prepareToPlay(double newSampleRate, int newBufferSize);

    double sampleRate = 44100.0;
    int bufferSize = 512;
    int numCallbacksToExecute = 0;

    int callbackCounter = 0;
};

struct UISimulator: public SimulatedThread
{
    UISimulator(MainController* mc);
    ~UISimulator() override;

    void startSimulation() override;
    void simulateUIEvents();
    void run() override;
};


} // dummy
} // dispatch
} // hise