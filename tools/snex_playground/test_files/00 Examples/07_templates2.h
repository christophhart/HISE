/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 11
  output: 18
  error: ""
  filename: "00 Examples/07_templates2"
END_TEST_DATA
*/


/** Now let's look at a barebone node definition in SNEX and how templates are used there.

	It's not complete as they are a few callbacks missing, but it includes
	 every callback that contains a template parameter.)
*/
struct ExampleNode
{
	
	/*  This method will be called to do block based processing.
		It is templated because the argument type can be different
		according to whether the processing is using a fix amount of
		channels (in this case, ProcessDataType will be `ProcessData<NumChannels>`)
		or a dynamic one (in this case ProcessDataType will be `ProcessDataDyn`)
	*/
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto frameProcessor = data.toFrameData();
		
		while(frameProcessor.next())
		{
			processFrame(frameProcessor.toSpan());
		}
	}
	
	/** This method will be called to do frame based processing. A frame
	    is a consecutive float array with one sample per channel, so you
	    can do interleaved processing.
	    
	    Because this function will be called very often, it needs to be
	    able to make compile time assumptions about the channel amount,
	    so the FrameDataType will always be a `span<float, NumChannels>`.
	*/
	template <typename FrameDataType> void processFrame(FrameDataType& data) 
	{
		// This loop will most likely be unrolled because the compiler
		// knows exactly how many elements `data` has
		for(auto& s: data)
		    s = 0.0f;
	}
	
	template <int P> void setParameter(double value)
	{
		
	}
};


span<float, 32> leftChannel = {0.0f};
span<float, 32> rightChannel = {0.0f};



int main(int input)
{
	span<dyn<float>, 2> channels;
	
	channels[0].referTo(leftChannel);
	channels[1].referTo(rightChannel);

	// This class will contain the processing information.
	// We'll pass the channels into the constructor
	// and it will initialise the channel pointers
	ProcessData<2> processData(channels);

	//TODO: make a constructor that takes a span<dyn<float>, NumChannels>

	ExampleNode testNode;
	
	
}

