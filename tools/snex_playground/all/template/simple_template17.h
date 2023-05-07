/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 149
  error: ""
  filename: "template/simple_template17"
END_TEST_DATA
*/

template <int C, typename T> T multiplyC(T a, T b)
{
    return C + a * b;
}

int main(int input)
{
	return multiplyC<5>(input, 12);
}

