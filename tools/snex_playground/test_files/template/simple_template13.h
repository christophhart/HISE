/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "template/simple_template13"
END_TEST_DATA
*/

template <int C> int multiply(int x)
{
    return x * C;
}

int main(int input)
{
	return multiply<5>(6);
}

