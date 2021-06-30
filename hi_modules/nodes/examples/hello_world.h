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
using namespace snex;
using namespace snex::Types;

namespace examples
{
/** This is just a simple test node that has all functions that a node is expected to have. 

*/
namespace hello_world_impl
{

/** Let's define a minimal class with all required callbacks. */
struct processor
{
	/** This macro tucks away all boiler-plate code (and code that can't be compiled by the SNEX compiler).

		The parameter is always the class name without quotes.
	*/
	DECLARE_NODE(processor);

	static const int NumChannels = 2;

	/** This is optional, but since parameters are identifier by their compile time integer constant
		we'll use the FirstParameter enum throughout this example for clarity.
	*/
	enum Parameters
	{
		FirstParameter = 0,
		SecondParameter,
		numParameters
	};

	/** Returns whether the node is polyphonic. */
	bool isPolyphonic() const { return false; }

	/** This method will be called right before the processing starts or if there was an interruption in the
	    audio stream.
		
		Unlike the prepare method, this method is called in the audio thread and is not supposed to take long,
		but rather initialise all state variables to their default in order to remove clicks. 
	*/
	void reset()
	{
		resetted = true;
	}

	/** If this node is supposed to process HiseEvents, this method can be used.
	
		Be aware that by default, the event processing is skipped for nodes, so in order to
		get MIDI processing, you need to wrap it into a wrap::midi node (take a look at the event_wrapped.h example).
	*/
	void handleHiseEvent(HiseEvent& e) {}

	/** This method will be called with the process specifications and can be used to setup temporary buffers and 
	    everything that the processing will require.
		
		It's guaranteed that this method will never be called simultaneuosly to one of the processing methods. 
	*/
	void prepare(PrepareSpecs ps)
	{
		prepared = true;
	}

	/** This method is being called for each signal frame. */
	void processFrame(span<float, NumChannels>& data)
	{
		data[0] = Math.max(value, 0.0);
	}

	/** This method is being called to calculate the signal buffer. */
	void process(ProcessData<NumChannels>& d)
	{
		auto f = d.toFrameData();

		while (f.next())
		{
			f[0] = value;
		}
	}

	/** This method will be called in order to calculate the modulation value.
	
		Take a look at the modulation_source.h example
	*/
	bool handleModulation(double& modValue)
	{
		return false;
	}

	/** This method will be called when a parameter is about to be changed. */
	template <int ParameterIndex> void setParameter(double v)
	{
		if (ParameterIndex == Parameters::FirstParameter)
			value = v;
	}

	// Just some dummy state variables.
	// setup a breakpoint in each callback to inspect their values...
	double value = 0.0;
	bool initialised = false;
	bool prepared = false;
	bool resetted = false;
};

/** Let's create a parameter that will control the parameter of our processor class. 
	This is being used as template argument in the container definition below and represents
	the macro parameter setup as you can define it in scriptnode.

	We want to customise the range, so that it will map it to values between 0...0.5
*/
DECLARE_PARAMETER_RANGE(firstParameterRange, 0.0, 0.5);



using FirstParameter = parameter::from0To1<processor,					// the node class
										   processor::FirstParameter,   // the parameter index
										   firstParameterRange>;        // the range class

using SecondParameter = parameter::from0To1<processor,					// the node class
	processor::SecondParameter,   // the parameter index
	firstParameterRange>;        // the range class

using ParameterType = parameter::chain<ranges::Identity, FirstParameter, SecondParameter>;

//using ParameterType = parameter::empty;

using OscType = core::oscillator<1>;

// We can't use the node directly, but wrap it into a container so that it can
// use the properties and parameters (in this example it's a bit overkill)...
using ChainWrapper = container::chain<ParameterType, processor, filters::one_pole, OscType, OscType, OscType, OscType> ;

/** This class will connect the parameters to the objects and defines the ID of this node. */
struct instance: public ChainWrapper
{
	struct metadata
	{
		SNEX_METADATA_ID("hello_world");
		SNEX_METADATA_NUM_CHANNELS(2);
		SNEX_METADATA_PARAMETERS(2, "FirstParameter", "SecondParameter");
	};

	instance()
	{
		// fetch a reference to the inner node
		auto& obj = get<0>();

		// just set the flag so you know it's initialised...
		obj.initialised = true;

		
		// As you can see, the container does not add any bytes to the actual
		// node object...
		//static_assert(chainSize == processorSize + parameterSize , "no fat");

		auto& fp = getParameter<0>();

		fp.connect<0>(obj);
		fp.connect<1>(obj);
	}
};



}

// By defining this alias, the node can be used by other code (check out the combined.h example).
using hello_world = wrap::node<hello_world_impl::instance>;

}

}
