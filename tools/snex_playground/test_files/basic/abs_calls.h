/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: -2.0f
  output: 5.0f
  error: ""
  filename: "basic/abs_calls"
END_TEST_DATA
*/

float main(float input)
{
	float z = input - 1.0f;

	// You probably know the Math.abs() 
	// from the HiseScript Math class
	return Math.abs(input) + Math.abs(z);
}

