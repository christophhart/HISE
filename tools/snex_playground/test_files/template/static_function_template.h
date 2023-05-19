/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "template/static_function_template"
END_TEST_DATA
*/


struct X
{
	template <int T> static int getValue()
	{
		return T;
	}
};

int main(int input)
{
	return X::getValue<5>();
}

