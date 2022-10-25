/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 25(24): private X::MyEnum::First = 0 is not accessible"
  filename: "visibility/private_enum"
END_TEST_DATA
*/


class X
{
	enum MyEnum
	{
		First = 0
	};
};


int main(int input)
{
	return X::MyEnum::First;
}

