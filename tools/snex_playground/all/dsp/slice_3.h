/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "dsp/slice_3"
END_TEST_DATA
*/

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
	auto d = slice(data, 4, 4);
	
	for(auto& v: slice(d, 2, 2))
	    v = 9.0f;
	    
	return (int)data[6];
}

