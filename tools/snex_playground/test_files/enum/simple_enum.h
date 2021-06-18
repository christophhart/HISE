/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "enum/simple_enum"
END_TEST_DATA
*/

enum class MyEnum
{
	First = 0,
	Second,
	numItems
};

int main(int input)
{
	return (int)MyEnum::First + (int)MyEnum::Second;
}

