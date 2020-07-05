/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  loop_count: 2
  filename: "loop/loop_combine4"
END_TEST_DATA
*/

span<span<float, 9>, 11> data = { {0.0f} };

int main(int input)
{
	for(auto& m: data)
	{
		for(auto& s: m)
		    s += 0.5f;
	}
	
	for(auto& m: data)
	{
		for(auto& s: m)
		    s *= 0.5f;
	}
	
	return (int)(data[0][0] * 8.0f);
}

