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

struct LibraryTest: public UnitTest,
					public SourceOwner,
					public ListenerOwner
{
    LibraryTest();;
    
    void init(const var& obj);
    void deinit(int numChecksExpected);
    void runTest() override;

    void testHelloWorld(int numAttributeCalls, int numExpected, DispatchType en, DispatchType ln);

	void testHighAttributeCount()
	{
		beginTest("testing attribute index 4098");

		StringBuilder b;

		auto ln = DispatchType::sendNotificationSync;
		auto en = DispatchType::sendNotificationSync;

		RootObject r(nullptr);
		library::ProcessorHandler ph(r);
		library::Processor p(ph, *this, "test processor");

		constexpr uint16 BigAttributeIndex = 4098;

		p.setNumAttributes(BigAttributeIndex+1);

		int numCallbacks = 0;

		library::Processor::AttributeListener l(r, *this, [&numCallbacks](library::Processor* l, int)
		{
			numCallbacks++;
		});

		uint16 attributes[1] = { BigAttributeIndex };
		p.addAttributeListener(&l, attributes, 1, ln);
		p.setAttribute(BigAttributeIndex, 10.0f, en);
		p.removeAttributeListener(&l);

		expectEquals(numCallbacks, 1, "callback count mismatch");
	}

	void testSuspension(const HashedCharPtr& pathToTest)
	{
		auto path = HashedPath::parse(pathToTest);

		beginTest("test suspension with path " + pathToTest.toString());
		RootObject r(nullptr);

		library::ProcessorHandler ph(r);

		library::Processor p(ph, *this, "test_processor");
		p.setNumAttributes(1);

		int numCallbacks = 0;

		library::Processor::AttributeListener l(r, *this, [&numCallbacks](library::Processor* l, int)
		{
			numCallbacks++;
		});

		uint16 attributes[1] = { 0 };
		p.addAttributeListener(&l, attributes, 1, sendNotificationSync);

		for(int i = 0; i < 50; i++)
			p.setAttribute(0, 10.0f, sendNotificationSync);

		expectEquals(numCallbacks, 50, "callback count mismatch");

		r.setState(path, State::Paused);

		for(int i = 0; i < 50; i++)
			p.setAttribute(0, 10.0f, sendNotificationSync);

		expectEquals(numCallbacks, 50, "callback count mismatch when paused");

		r.setState(path, State::Running);

		expectEquals(numCallbacks, 51, "callback count mismatch after resume");


		p.removeAttributeListener(&l);
	}

    void runTestFiles();

    struct HardcodedTest: public JSONBuilder
	{
		using MidFunction = std::function<void(HardcodedTest&)>;

		static constexpr int AttributeIndex = 5;

		HardcodedTest(LibraryTest& parent_, const String& name, int milliSeconds=1000):
		  JSONBuilder(name, milliSeconds),
		  parent(parent_)
		{
			d.id = "my_processor";
			d.n = DispatchType::sendNotificationSync;
			d.t = ThreadType::UIThread;
			d.lifetime = {0.05, 0.9};
			
			addProcessor(d, AttributeIndex+1);
		}

		DynamicObject::Ptr expectNumCallbacks(int numExpectedAttributes, int numExpectedBypassCalls=-1)
		{
			auto processorId = d.id;
			auto listenerData = getListenerData(processorId);
			auto counter = addListenerEvent(listenerData.withSingleTimestampWithin(0.99), ActionTypes::count);
			counter->setProperty(ActionIds::attributes, numExpectedAttributes);
			counter->setProperty(ActionIds::bypassed, numExpectedBypassCalls);
			return counter;
		}

		DynamicObject::Ptr addListenerToProcessor(DispatchType n, ThreadType t=ThreadType::Undefined)
		{
			auto listenerData = d.withTimestampRangeWithin(0.1, 0.8).withDispatchType(n);

			if(t != ThreadType::Undefined)
				listenerData = listenerData.withThread(t);

			return addListener(listenerData);
		}

		DynamicObject::Ptr addListener(const Data& d)
		{
			return JSONBuilder::addListener(d, { HardcodedTest::AttributeIndex });
		}

		DynamicObject::Ptr addSetAttributeCall(double ts, DispatchType n, dummy::ThreadType t=dummy::ThreadType::Undefined)
		{
			using namespace dummy;
			auto attributeData = d.withSingleTimestampWithin(ts).withDispatchType(n);

			if(t != ThreadType::Undefined)
				attributeData = attributeData.withThread(t);

			auto obj = addProcessorEvent(attributeData, ActionTypes::set_attribute);
    		obj->setProperty(ActionIds::index, AttributeIndex);
			return obj;
		}

		void flush()
		{
			parent.runEvents(getJSON());
		}

		JSONBuilder::Data d;
		LibraryTest& parent;
	};

    void testBasicSetup();

    static File getTestDirectory()
    {
        auto f = File::getSpecialLocation(File::currentExecutableFile);
        f = f.getParentDirectory().getParentDirectory().getParentDirectory();
        f = f.getParentDirectory().getParentDirectory().getParentDirectory();
        f = f.getParentDirectory().getParentDirectory();
        
        auto dir = f.getChildFile("hi_tools/hi_dispatch/04_unit_test/json");
        jassert(dir.isDirectory());
        return dir;
    }

    void runEvents(const var& obj)
    {
        beginTest(obj[dummy::ActionIds::description].toString());

        // must not run on the message thread!
		jassert(!MessageManager::getInstance()->isThisTheMessageThread());

		init(obj);

        if(mc != nullptr)
		{
			TRACE_DISPATCH("run test");
			Thread::getCurrentThread()->wait(500);

			while(!mc->isFinished())
			{
				Thread::getCurrentThread()->wait(500);
			}
		}

        deinit((int)obj[ActionTypes::count]);
    }

    

    ScopedPointer<MainController> mc;
};

} // dummy
} // dispatch
} // hise