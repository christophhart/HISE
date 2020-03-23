/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 92
  error: ""
  filename: "span/unsafe_3"
END_TEST_DATA
*/

span<float, 9>::unsafe i;

int main(int input)
{
    i = 91;
	return ++i;
}

