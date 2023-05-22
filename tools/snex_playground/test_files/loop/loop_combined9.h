/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 80
  error: ""
  compile_flags: Inlining
  loop_count: 3
  filename: "loop/loop_combined9"
END_TEST_DATA
*/

void tut1(span<float, 9>& d)
{
	for(auto& s: d)
	    s += 8.0f;
}

void tut2(span<float, 9>& d)
{
	for(auto& s: d)
	    s *= 8.0f;
}

int main(int input)
{
	span<float, 9> data = { 2.0f };
	
	tut1(data);
	tut2(data);
	
	return (int)data[0];
}

