/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "enum/enum_as_template_argument"
END_TEST_DATA
*/

enum class MyEnum
{
	First = 9
};

template <int N> int getArgs()
{
	return N;
}

int main(int input)
{
	return getArgs<(int)MyEnum::First>();
}

