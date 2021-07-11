/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  compile_flags: Inlining
  loop_count: 3
  filename: "loop/loop_combine2"
END_TEST_DATA
*/

span<float, 37> data = { 0.0f };


void add()
{
	for(auto& s: data)
	    s += 0.4f;
}

void mul()
{
	for(auto& s: data)
	s *= 0.5f;
}

int main(int input)
{
	add();
	mul();
	    
	
	    
	return (int)(data[0] * 10.0f);
}

