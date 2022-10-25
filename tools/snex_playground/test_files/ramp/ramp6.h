/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 0.25
  error: ""
  filename: "ramp/ramp6"
END_TEST_DATA
*/

sfloat r;

float main(int input)
{
	r.prepare(4.0, 1000.0);
	
	r.set(1.0f);
	r.advance();
	return r.advance();
}

