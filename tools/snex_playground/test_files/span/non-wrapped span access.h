/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 18: Can't use non-constant or non-wrapped index"
  filename: "non-wrapped span access"
END_TEST_DATA
*/

span<int, 5> d;

int main(int input)
{
    d[3] = 5;
	d[input] = 2;
}

