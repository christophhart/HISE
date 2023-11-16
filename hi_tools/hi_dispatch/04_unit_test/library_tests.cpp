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
	auto n = Thread::getCurrentThread()->getThreadName();
	PerfettoHelpers::setCurrentThreadName(n.getCharPointer().getAddress());

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


static LibraryTest libraryTest;

}
} // dispatch
} // hise