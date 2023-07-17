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


struct processor
{
	DECLARE_NODE(processor);

	template <int P> void setParameter(double v)
	{
		
	}

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
				s += Math.sin(uptime);
				uptime += delta;
			}
		}
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		delta = (Math.PI * 2.0) / (double)ps.blockSize;
	}
};


