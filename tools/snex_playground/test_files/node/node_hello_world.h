/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero2.wav"
  output: "zero2.wav"
  error: ""
  filename: "node/node_hello_world"
END_TEST_DATA
*/

struct processor
{
	void reset()
	{
	}
	
	void processFrame(span<float, 2>& data)
	{
		
	}
	
	void process(ProcessData<2>& data)
	{
		
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		
	}
};




