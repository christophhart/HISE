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


#define BEGIN_TEST(x) TRACE_DISPATCH(DYNAMIC_STRING(x)); beginTest(x);

struct LoggerTest: public UnitTest
{
	struct MyTestQueuable: public Queueable
	{
		MyTestQueuable(RootObject& r):
			Queueable(r)
		{};

		HashedCharPtr getDispatchId() const override { return "test"; }
	};

	struct MyDanglingQueable: public Queueable
	{
		MyDanglingQueable(RootObject& r):
			Queueable(r)
		{};

		HashedCharPtr getDispatchId() const override { return "dangling"; }
	};

	// A test object that should never be exeucted.
	struct NeverExecuted: public Queueable
	{
		NeverExecuted(RootObject& r, const HashedCharPtr& id_):
		  Queueable(r),
          id(id_)
		{};

		operator bool() const { return !wasExecuted; }
		bool wasExecuted = false;

		HashedCharPtr getDispatchId() const override { return id; }

        const HashedCharPtr id;
	};

	LoggerTest();

	void testLogger();
	void testQueue();
	void testQueueResume();
	void testSourceManager();

	void runTest() override;
};

struct CharPtrTest: public UnitTest
{
    CharPtrTest():
      UnitTest("testing char pointer classes", "dispatch")
    {};
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_length(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_length(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_hash(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_hash(CharPtrType1 p1, CharPtrType2 p2);

    // if hashed then same else diff
    template <typename CharPtrType1, typename CharPtrType2>
    void HASH(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType> void testCharPtr();
    
    void expectStringResult(const StringBuilder& b, const String& e);
    void testStringBuilder();
    
    void runTest() override;
};



namespace dummy
{

static constexpr int NumTotalSeconds = 10;
static constexpr int TimeoutMilliseconds = 200;
    
struct Action
{
    static const std::chrono::nanoseconds overhead;

    static uint32 normalisedToTimestamp(float normalised)
    {
	    return (uint32)roundToInt(normalised * (float)(NumTotalSeconds * 1000));
    }

    static void busyWait(float milliseconds)
	{
        auto t2 = roundToInt(milliseconds * 1000.0f * 1000.0f);

		std::chrono::nanoseconds t{t2};
	    auto end = std::chrono::steady_clock::now() + t - overhead;
	    while(std::chrono::steady_clock::now() < end);
	}

    struct Sorter
    {
        static int compareElements(Action* first, Action* second)
        {
            if(first->timestampMilliseconds < second->timestampMilliseconds)
            {
	            return -1;
            }
            if(first->timestampMilliseconds > second->timestampMilliseconds)
            {
                return 1;
            }
            return 0;
        }
    };

    Action(uint32 ts):
      timestampMilliseconds(ts)
    {};

    bool operator<(const Action& other) const
    {
        return timestampMilliseconds < other.timestampMilliseconds;
    }

    bool operator>(const Action& other) const
    {
        return timestampMilliseconds > other.timestampMilliseconds;
    }

    bool operator==(const Action& other) const
    {
        return timestampMilliseconds == other.timestampMilliseconds;
    }

    const uint32 timestampMilliseconds = 0;

    virtual ~Action() {};

    virtual void perform() = 0;

    virtual const StringBuilder& getActionDescription() const = 0;
};

struct BusyWaitAction: public Action
{
	BusyWaitAction(uint32 ts, float milliseconds):
      Action(ts),
      duration(milliseconds)
	{
		b << "busywait " << roundToInt(milliseconds) << "ms";
	}

    void perform() override
	{
		busyWait(duration);
	}

    const StringBuilder& getActionDescription() const override
	{
		return b;
	}

    StringBuilder b;
    const float duration;
};

struct SleepWait: public Action
{
	SleepWait(uint32 ts, int milliseconds):
      Action(ts),
      duration(milliseconds)
	{
		b << "sleep " << milliseconds << "ms";
	}

    void perform() override
	{
		Thread::getCurrentThread()->wait(duration);
	}

    const StringBuilder& getActionDescription() const override
	{
		return b;
	}

    StringBuilder b;
    const int duration;
};

struct Processor
{
    struct AddAction: public Action
	{
		AddAction(uint32 ts, library::ProcessorHandler& handler_, HashedCharPtr id_):
	      Action(ts),
          id(id_),
          handler(handler_)
		{
			b << "add processor " << id;
		};

		void perform() override
		{
			ownedProcessor = new Processor(handler, id);
		}

        const StringBuilder& getActionDescription() const override { return b; }

        StringBuilder b;
        library::ProcessorHandler& handler;
        HashedCharPtr id;
        ScopedPointer<Processor> ownedProcessor;
	};

    struct SetAttributeAction: public Action
    {
	    SetAttributeAction(uint32 ts, AddAction* addAction_):
          Action(ts),
          addAction(addAction_)
		{
			b << addAction->id << "::setAttribute";
		}

        void perform() override
		{
            if(auto a = addAction->ownedProcessor.get())
            {
            	a->setAttribute(0, 0, sendNotificationSync);
            }
		}

        const StringBuilder& getActionDescription() const override { return b; }

        StringBuilder b;
        AddAction* addAction;
    };

    struct RemoveAction: public Action
	{
		RemoveAction(uint32 ts, AddAction* addAction_):
          Action(ts),
          addAction(addAction_)
		{
			b << "remove processor " << addAction->id;
		}

        void perform() override
		{
			addAction->ownedProcessor = nullptr;
		}

        const StringBuilder& getActionDescription() const override { return b; }

        StringBuilder b;
        AddAction* addAction;
	};

    void setAttribute(int parameterIndex, float newValue, NotificationType n)
    {
	    dispatcher.setAttribute(parameterIndex, newValue, n);
    }

    Processor(library::ProcessorHandler& h, HashedCharPtr id):
      dispatcher(h, id)
    {
        dispatcher.setNumAttributes(30);
	    Action::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
    };

    ~Processor()
    {
	    Action::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
    }

	library::Processor dispatcher;
};





struct SimulatedThread: public Thread
{
    SimulatedThread(RootObject& r, CriticalSection& audioLock_, const String& name):
      Thread(name),
      root(r),
      audioLock(audioLock_)
    {};

    ~SimulatedThread() override
    {
	    stopThread(100);
    }

    virtual void startSimulation()
    {
	    startTime = Time::getMillisecondCounter();
        actionIndex = 0;
    }

	void addAction(Action* a)
    {
        Action::Sorter sorter;
        actions.addSorted(sorter, a);
    }

protected:

    CriticalSection& audioLock;
    RootObject& root;
    OwnedArray<dummy::Action> actions;

    Action* getNextAction()
    {
	    const auto deltaFromStart = Time::getMillisecondCounter() - startTime;

        if(isPositiveAndBelow(actionIndex, actions.size()) && 
           actions[actionIndex]->timestampMilliseconds < deltaFromStart )
        {
            return actions[actionIndex++];
        }

        return nullptr;
    }

private:

    uint32 startTime = 0;
    int actionIndex = 0;
};

struct AudioThread: public SimulatedThread
{
    static constexpr int AudioCallbackLength = 10;

    AudioThread(RootObject& r, CriticalSection& audioLock_):
      SimulatedThread(r, audioLock_, "Audio Thread")
    {}

    void startSimulation() override
    {
        SimulatedThread::startSimulation();
        startThread(Thread::realtimeAudioPriority);
    }
    
    void audioCallback()
    {
        TRACE_DISPATCH("audio processing");

        {

            ScopedLock sl(audioLock);
            TRACE_DISPATCH("locked");

            float microSeconds = Random::getSystemRandom().nextFloat();

            if(auto a = getNextAction())
            {
                StringBuilder b;
                b << "audio thread action: " << a->getActionDescription();
                TRACE_DYNAMIC_DISPATCH(b);
                a->perform();
            }

            auto t2 = microSeconds * 8.0;
            
            Action::busyWait(t2);
        }
    }

    void simulatedAudioThread()
    {
	    static constexpr int NumCallbacks = NumTotalSeconds * 1000 / AudioCallbackLength;
        
        TRACE_DSP();
        
	    while(!threadShouldExit() && callbackCounter++ < NumCallbacks)
	    {
            StringBuilder b;
            b << "audio callback " << callbackCounter;
            TRACE_DYNAMIC_DISPATCH(b);
		    const auto before = Time::getHighResolutionTicks();
		    audioCallback();
		    const auto delta = Time::getHighResolutionTicks() - before;
		    const auto deltaSeconds = (double)delta / (double)Time::getHighResolutionTicksPerSecond();
		    const auto deltaMilliseconds = deltaSeconds * 1000.0;

		    const auto waitTime = 10 - roundToInt(deltaMilliseconds);

            if(waitTime > 0)
                wait(waitTime);
	    }
    }

    void run() override
    {
        simulatedAudioThread();
    }

    int callbackCounter = 0;
};

struct UISimulator: public SimulatedThread
{
    UISimulator(RootObject& r, CriticalSection& audioLock_):
      SimulatedThread(r, audioLock_, "UI Simulator Thread")
    {}

    void startSimulation() override
    {
        SimulatedThread::startSimulation();
        startThread(5);
    }

    ~UISimulator() override
    {
	    stopThread(100);
    }
    
    void run() override
    {
        TRACE_DISPATCH("ui event simulator");

	    while(!threadShouldExit())
	    {
		    if(auto a = getNextAction())
            {
                StringBuilder b;
                b << "ui simulator action: " << a->getActionDescription();
                TRACE_DYNAMIC_DISPATCH(b);
                {
                    MessageManagerLock mm;
                    TRACE_DISPATCH("message lock");

                    {
	                    ScopedLock sl(audioLock);
                        TRACE_DISPATCH("audio lock");
                        a->perform();
                    }
                }
            }

            Thread::wait(15);
	    }
    }
};

struct SampleLoadingThread: public Thread
{
    void sampleLoadingTask()
    {
	    
    }
};

struct ScriptingThread: public Thread
{
    void scriptingTask()
    {
	    
    }
};

struct MainController
{
    enum class ThreadType
	{
		AudioThread,
	    UIThread,
        numThreadTypes,
	    SampleLoadingThread,
	    ScriptingThread
	};

    CriticalSection audioLock;

    MainController():
      root(&updater),
      processorHandler(root)
    {
        audioThread = threads.add(new AudioThread(root, audioLock));
        uiSimThread = threads.add(new UISimulator(root, audioLock));
    };

    void addAction(ThreadType t, Action* a)
    {
        if(auto th = threads[(int)t])
        {
	        th->addAction(a);
        }
    }

    void addSetAttributeAction(Processor::AddAction* a, uint32 timestampAfterCreation)
    {
	    auto n = new Processor::SetAttributeAction(timestampAfterCreation + a->timestampMilliseconds, a);
        addAction(ThreadType::AudioThread, n);
    }

    Processor::AddAction* addAndRemoveProcessor(HashedCharPtr id, Range<float> normalisedLifetime)
    {
        auto startLifetime = Action::normalisedToTimestamp(normalisedLifetime.getStart());
        auto endLifetime = Action::normalisedToTimestamp(normalisedLifetime.getEnd());

        auto s = new Processor::AddAction(startLifetime, processorHandler, "firstProcessor");
        auto e = new Processor::RemoveAction(endLifetime, s);

	    addAction(ThreadType::UIThread, s);
        addAction(ThreadType::UIThread, e);

        return s;
    }

    void addRandomBusyTasks()
    {
        Random r;
	    for(int i = 0; i < r.nextInt(500); i++)
        {
            auto ts = r.nextInt(NumTotalSeconds * 1000);
            auto type = r.nextFloat() > 0.5 ? ThreadType::AudioThread : ThreadType::UIThread;
            addAction(type, new BusyWaitAction(ts, r.nextFloat() * 12.0f));
        }
    }

    void start()
    {
        Random r;
        
        addRandomBusyTasks();
        
        auto p1 = addAndRemoveProcessor("first processor", { 0.2, 0.8});
        auto p2 = addAndRemoveProcessor("second processor", {0.3, 0.5});
        
        for(int i = 0; i < 100; i++)
        {
	        addSetAttributeAction(p1, Action::normalisedToTimestamp(r.nextFloat() * 0.6));
			addSetAttributeAction(p2, Action::normalisedToTimestamp(r.nextFloat() * 0.2));
        }

        


        updater.startTimer(30);

        for(auto& t: threads)
            t->startSimulation();

        started = true;
    }

    SimulatedThread* operator[](ThreadType t) const
    {
	    return threads[(int)t];
    }

    bool isFinished() const
    {
	    return started && !audioThread->isThreadRunning();
    }

    ~MainController()
    {
        threads.clear();
    }

    PooledUIUpdater updater;
	RootObject root;
	library::ProcessorHandler processorHandler;

    OwnedArray<SimulatedThread> threads;

    SimulatedThread* audioThread;
    SimulatedThread* uiSimThread;

    bool started = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainController);
};


}

struct LibraryTest: public UnitTest
{
    LibraryTest():
      UnitTest("library test", "dispatch")
    {};
    
    void init()
    {
        TRACE_DISPATCH("start library test");
	    MessageManagerLock mm;
        mc = new dummy::MainController();
        mc->start();
    }

    void shutdown()
    {
        TRACE_DISPATCH("shutdown library test");
	    MessageManagerLock mm;
        mc = nullptr;
    }

    void runTest() override;

    ScopedPointer<dummy::MainController> mc;
};


// Implementations of template functions
	template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same(CharPtrType1 p1, CharPtrType2 p2)
{
    String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
    String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
    expect(p1 == p2, t1 + " == " + t2);
    expect(p2 == p1, t2 + " == " + t1);
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff(CharPtrType1 p1, CharPtrType2 p2)
{
    String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
    String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
    expect(p1 != p2, t1 + " != " + t2);
    expect(p2 != p1, t2 + " != " + t1);
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same_length(CharPtrType1 p1, CharPtrType2 p2)
{
    expectEquals(p1.length(), p2.length(), "same length");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff_length(CharPtrType1 p1, CharPtrType2 p2)
{
    expectNotEquals(p1.length(), p2.length(), "not same length");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same_hash(CharPtrType1 p1, CharPtrType2 p2)
{
    expectEquals(p1.hash(), p2.hash(), "same hash");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff_hash(CharPtrType1 p1, CharPtrType2 p2)
{
    expectNotEquals(p1.hash(), p2.hash(), "not same hash");
}


// if hashed then same else diff

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::HASH(CharPtrType1 p1, CharPtrType2 p2)
{
    if constexpr (CharPtrType1::isHashed() || CharPtrType2::isHashed())
        same(p1, p2);
    else
        diff(p1, p2);
}

template<typename CharPtrType>
void CharPtrTest::testCharPtr()
{
    String t = CharPtrType::isHashed() ? "HashedCharPtr" : "CharPtr";

    BEGIN_TEST("test " + t + " juce::Identifier constructor");

    // 0 == 1, 1.length() == 3.length()
    CharPtr p1(hise::dispatch::IDs::event::bypassed);
    CharPtr p0(hise::dispatch::IDs::event::bypassed);
    CharPtr p2(hise::dispatch::IDs::event::value);
    CharPtr p3(hise::dispatch::IDs::event::property);

    expect(!p1.isDynamic());
    expect(!p1.isWildcard());

    same(p1, p0); same_hash(p1, p0); same_length(p1, p0);
    diff(p1, p2); diff_hash(p1, p2); diff_length(p1, p2);
    diff(p1, p3); diff_hash(p1, p3); same_length(p1, p3);

    BEGIN_TEST("test " + t + " copy constructor");

    CharPtrType c1(p1);
    CharPtrType c0(p0);
    CharPtrType c2(p2);
    CharPtrType c3(p3);

    {
        auto& t1 = c1;
        auto& t0 = c0;
        auto& t2 = c2;
        auto& t3 = c3;

        expect(!t1.isDynamic());
        expect(!t1.isWildcard());

        same(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        same(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        same(t2, p2); same_hash(t2, p2); same_length(t2, p2);
        same(t3, p3); same_hash(t3, p3); same_length(t3, p3);
    }

    BEGIN_TEST("test " + t + " raw literal constructor");

    CharPtrType r1("bypassed");
    CharPtrType r0("bypassed");
    CharPtrType r2("value");
    CharPtrType r3("property");

    {
        auto& t1 = r1;
        auto& t0 = r0;
        auto& t2 = r2;
        auto& t3 = r3;

        expect(!t1.isDynamic());
        expect(!t1.isWildcard());

        HASH(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
    }

    BEGIN_TEST("test " + t + " constructor from uint8* and size_t");

    uint8 b1[8]; uint8 b0[8]; uint8 b2[5]; uint8 b3[8];
    memcpy(b1, "bypassed", sizeof(b1));
    memcpy(b0, "bypassed", sizeof(b0));
    memcpy(b2, "value", sizeof(b2));
    memcpy(b3, "property", sizeof(b3));

    CharPtrType u1(b1, sizeof(b1));
    CharPtrType u0(b0, sizeof(b0));
    CharPtrType u2(b2, sizeof(b2));
    CharPtrType u3(b3, sizeof(b3));

    {
        auto& t1 = u1;
        auto& t0 = u0;
        auto& t2 = u2;
        auto& t3 = u3;

        expect(t1.isDynamic());
        expect(!t1.isWildcard());

        HASH(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
        HASH(t1, r0); same_hash(t1, r0); same_length(t1, r0); // cmp to raw
        diff(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        diff(t1, r3); diff_hash(t1, r3); same_length(t1, r3);
    }

    BEGIN_TEST("test " + t + " with juce::String");

    juce::String j1_("bypassed");
    juce::String j0_("bypassed");
    juce::String j2_("value");
    juce::String j3_("property");

    CharPtrType j1(j1_);
    CharPtrType j0(j0_);
    CharPtrType j2(j2_);
    CharPtrType j3(j3_);

    {
        auto& t1 = j1;
        auto& t0 = j0;
        auto& t2 = j2;
        auto& t3 = j3;

        expect(t1.isDynamic());
        expect(!t1.isWildcard());

        HASH(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
        HASH(t1, r0); same_hash(t1, r0); same_length(t1, r0); // cmp to raw
        diff(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        diff(t1, r3); diff_hash(t1, r3); same_length(t1, r3);
        HASH(t1, u0); same_hash(t1, u0); same_length(t1, u0); // cmp to byte array
        diff(t1, u2); diff_hash(t1, u2); diff_length(t1, u2);
        diff(t1, u3); diff_hash(t1, u3); same_length(t1, u3);
    }

    BEGIN_TEST("test " + t + " Wildcard");

    CharPtrType w1(CharPtrType::Type::Wildcard);
    CharPtrType w0(CharPtrType::Type::Wildcard);
    CharPtrType w2(CharPtrType::Type::Wildcard);
    CharPtrType w3(CharPtrType::Type::Wildcard);

    {
        auto& t1 = w1;
        auto& t0 = w0;
        auto& t2 = w2;
        auto& t3 = w3;

        expect(!t1.isDynamic());
        expect(t1.isWildcard());

        same(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        same(t1, t2); same_hash(t1, t2); same_length(t1, t2);
        same(t1, t3); same_hash(t1, t3); same_length(t1, t3);
        same(t1, p0); diff_hash(t1, p0); diff_length(t1, p0); // cmp to original
        same(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        same(t1, p3); diff_hash(t1, p3); diff_length(t1, p3);
        same(t1, c0); diff_hash(t1, c0); diff_length(t1, c0); // cmp to copy
        same(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        same(t1, c3); diff_hash(t1, c3); diff_length(t1, c3);
        same(t1, r0); diff_hash(t1, r0); diff_length(t1, r0); // cmp to raw
        same(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        same(t1, r3); diff_hash(t1, r3); diff_length(t1, r3);
        same(t1, u0); diff_hash(t1, u0); diff_length(t1, u0); // cmp to byte array
        same(t1, u2); diff_hash(t1, u2); diff_length(t1, u2);
        same(t1, u3); diff_hash(t1, u3); diff_length(t1, u3);
        same(t1, j0); diff_hash(t1, j0); diff_length(t1, j0); // cmp to juce::String
        same(t1, j2); diff_hash(t1, j2); diff_length(t1, j2);
        same(t1, j3); diff_hash(t1, j3); diff_length(t1, j3);
    }
}

} // dispatch
} // hise