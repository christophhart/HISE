/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<2>
  input: zero2.wav
  output: half2.wav
  error: ""
  loop_count: 0
  filename: "loop/unroll_6"
END_TEST_DATA
*/


int main(ProcessData<2>& data)
{
	auto fp = data.toFrameData();
	
	while(fp.next())
	{
		for(auto& s: fp)
		    s = 0.5f;
	}
	
	return 0;
}

