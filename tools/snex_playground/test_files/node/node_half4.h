/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero2.wav"
  output: "half2.wav"
  error: ""
  filename: "node/node_half4"
END_TEST_DATA
*/

static const int NumChannels = 2;

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
		for(auto c: data)
		{
			for(auto& s: data.toChannelData(c))
			{
				s = 0.5f;
			}
		}
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		
	}
};




