/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "sine.wav"
  error: ""
  filename: "node/node_chain"
END_TEST_DATA
*/


struct dc_add
{
	void reset()
	{
	}
	
	void processFrame(span<float, 1>& data)
	{
		
	}
	
	void process(ProcessData<NumChannels>& data)
	{
		for(auto& ch: data)
		{
			for(auto& s: data.toChannelData(ch))
			{
				s += 0.125f;//Math.sin(uptime);
			}
		}
	}
	
	void handleEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
	}
};

using processor = container::chain<parameter::empty, dc_add, dc_add>;


