/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<1>
  input: "zero.wav"
  output: "half.wav"
  error: ""
  filename: "dyn/simdable1"
END_TEST_DATA
*/

int main(ProcessData<1>& data)
{
	for(auto& s: data[0])
	{

		s = 0.5f;
	}
	
	return 0;
}


