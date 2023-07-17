/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "zero.wav"
  error: ""
  filename: "node/node_table"
END_TEST_DATA
*/


struct table_test
{
	static const int NumTables = 1;
	

	table_test()
	{
	}

	void reset()
	{
	}
	
	void processFrame(span<float, 1>& data)
	{
	}
	
	void process(ProcessData<1>& data)
	{	
	}
	
	void handleHiseEvent(HiseEvent& e)
	{
		
	}
	
	void prepare(PrepareSpecs ps)
	{
	}
	
	void setExternalData(const ExternalData& d, int index)
	{
		v = index;
	}
	
	int v = 10;
	dyn<float> data;
};





using processor = table_test;


