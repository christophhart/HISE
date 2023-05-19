/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 7.0f
  error: ""
  filename: "span/two_local_spans"
END_TEST_DATA
*/


float main(float input)
{
	span<float, 2> d1 = { 1.0f, 2.0f };
	span<float, 5> d2 = { 6.0f};
	
	return d1[0] + d2[4];
}

