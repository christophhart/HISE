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

#define BEGIN_TEST(x) beginTest(x); TRACE_DISPATCH(x);

LibraryTest::LibraryTest():
	UnitTest("library test", "dispatch")
{}

void LibraryTest::init(const var& obj)
{
	TRACE_DISPATCH("start library test");
	MessageManagerLock mm;

	mc = new MainController();
	mc->currentTest = this;

	try
	{
		mc->prepareToPlay(obj);
		mc->setActions(obj[ActionIds::objects]);
		mc->start();
	}
	catch(String& s)
	{
		expect(false, s);
		mc = nullptr;
	}
}

void LibraryTest::deinit(int numChecksExpected)
{
	if(mc != nullptr)
	{
		expectEquals(mc->numChecksPerformed, numChecksExpected, "check count mismatch");

		TRACE_DISPATCH("shutdown library test");
		MessageManagerLock mm;
		mc = nullptr;
	}
	
}

void LibraryTest::runTest()
{
	if(Thread::getCurrentThread() == nullptr)
		return;

	testMultipleListeners();

	testSigSlotBasics();

	testSlotBitMap();
	testSingleValueSources();

	testOtherChangeMessage();

	

	testMultipleAttributes({1});
	testMultipleAttributes({1, 4});
	testMultipleAttributes({1, 4, 31, 32});
	testMultipleAttributes({1, 4, 31, 322});
	testMultipleAttributes({0, 4, 31, 4, 24, 211, 32});
	testMultipleAttributes({0, 32, 64, 63, 900, 901, 902 });

	auto ct = Thread::getCurrentThread();

	if(ct != nullptr)
	{
		auto n = ct->getThreadName();
		PerfettoHelpers::setCurrentThreadName(n.getCharPointer().getAddress());
	}
	
	testHelloWorld(1, 1, sendNotificationSync, sendNotificationAsync);

	testHelloWorld(159, 159, sendNotificationSync, sendNotificationSync);
	testHelloWorld(159, 1, sendNotificationSync, sendNotificationAsync);

	testBasicSetup();

	testHighAttributeCount();

	testSuspension("*");
	testSuspension("modules.*");
	testSuspension("modules.test_processor.*");
	testSuspension("modules.*.attribute");

	auto SYNC = DispatchType::sendNotificationSync;
	auto ASYNC = DispatchType::sendNotificationAsync;
	auto HIPRIO = DispatchType::sendNotificationAsyncHiPriority;

	ignoreUnused(SYNC, ASYNC, HIPRIO);

	Helpers::forEachDispatchType([&](DispatchType n)
	{
		auto isAsync = n != DispatchType::sendNotificationSync;

		testHelloWorld(1, isAsync ? 1 : 1, SYNC, n);
		testHelloWorld(10, isAsync ? 1 : 10, SYNC, n); 
		testHelloWorld(1020, isAsync ? 1 : 1020, SYNC, n);
	});
	
	testBasicSetup();

	auto testMultipleCalls = [&](DispatchType listenerNotificationType)
	{
		HardcodedTest b(*this, "multiple calls with listener type " + Helpers::getDispatchName(listenerNotificationType), 300);

		auto l1 = b.addListenerToProcessor(listenerNotificationType, ThreadType::UIThread);
		
		for(int i = 0; i < 100; i++)
		{
			auto ts = 0.65f + (((float)i / 100.0f) * 0.01f);
			auto obj = b.addSetAttributeCall(ts, DispatchType::sendNotificationSync, ThreadType::AudioThread);
			obj->setProperty(ActionIds::duration, 0.25);
		}

		switch(listenerNotificationType)
		{
		case DispatchType::sendNotificationSync: b.expectNumCallbacks(100); break;
		case DispatchType::sendNotificationAsync: b.expectNumCallbacks(1); break;
		case DispatchType::sendNotificationAsyncHiPriority: b.expectNumCallbacks(1); break;
		}
		
		b.flush();
	};

	Helpers::forEachDispatchType(testMultipleCalls);
}

