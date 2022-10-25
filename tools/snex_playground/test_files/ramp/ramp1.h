/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 3.5
  error: ""
  filename: "ramp/ramp1"
END_TEST_DATA
*/

sfloat r;

span<float, 6> d;
float v = 0.0f;	


float main(int input)
{
	r.prepare(4.0, 1000.0);
	r.set(1.0f);
	
	for(auto& s: d)
	{
		s = r.advance();
		v += s;
	}
	
	return v;
}

