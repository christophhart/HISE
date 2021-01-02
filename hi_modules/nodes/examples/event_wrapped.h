/** SNEX / Scriptnode Example code

	These code examples demonstrate the C++ API of scriptnode.

	If you export a scriptnode patch to C++ code, it will look similar to these code examples.
	Also, these examples can be compiled by the SNEX JIT compiler in order to create
	machine-code optimized nodes on runtime.

	The examples have a varying degree of complexity and focus on one particular feature.
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace examples
{
/** This example contains a node that is processing incoming HiseEvents.

	By default, the handleHiseEvent() method is not called, so you need to wrap the node into
	a wrap::event template.
*/
namespace event_processor_impl
{

struct processor
{
	/** This macro tucks away all boiler-plate code (and code that can't be compiled by the SNEX compiler.
		
		The parameter is always the class name without quotes. 
	*/
	DECLARE_SNEX_NODE(processor);

	static const int NumChannels = 2;

	bool isPolyphonic() const { return false; }
	void reset() {}

	/** Add a breakpoint to this method to verify that it will be called
	*/
	void handleHiseEvent(HiseEvent& e) 
	{
		lastEvent = e;
	}

	void prepare(PrepareSpecs ps) 
	{
		// We'll store the maximum block amount here...
		maxSize = ps.blockSize;
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d) {}

	void process(ProcessData& d)
	{
		lastBufferSize = d.getNumSamples();

		/** The buffer will be divided by the incoming events, so the lastBufferSize 
		    might be smaller than the full buffer*/
		if (lastBufferSize > maxSize)
		{
			jassertfalse;
		}

		// The MIDI timestamps are aligned to the HISE_EVENT_RASTER value
		// to prevent odd timestamps from messing up the modulation rate system...
		jassert(lastBufferSize % HISE_EVENT_RASTER == 0);
	}

	bool handleModulation(double& /*v*/) { return false; }
	template <int P> void setParameter(double v) {}

	HiseEvent lastEvent;
	int lastBufferSize = -1;
	int maxSize = -1;
};


/** By wrapping the processor into a wrap::event template, we'll make sure that the
    MIDI processing will take place. 
*/
using Type = container::chain<parameter::empty, fix<1, wrap::event<processor>>>;

/** You can uncomment this line instead of the one above and the handleHiseEvent method
    will never be called. 
*/
//using Type = container::chain<parameter::empty, processor>;

struct initialiser
{
	DECLARE_SNEX_INITIALISER(event_processor);
	void initialise(Type& c) {}
};


using instance = node<initialiser, Type>;

}

using event_processor = event_processor_impl::instance;



}

}
