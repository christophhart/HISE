/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "sine.wav"
  error: ""
  filename: "node/node_sine2"
END_TEST_DATA
*/


struct osc
{
	// setting some random values here...
	double uptime = 0.1260;
	double delta = 0.2;

	void reset()
	{
		uptime = 0.0;
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
				s += 2.0f;//Math.sin(uptime);
				uptime += delta;
			}
		}

		#if 0
		auto& f = data.toFrameData();
		
		while(f.next())
		{
			f[0] += Math.sin((float)uptime);
			
			uptime += delta;
		}
		#endif
	}
	
	void handleEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		delta = (Math.PI * 2.0) / (double)ps.blockSize;
	}
};

using processor = container::chain<parameter::empty, osc>;


