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

#include "02_events/signal.hpp"

namespace hise {
namespace dispatch {	
using namespace juce;

// A list of all IDs that can be used in a scope statement. (eg. .defer(modules.*.attribute)

#define DECLARE_ID(x) static const Identifier x(#x);
namespace IDs
{
namespace source
{
DECLARE_ID(modules);
DECLARE_ID(automation);
DECLARE_ID(samplemap);
DECLARE_ID(content);
DECLARE_ID(broadcaster)
DECLARE_ID(complex_data);
// TODO: global events (adding / removing modules)
}
namespace event
{
DECLARE_ID(attribute);
DECLARE_ID(bypassed);
DECLARE_ID(other);
DECLARE_ID(value);
DECLARE_ID(property);
DECLARE_ID(repaint);
DECLARE_ID(intensity);
DECLARE_ID(othermod);
DECLARE_ID(namecolour);
}
}
#undef DECLARE_ID

/* The event types in ascending priority. */
enum class EventType: uint8 // change to HeaderType
{
	Nothing,
	Warning,
	LogString,
	LogRawBytes,
	Add,
	Remove,
	SlotChange,
	SourcePtr,

	ListenerAnySlot,
	ListenerWithoutData,
	SingleListenerSingleSlot,
	SingleListenerSubset,
	AllListener,
	numEventTypes
};

// What happens when a queue needs to be invalidated
// This is defined by the destructor of the Queueable object that is removed
enum class DanglingBehaviour
{
	Undefined,   // no special preference, it will either use the requested behaviour of the other end or default to Invalidate
	Invalidate,  // just sets the pointer to nullptr so subsequent calls to flush will skip the item
	CloseGap	 // close the gap so that there the queue is guaranteed to be consecutive
};

enum State
{
	Undefined,   // Not defined
	Running,	 // Everything's operational
	Paused,		 // The execution of the queue(s) are currently halted
	Rebuild,	 // a heavyweight rebuild is in process (?)
	Shutdown,	 // the app is about to be shut down
	numStates
};

enum TransportCommand // TODO
{
	Stop,		 // Stop the execution of queues
	Resume,		 // Resume at the stopped position
	Skip,		 // Skip to the end (flush the remaining elements)
	Rewind,		 // Start again at position 0
	numCommands	
};

enum class Behaviour
{
	BreakIfPaused,
	AlwaysRun,
	numBehaviours
};

enum DispatchType
{
    dontSendNotification = 0,   
    sendNotification = 1,       
    sendNotificationSync,       
	sendNotificationAsync,
	sendNotificationAsyncHiPriority // These will be executed on a separate thread and faster before
};

enum ErrorType
{
	nothing
};

using NotificationType = ErrorType;

// forward declarations of all important classes

// Any object that can be put in a Queue
// Subclassed from RootObject::Child
// Will automatically clean itself from all pending queued messages
class Queueable; // => file1.h OK

// A queue with preallocated data storage that can hold items with varying size
class Queue; // => file1.h OK

// A logger interface that can print out a serialised list of events
// TODO: make createString a static method, add a static dump(QueueEvent) function with a breakpointable condition
class Logger; // => file1.h/cpp OK

// parses the wildcard to a Source object
// usable for scope statements and the Logger class
class IdParser; //=> file1.h // TODO

// The root manager that manages the serial
// lifetime == app runtime
// multithreading (TODO): holds a read/write lock that is locked while the queues are cleared
class RootObject; // => file1.h OK

// A data object holding references to tree elements in a serialised order
// has a constant ID
// is a simple Timer
// Can register Listener objects that will react on change notifications
// lifetime < RootObject (automatically registers & deregisters itself to the RootObject)
// Examples in HISE:
// - MainController				   (IDs::source::modules),
// - ProcessorWithScriptingContent (IDs::source::content),
// - UserPresetHandler			   (IDs::source::automation)
// - Samplemap					   (IDs::source::samplemap)
//
class SourceManager;  // file2.h (SourceManager in gist)

// A class that can have multiple slots sending out change notifications
// has a SourceManager parent
// subclass of Queueable
// has a dynamic ID
// lifetime < SourceManager (automatically registers & deregisters itself from a SourceManager
// Examples in HISE:
// - Processor
// - ScriptComponent,
// - CustomAutomationData,
// - SampleMap
//
class Source; // => file2.h

// A object that sends change notification through the parent SourceManager
// has a constant ID
// has a Source parent (which has a SourceManager parent)
// lifetime < Source < SourceManager (usually a member of Source objects)
// Examples in HISE:
// - Processor::setAttribute():			(IDs::event::attribute)
// - ScriptComponent::PropertyListener  (IDs::event::property)
// - ScriptComponent::changed()			(IDs::event::value)
// - SampleMap::PropertyListener		(IDs::event::samplemap)
//
class SlotSender; // => file2.h

using SlotBitmap = VoiceBitMap<32, uint8, true>;

// A listener object that receives notifications about events of the SourceManager
// lifetime < SourceManager
// subclass of queuable (later)
// lifetime NOT guaranteed to be < Source (!), but always > SourceManager && > RootObject
// Examples in HISE:
// - PatchBrowser:		modules.*.*, async
// - ProcessorEditor:	modules.id.*, async
// - Bypass button of processor editor header: modules.id.bypassed
// - ScriptComponentWrapper: content.id.property
class Listener; // => file2.h

// defers notifications until it goes out of scope
// can be scoped to a Source and Slot using the wildcard
// lifetime < Source (usually block scope)
class ScopedDelayer; // => file2.h

// dummy objects that test the system using fuzzy testing and unit tests.
// TODO: Replicate the entire HISE system with dummy classes here
namespace test
{
// assorted unit tests
struct UnitTests;

// randomly creates objects, then simulates multithreaded usage...
struct TestRoot;

struct TestSourceManager1;  // two different source manager types
struct TestSourceManager2;

// different Source subclasses, templated to use different amounts of values / slot combinations
template <int NumValuesPerSlot> struct TestSource1_T;		// 1-100 slots, 1-100 values per slot
template <int NumValuesPerSlot> struct TestSource2_T;		// 1-100 slots, 1-100 values per slot
template <int NumValuesPerSlot> struct TestSource3_T;		// 1-100 slots, 1-100 values per slot
template <int NumSlots> struct TestSourceT_1;		
template <int NumSlots> struct TestSourceT_2;		
template <int NumSlots> struct TestSourceT_3;		
template <int NumSlots, int NumValuesPerSlot> struct TestSourceT_T;	

enum class TaskType
{
	Light,			// fast operation
	Heavyweight,	// something that will take very long
	numTaskTypes	
};

// a dummy listener that will perform short or long tasks for notifications of the given type
template <class SourceType, TaskType T, NotificationType N> struct listener;

// tests multithreaded scenarios using dummy classes
struct FuzzyTester;
}


} // dispatch
} // hise

