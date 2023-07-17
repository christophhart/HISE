/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 13.8
  error: ""
  filename: "vop/vop8"
END_TEST_DATA
*/



span<float, 2> d2 = { 1.5f, 2.3f };

span<float, 2> tick1()
{
	span<float, 2> d = { 4.0f, 6.0f };
	
	return d;
}

float main(int input)
{
	d2 += tick1();
	
	return d2[0] + d2[1];
}

