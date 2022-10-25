/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "nodes/node_chain.wav"
  error: ""
  filename: "node/node_chain"
END_TEST_DATA
*/


struct dc
{
	DECLARE_NODE(dc);
	
	static const int NumChannels = 1;
	
	template <int P> void setParameter(double v)
	{
		
	}

	dc()
	{
		v = 0.2f;
	}

	float v = 0.2f;

	void reset()
	{
	}
	
	void processFrame(span<float, 1>& data)
	{
		
	}
	
	void process(ProcessData<1>& data)
	{	
		for(auto& s: data[0])
			s += v;
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
	}
};

using processor = container::chain<parameter::empty, dc, dc>;


