/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 29
  error: ""
  filename: "enum/enum_in_struct"
END_TEST_DATA
*/


struct X
{
	enum MyEnum
	{
		First = 9,
		Second,
		Third = 20
	};
};

int main(int input)
{
	return X::MyEnum::First + X::Third;
}

