/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 23(8): First can't be resolved"
  filename: "enum/missing_class_enum"
END_TEST_DATA
*/

struct X
{
	enum class MyEnum
	{
		First = 9
	};
};

int main(int input)
{
	return X::First;
}

