/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 144
  error: ""
  filename: "template/simple_template16"
END_TEST_DATA
*/

template <typename T> T multiply(T a, T b)
{
    return a * b;
}

int main(int input)
{
	return multiply(input, 12);
}

