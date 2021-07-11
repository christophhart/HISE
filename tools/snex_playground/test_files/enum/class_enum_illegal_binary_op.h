/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 20(22): Can't implicitely cast MyEnum::First to int"
  filename: "enum/class_enum_illegal_binary_op"
END_TEST_DATA
*/

enum class MyEnum
{
	First = 9
};

int main(int input)
{
	return MyEnum::First + input;
}

