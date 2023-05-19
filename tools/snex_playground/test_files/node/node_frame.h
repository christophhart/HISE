/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero2.wav"
  output: "half2.wav"
  error: ""
  filename: "node/node_frame"
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
		data[0] = 0.5f;
		data[1] = 0.5f;
	}

	void processFrame(span<float, 1>& data)
	{
		data[0] = 0.5f;
	}

	template <int NC> void process(ProcessData<NC>& data)
	{
		auto f = data.toFrameData();
		
		while(f.next())
			processFrame(f.toSpan());
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
		
	}
};




