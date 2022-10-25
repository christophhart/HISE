/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 32(11): Can't use rvalues for reference parameters"
  filename: "basic/function_ref_with_dot"
END_TEST_DATA
*/

struct X
{
	int get5()
	{
		return 5;
	}
};

void changeInt(int& value)
{
	value = 8;
}

int main(int input)
{
	changeInt(input);
	
	X obj;
	
	changeInt(obj.get5());

	return input;
}

