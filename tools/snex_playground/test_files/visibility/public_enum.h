/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 10
  error: ""
  filename: "visibility/public_enum"
END_TEST_DATA
*/


class X
{
public:

	enum MyEnum
	{
		First = 10
	};
};


int main(int input)
{
	return X::MyEnum::First;
}