void LibraryTest::testHelloWorld(int numAttributeCalls, int numExpected, DispatchType en, DispatchType ln)
{
	StringBuilder b;

	b << "testing simple " << Helpers::getDispatchName(en) <<  " message with " + String(numAttributeCalls) + " calls and ";
	b << Helpers::getDispatchName(ln) << " listener";

	TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
	beginTest(b.toString());
	PooledUIUpdater updater;
	RootObject r(ln != sendNotificationSync ? &updater : nullptr);
	library::ProcessorHandler ph(r);

	library::Processor p(ph, *this, "test processor");
	p.setNumAttributes(1);

	int numCallbacks = 0;

	library::Processor::AttributeListener l(r, *this, [&numCallbacks](library::Processor* l, int)
	{
		numCallbacks++;
	});

	uint16 attributes[1] = { 0 };
	p.addAttributeListener(&l, attributes, 1, ln);

	for(int i = 0; i < numAttributeCalls; i++)
		p.setAttribute(0, 10.0f, en);

	if(ln != DispatchType::sendNotificationSync)
	{
		auto t = Thread::getCurrentThread();
		r.flushHighPriorityQueues(t);
		t->wait(10);
		r.flushHighPriorityQueues(t);
		t->wait(10);
		r.flushHighPriorityQueues(t);
		t->wait(480);
	}

	p.removeAttributeListener(&l);

	expectEquals(numCallbacks, numExpected, "callback count mismatch");

	MessageManagerLock mm;

	updater.stopTimer();
}

void LibraryTest::testMultipleListeners()
{
	BEGIN_TEST("testing multiple listeners");

	

	auto sync = DispatchType::sendNotificationSync;

	RootObject r(nullptr);
	library::ProcessorHandler ph(r);

	library::Processor p(ph, *this, "first");

	p.setNumAttributes(10);

	SlotBitmap bm;
	int numCalls = 0;

	library::Processor::AttributeListener l1(r, *this, [&](library::Processor* p, int slotIndex)
	{
		numCalls++;
		bm.setBit(slotIndex, true);
	});

	library::Processor::AttributeListener l2(r, *this, [&](library::Processor* p, int slotIndex)
	{
		numCalls++;
		bm.setBit(slotIndex, true);
	});

	library::Processor::AttributeListener l3(r, *this, [&](library::Processor* p, int slotIndex)
	{
		numCalls++;
		bm.setBit(slotIndex, true);
	});

	p.addAttributeListener(&l1, 3, sync);
	p.addAttributeListener(&l2, 4, sync);
	p.addAttributeListener(&l3, 5, sync);

	p.removeAttributeListener(&l2);
	
	for(int i = 0; i < 10; i++)
		p.setAttribute(i, 0.5f, sync);

	expectEquals(numCalls, 2, "call amount mismatch");

	expect(bm[3], "l1 not called");
	expect(!bm[4], "l2 was called");
	expect(bm[5], "l3 was called");

	p.removeAttributeListener(&l1);
	p.removeAttributeListener(&l3);

}

void LibraryTest::testHighAttributeCount()
{
	BEGIN_TEST("testing attribute index 4098");

	auto ln = DispatchType::sendNotificationSync;
	auto en = DispatchType::sendNotificationSync;

	StringBuilder b;

	RootObject r(nullptr);
	library::ProcessorHandler ph(r);

	{
		library::Processor p(ph, *this, "test processor");

		constexpr uint16 BigAttributeIndex = 4098;

		p.setNumAttributes(BigAttributeIndex+1);

		int numCallbacks = 0;

		library::Processor::AttributeListener l(r, *this, [&numCallbacks](library::Processor* l, uint16 idx)
		{
			jassert(idx == BigAttributeIndex);
			numCallbacks++;
		});

			

		uint16 attributes[1] = { BigAttributeIndex };
		p.addAttributeListener(&l, attributes, 1, ln);
		p.setAttribute(BigAttributeIndex, 10.0f, en);
		p.removeAttributeListener(&l);

		expectEquals(numCallbacks, 1, "callback count mismatch");

		int numIdChanges = 0;

		library::Processor::NameAndColourListener idListener(r, *this, [&](library::Processor* l)
		{
			expect(l->getDispatchId() == HashedCharPtr("changed"), "not changed");
			numIdChanges++;
		});

		p.addNameAndColourListener(&idListener, ln);
			
		p.setId("changed");

		expectEquals(numIdChanges, 1, "second id change mismatch");

		library::Processor p2(ph, *this, "second processor");

		p2.addNameAndColourListener(&idListener, ln);

		p2.setId("changed");
			
		expectEquals(numIdChanges, 2, "second id change mismatch");

		p.removeNameAndColourListener(&idListener);
		p2.removeNameAndColourListener(&idListener);

			
	}
		
		
}

