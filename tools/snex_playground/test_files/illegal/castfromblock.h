/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 19: Can't cast span<float, 2> to int"
  filename: "illegal/castfromblock"
END_TEST_DATA
*/

static const int s = 5;

span<span<float, 2>, s> d;

int main(int input)
{
	return d[0];
}

