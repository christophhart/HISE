/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 120
  error: ""
  filename: "template/simple_template28"
END_TEST_DATA
*/


template <int C> int multiply(int input)
{
    return input * C;
}

int main(int input)
{
	return multiply<5>(input) + multiply<5>(input);
}

