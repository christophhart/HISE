/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero4.wav"
  output: "half4.wav"
  error: ""
  filename: "node/node_half4frame"
END_TEST_DATA
*/

static const int NumChannels = 4;

struct processor
{
	void reset()
	{
	}
	
	void processFrame(span<float, NumChannels>& data)
	{
		
	}
	
	void process(ProcessData<NumChannels>& data)
	{
		auto f = data.toFrameData();
		
		while(f.next())
		{
			for(auto& s: f)
				s = 0.5f;
		}
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		
	}
};




