/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 19.0
  error: ""
  filename: "ramp/ramp4"
END_TEST_DATA
*/

sfloat r;

span<float, 6> s;
float v = 0.0f;	


float main(int input)
{
	//r.prepare(4.0, 1000.0);
	//r.set(1.0f);
	
	r.set(19.0f);
	r.reset();
	
	return r.get();
	
	/*
	s[0] = r.advance();
	v += s[0];
	s[1] = r.advance();
	v += s[1];
	s[2] = r.advance();
	v += s[2];
	s[3] = r.advance();
	v += s[3];
	s[4] = r.advance();
	v += s[4];
	s[5] = r.advance();
	v += s[5];
	*/
}