void LibraryTest::testSlotBitMap()
{
	BEGIN_TEST("testing bit map");

	SlotBitmap b;
	expectEquals(static_cast<int>(b.getNumBytes()), 4, "bytesize match");
	expect(b.isEmpty(), "not empty at beginning");
	expectEquals<int>(b.getHighestSetBit(), 0, "highest bit doesn't work");
	b.setBit(12, true);
	expect(b[12], "bit not set");
	expect(!b.isEmpty(), "empty after set");
	expectEquals<int>(b.getHighestSetBit(), 12, "highest bit doesn't work");



	b.clear();
	expect(!b[12], "bit still set");
	expect(b.isEmpty(), "not empty after clear");

	b.setBit(16, true);
	b.setBit(16, false);
	b.setBit(17, true);
	b.setBit(19, true);
	expectEquals<int>(b.getHighestSetBit(), 19, "highest bit doesn't work pt. II");


	bool caught = false;

	try
	{
		b.setBit(45, true);
		expect(false, "didn't throw");
	}
	catch(std::out_of_range& e)
	{
		caught = true;
	}

	expect(caught, "not caught");
}

void LibraryTest::testMultipleAttributes(const Array<uint16>& multiParameters)
{
	TRACE_DISPATCH("test multiple parameters");
	beginTest("Testing multiple parameters with " + String(multiParameters.size()) + " indexes");

	uint16 numMax = 0;

	for(auto& m: multiParameters)
		numMax = jmax(m, numMax);

	auto ln = DispatchType::sendNotificationSync;

	RootObject r(nullptr);
	library::ProcessorHandler ph(r);

	library::Processor p(ph, *this, "multi_processor");

	p.setNumAttributes(numMax+1);

	BigInteger b;
	b.setBit(numMax+1, true);
	b.setBit(numMax+1, false);

	library::Processor::AttributeListener multiListener(r, *this, [&](library::Processor*, uint16 parameterIndex)
	{
		b.setBit(parameterIndex, true);
	});

	p.addAttributeListener(&multiListener, multiParameters.getRawDataPointer(), multiParameters.size(), ln);

	for(auto& mp: multiParameters)
		p.setAttribute(mp, 0.0f, ln);

	for(auto& mp: multiParameters)
		expect(b[mp], String(mp) + " was not received at multi test");

	p.removeAttributeListener(&multiListener);
}

