/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 41
  error: ""
  filename: "basic/templated_struct"
END_TEST_DATA
*/


struct hello_world
{
	// Process the signal here
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		
	}
	
	void processFrame(span<float, 1>& data)
	{
	}

	void processFrame(span<float, 2>& data)
	{
		data[0] = 41.0f;
	}
	
};
// Adding parameter methods

hello_world instance;

void processFrame(span<float, 1>& data)
{
	instance.processFrame(data);
}
void process(ProcessData<1>& data)
{
	instance.process(data);
}
void processFrame(span<float, 2>& data)
{
	instance.processFrame(data);
}
void process(ProcessData<2>& data)
{
	instance.process(data);
}


int main(int input)
{
	span<float, 2> d = { 0.0f };

	processFrame(d);
	return (int)d[0];
}



