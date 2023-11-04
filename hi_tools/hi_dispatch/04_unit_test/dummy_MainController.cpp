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
	
	if(auto obj = jsonData.getDynamicObject())
	{
		for(auto& nv: obj->getProperties())
		{
			if(auto b = createBuilder(nv.name))
			{
				list.addArray(b->createActions(nv.value));
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
	processorHandler(root),
    factory(this)
{
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
}

MainController::~MainController()
{
	threads.clear();
}

void MainController::setActions(const var& obj)
{
	auto list = factory.create(obj);

	for(auto a: list)
	{
		if(auto th = threads[(int)a->getPreferredThread()])
		{
			th->addAction(a);
		}
	}
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
} // dummy
} // dispatch
} // hise