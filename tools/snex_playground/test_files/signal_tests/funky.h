/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<4>
  input: "zero4.wav"
  output: "half4.wav"
  error: ""
  filename: "signal_tests/funky"
END_TEST_DATA
*/

int main(ProcessData<4>& data)
{
	
	for(auto& c: data)
	{
		for(auto& s: data.toChannelData(c))
		{
			s = 0.5f;
		}
	}
	
	
	return 0;
}


