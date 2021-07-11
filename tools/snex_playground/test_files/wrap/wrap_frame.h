/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "halfy.wav"
  error: ""
  filename: "wrap/wrap_frame"
END_TEST_DATA
*/


struct TestNode
{
	DECLARE_NODE(TestNode);
	
	template <int P> void setParameter(double v)
	{
		
	}
	
	void processFrame(span<float, 1>& data)
	{
		data[0] = 0.6f;
	}
	
	void prepare(PrepareSpecs ps)
	{
		//Console.print(ps.blockSize);
		
	}
	
	void process(ProcessData<1>& d)
	{
		
	}
	
	void reset()
	{
		
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
};



using processor = container::chain<parameter::empty, wrap::fix<1, TestNode>, wrap::frame<1, TestNode>, TestNode>;


