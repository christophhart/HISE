/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 2.0
  error: ""
  filename: "basic/int_cast_return"
END_TEST_DATA
*/


int x=2;

float main(int input)
{
	return (float)x;
}

