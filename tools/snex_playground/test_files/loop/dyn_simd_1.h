/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: float
  input: 0.0f
  output: 1
  error: ""
  filename: "loop/dyn_simd_1"
END_TEST_DATA
*/

span<float, 15> s = { 1.0f };

dyn<float> d;

int main(float input)
{
	d.referTo(s);

	for(auto& v: d)
    {
        v = 8.0f;
    }
	
	for(auto& v: d)
    {
        input += v;
    }
    
    auto result = s.size() * 8;
    
    return  result == input;
}

