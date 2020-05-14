/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 2.0
  output: 3
  error: ""
  filename: "basic/abs_calls"
END_TEST_DATA
*/

float main(float input)
{
	float z = input - 1.0f;

	return Math.abs(input) + Math.abs(z);
}