#define SUSPEND_GLOBAL_DISPATCH(mc, description) dispatch::RootObject::ScopedGlobalSuspender sps(mc->getRootDispatcher(), dispatch::Paused, dispatch::CharPtr(description));

#ifndef ENABLE_DISPATCH_QUEUE_RESUME
#define ENABLE_DISPATCH_QUEUE_RESUME 0
#endif

#ifndef ENABLE_QUEUE_AND_LOGGER
#define ENABLE_QUEUE_AND_LOGGER 0
#endif

#if PERFETTO
#define TRACE_FLUSH(x) dispatch::StringBuilder b; b << "flush " << n << ": " << x; TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
#define TRACE_OPEN_FLOW(x, flow) dispatch::StringBuilder b; b << "send " << n << ": " << x; TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(b), flow);
#define TRACE_FLUSH_FLOW(x, flow) dispatch::StringBuilder b; b << "flush " << n << ": " << x; TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(b), flow);
#define TRACE_DISPATCH_CALLBACK(obj, callbackName, arg) StringBuilder n; n << (obj).getDispatchId() << "." << callbackName << "(" << (int)arg << ")"; TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(n));
#else
#define TRACE_FLUSH(x);
#define TRACE_OPEN_FLOW(x, flow);
#define TRACE_FLUSH_FLOW(x, flow);
#define TRACE_DISPATCH_CALLBACK(obj, callbackName, arg);
#endif

#include "file3.h" // contains all String-related classes
#include "file1.h" // contains the queue, the logger and the Stringbuilder
#include "file2.h" // contains the Source, SourceManager, Listener stuff
#include "file4.h" // contains the "library" of subclasses that emulate the HISE logic

#include "01_string/CharPtr.h" // OK

#include "02_events/RootObject.h"
#if ENABLE_QUEUE_AND_LOGGER
#include "02_events/Queue.h"
#include "01_string/Logger.h" // needs Queue
#endif
#include "01_string/StringBuilder.h" // needs Queue
#include "02_events/SourceManager.h"
#include "02_events/Source.h"
#include "02_events/Listener.h"
#include "02_events/SlotSender.h"


#include "03_library/Library.h"
#include "03_library/Processor.h"

#if HI_RUN_UNIT_TESTS
#include "04_unit_test/tests.h"
#include "04_unit_test/tests_impl.h"
#include "04_unit_test/dummy_Actions.h"
#include "04_unit_test/dummy_Threads.h"
#include "04_unit_test/dummy_Processor.h"
#include "04_unit_test/dummy_MainController.h"
#include "04_unit_test/library_tests.h"
#endif

//#include "tests.h"