/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12.0f
  output: 90.0f
  error: ""
  filename: "basic/function_with_ternary"
END_TEST_DATA
*/

float condGet(float input)
{
    return input > 10.0f ? 90.0f : 40.0f;
}

float main(float input)
{
	return condGet(input);
    
}

