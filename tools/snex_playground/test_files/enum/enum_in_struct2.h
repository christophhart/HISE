/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
  error: ""
  filename: "enum/enum_in_struct2"
END_TEST_DATA
*/


struct X
{
	enum MyOtherEnum
	{
		FirstOther = 10,
		SecondOther
	};

	enum class MyEnum
	{
		First = 9,
		Second,
		Third = 20
	};
	
	int get()
	{
		return (int)MyEnum::First + MyOtherEnum::SecondOther;
	}
};

int main(int input)
{
	X obj;
	return obj.get();
}

