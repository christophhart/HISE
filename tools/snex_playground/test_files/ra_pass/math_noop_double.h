/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 120
  error: ""
  filename: "ra_pass/math_noop_double"
END_TEST_DATA
*/

double main(double input)
{
	input *= 10.0;
	

	return input + 0.0;
}

