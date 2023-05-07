/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 120
  error: ""
  filename: "ra_pass/math_noop_float"
END_TEST_DATA
*/

float main(float input)
{
	input *= 10.0f;

	return input + 0.0f;
}

