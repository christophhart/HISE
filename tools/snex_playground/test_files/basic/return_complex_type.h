/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 10.0f
  output: 50.0f
  error: ""
  filename: "basic/return_complex_type"
END_TEST_DATA
*/

span<float, 4> getData(float input)
{
	span<float, 4> d = { input, input * 2.0f, input * 3.0f, input * 4.0f };//input, input * 2.0f, input * 3.0f, input * 4.0f };
	return d;
}

float main(float input)
{
	auto x = getData(input);
	
	return x[0] + x[3];
}

