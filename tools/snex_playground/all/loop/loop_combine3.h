/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  loop_count: 1
  filename: "loop/loop_combine3"
END_TEST_DATA
*/

span<float, 37> data = { 0.0f };

int main(int input)
{
	for(auto& s: data)
	    s += 0.4f;
	    
	for(auto& s: data)
	    s *= 0.5f;
	    
	for(auto& s: data)
	   s /= 0.8f;
	    
	return (int)(data[0] * 10.0f);
}

