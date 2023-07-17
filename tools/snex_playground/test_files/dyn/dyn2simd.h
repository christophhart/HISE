/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 36
  error: ""
  compile_flags: AutoVectorisation
  filename: "dyn/dyn2simd"
END_TEST_DATA
*/

span<float, 8> s = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
dyn<float> d;


int main(int input)
{
	d.referTo(s, s.size());
	float sum = 0.0f;
	
	for(auto& f4: d.toSimd())
    {
        sum += f4[0];
        sum += f4[1];
        sum += f4[2];
        sum += f4[3];
    }
    
    return sum;
}

