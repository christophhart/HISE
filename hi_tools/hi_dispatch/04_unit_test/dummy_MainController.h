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

    MainController();;
    ~MainController();

    void setActions(const var& obj);
    void start();
	bool isFinished() const;

    PooledUIUpdater updater;
	RootObject root;
	library::ProcessorHandler processorHandler;

    OwnedArray<SimulatedThread> threads;

    SimulatedThread* audioThread;
    SimulatedThread* uiSimThread;
    BuilderFactory factory;

    bool started = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainController);
};


} // dummy
} // dispatch
} // hise