/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "vop/vop8"
END_TEST_DATA
*/

span<float, 2> tick1()
{
	span<float, 2> d = { 1.0f, 2.0f };
	
	return d;
}

span<float, 2> tick2()
{
	span<float, 2> e = { 2.0f, 4.0f};
	
	e += tick1();
	
	return e;
}

int main(int input)
{
	span<float, 2> d2;
	
	d2 += tick2();
	
	return (int)(d2[0] + d2[1]);
	
}

