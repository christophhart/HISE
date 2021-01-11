/**
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "zero.wav"
  error: ""
  filename: "node/node_polydata"
END_TEST_DATA
*/

struct dc_add
{
	DECLARE_NODE(dc_add);

	template <int P> void setParameter(double v) {}

	float value = 0.9f;

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
				s = value;
		}
	}
	
	void handleEvent(HiseEvent& e)
	{
		value = (float)e.getVelocity() / 127.0f;
	}
	
	void prepare(PrepareSpecs ps)
	{
		pd.prepare(ps);
	
	}
	
	PolyData<int, 12> pd;
};

using processor = wrap::event<dc_add>;