void LibraryTest::testSuspension(const HashedCharPtr& pathToTest)
{
	auto path = HashedPath(pathToTest);
	TRACE_DISPATCH("test suspension");
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

void LibraryTest::testOtherChangeMessage()
{
	beginTest("Test other change message");

	RootObject r(nullptr);
	library::ProcessorHandler ph(r);
	library::Processor p(ph, *this, "test_processor");

	int numCallbacks = 0;
	
	library::Processor::OtherChangeListener l(r, *this, [&](library::Processor* p)
	{
		numCallbacks++;
	}, library::ProcessorChangeEvent::Custom);

	MessageManagerLock mm;

	p.addOtherChangeListener(&l, sendNotificationSync);

	p.sendChangeMessage(library::ProcessorChangeEvent::Macro, sendNotificationSync);

	expectEquals(numCallbacks, 0, "shouldn't fire at intensity");

	p.sendChangeMessage(library::ProcessorChangeEvent::Custom, sendNotificationSync);

	expectEquals(numCallbacks, 1, "should fire at custom");
	
	p.removeOtherChangeListener(&l);

	p.sendChangeMessage(library::ProcessorChangeEvent::Custom, sendNotificationSync);

	expectEquals(numCallbacks, 1, "remove didn't work");

	numCallbacks = 0;

	library::Processor::OtherChangeListener a(r, *this, [&](library::Processor* p)
	{
		numCallbacks++;
	}, library::ProcessorChangeEvent::Any);

	p.addOtherChangeListener(&a, sendNotificationSync);

	p.sendChangeMessage(library::ProcessorChangeEvent::Custom, sendNotificationSync);

	expectEquals(numCallbacks, 1, "should fire at custom");

	p.sendChangeMessage(library::ProcessorChangeEvent::Preset, sendNotificationSync);

	expectEquals(numCallbacks, 2, "should fire at preset");

	p.removeOtherChangeListener(&a);

	p.sendChangeMessage(library::ProcessorChangeEvent::Custom, sendNotificationSync);

	expectEquals(numCallbacks, 2, "remove didn't work");

}

void LibraryTest::testSigSlotBasics()
{
	beginTest("testing sig slot basics");

	RootObject r(nullptr);

	struct DummyListener: public dispatch::Listener
	{
		DummyListener(RootObject& r, LibraryTest& o, bool& flag_):
		  Listener(r, o),
		  t(o),
		  flag(flag_)
		{};

		~DummyListener() { clearFromRoot(); } 

		void slotChanged(const ListenerData& d) final
		{
			auto bm = d.toBitMap();
			auto x = bm[0];
			t.expect(bm[0]);
			t.expect(bm[18]);
			t.expect(!bm[12]);
			flag = true;
		}

		bool& flag;
		LibraryTest& t;
	};



	bool executed = false;
	DummyListener l(r, *this, executed);
	
	sigslot::signal<const Listener::ListenerData&> slot;
	auto con = slot.connect(&DummyListener::slotChanged, &l);

	SlotBitmap bm;
	bm.setBit(0, true);
	bm.setBit(18, true);

	Listener::ListenerData d;
	d.t = EventType::SingleListenerSubset;
	d.changes = bm;

	slot(d);

	expect(executed, "not executed");;

	DummyListener l2(r, *this, executed);

	executed = false;

	auto con2 = slot.connect(&DummyListener::slotChanged, &l2);

	slot.disconnect(con);

	slot(d);

	expect(executed, "not executed after remove");;
}

void LibraryTest::runTestFiles()
{
	File rootDirectory = getTestDirectory();

	auto list = rootDirectory.findChildFiles(File::findFiles, true, "*.json");

	for(auto l: list)
	{
		var obj;

		auto r = JSON::parse(l.loadFileAsString(), obj);
		expect(r.wasOk(), r.getErrorMessage());

		if(r.wasOk())
		{
			runEvents(obj);
		}
	}
}

LibraryTest::HardcodedTest::HardcodedTest(LibraryTest& parent_, const String& name, int milliSeconds):
	JSONBuilder(name, milliSeconds),
	parent(parent_)
{
	d.id = "my_processor";
	d.n = DispatchType::sendNotificationSync;
	d.t = ThreadType::UIThread;
	d.lifetime = {0.05, 0.9};
			
	addProcessor(d, AttributeIndex+1);
}

DynamicObject::Ptr LibraryTest::HardcodedTest::expectNumCallbacks(int numExpectedAttributes, int numExpectedBypassCalls)
{
	auto processorId = d.id;
	auto listenerData = getListenerData(processorId);
	auto counter = addListenerEvent(listenerData.withSingleTimestampWithin(0.99), ActionTypes::count);
	counter->setProperty(ActionIds::attributes, numExpectedAttributes);
	counter->setProperty(ActionIds::bypassed, numExpectedBypassCalls);
	return counter;
}

DynamicObject::Ptr LibraryTest::HardcodedTest::addListenerToProcessor(DispatchType n, ThreadType t)
{
	auto listenerData = d.withTimestampRangeWithin(0.1, 0.8).withDispatchType(n);

	if(t != ThreadType::Undefined)
		listenerData = listenerData.withThread(t);

	return addListener(listenerData);
}

DynamicObject::Ptr LibraryTest::HardcodedTest::addListener(const Data& d)
{
	return JSONBuilder::addListener(d, { HardcodedTest::AttributeIndex });
}

DynamicObject::Ptr LibraryTest::HardcodedTest::addSetAttributeCall(double ts, DispatchType n, dummy::ThreadType t)
{
	using namespace dummy;
	auto attributeData = d.withSingleTimestampWithin(ts).withDispatchType(n);

	if(t != ThreadType::Undefined)
		attributeData = attributeData.withThread(t);

	auto obj = addProcessorEvent(attributeData, ActionTypes::set_attribute);
	obj->setProperty(ActionIds::index, AttributeIndex);
	return obj;
}

void LibraryTest::HardcodedTest::flush()
{
	parent.runEvents(getJSON());
}

void LibraryTest::testBasicSetup()
{
	Helpers::forEachDispatchType<DispatchType::sendNotificationSync>([&](DispatchType an)
	{
		Helpers::forEachDispatchType<DispatchType::sendNotificationSync>([&](DispatchType ln)
		{
			Helpers::forEachThread<ThreadType::UIThread>([&](dummy::ThreadType t)
			{
				String title;
				title << "basic setup with " + dummy::Helpers::getDispatchName(an) + " attribute and ";
				title << dummy::Helpers::getDispatchName(ln) + " listener";
				using namespace dummy;

				HardcodedTest b(*this, title, 1000);

				b.addSetAttributeCall(0.5, an, t);
				b.addListenerToProcessor(ln);
				b.expectNumCallbacks(1);
				b.flush();
			});
		});
	});

}

File LibraryTest::getTestDirectory()
{
	auto f = File::getSpecialLocation(File::currentExecutableFile);
	f = f.getParentDirectory().getParentDirectory().getParentDirectory();
	f = f.getParentDirectory().getParentDirectory().getParentDirectory();
	f = f.getParentDirectory().getParentDirectory();
        
	auto dir = f.getChildFile("hi_tools/hi_dispatch/04_unit_test/json");
	jassert(dir.isDirectory());
	return dir;
}

void LibraryTest::runEvents(const var& obj)
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

void LibraryTest::testSingleValueSources()
{
	BEGIN_TEST("testing single value source");
	RootObject r(nullptr);
	library::CustomAutomationSourceManager m(r);
	library::CustomAutomationSource s(m, *this, 0, "testSource");

	int numCallbacks = 0;
	float valueToSend = 90.0f;

	library::CustomAutomationSource::Listener l(r, *this, [&](int index, float v)
	{
		numCallbacks++;
		expectEquals(v, valueToSend);
	});

	s.addValueListener(&l, false, DispatchType::sendNotificationSync);

	expect(!s.getSlotSender(0)->getListenerQueue(DispatchType::sendNotificationSync)->isEmpty(), "should not be empty");

	s.setValue(valueToSend, DispatchType::sendNotificationSync);

	//expectEquals(s.getNumListenersWithClass<LibraryTest>(), 1, "getNumListeners doesn't work");

	//expectEquals(s.getNumListenersWithClass<LibraryTest>(), 1, "getNumListeners still works");

	s.removeValueListener(&l);

	//expectEquals(s.getNumListenersWithClass<LibraryTest>(), 0, "getNumListeners doesn't work pt. II");

	expect(s.getSlotSender(0)->getListenerQueue(DispatchType::sendNotificationSync)->isEmpty(), "should be empty");

	expectEquals(numCallbacks, 1, "not fired");

	s.setValue(44, DispatchType::sendNotificationSync);

	expect(numCallbacks <= 1, "fired twice");

	bool fired = false;

	library::CustomAutomationSource::Listener l2(r, *this, [&](int index, float v)
	{
		fired = true;
		expectEquals(v, 44.0f);
	});

	s.addValueListener(&l2, true, sendNotificationSync);

	expect(fired, "second listener not fired");

	s.removeValueListener(&l2);
}

static LibraryTest libraryTest;

}
} // dispatch
} // hise
