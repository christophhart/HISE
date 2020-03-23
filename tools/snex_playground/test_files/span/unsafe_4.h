/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 100
  error: ""
  filename: "span/unsafe_4"
END_TEST_DATA
*/

span<float, 9>::unsafe i;

int main(int input)
{
    i = 91;
	return i.moved(9);
}

