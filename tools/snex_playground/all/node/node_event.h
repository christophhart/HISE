/**
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "node/node_event.wav"
  error: ""
  events: [{"Type": "NoteOn", "Channel": 1, "Value1": 64, "Value2": 64, "Timestamp": 256}]
  filename: "node/node_event"
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
	
	void handleHiseEvent(HiseEvent& e)
	{
		value = (float)e.getVelocity() / 127.0f;
	}
	
	void prepare(PrepareSpecs ps)
	{
	}
};

using processor = wrap::event<dc_add>;