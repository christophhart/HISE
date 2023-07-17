/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 22(19): wrap::fix<2, Test>: illegal channel wrap amount"
  filename: "wrap/illegal_fix"
END_TEST_DATA
*/

struct Test
{
	DECLARE_NODE(Test);
	
	int value = 12;
	
	static const int NumChannels = 1;
};

wrap::fix<2, Test> obj;

int main(int input)
{
	
	
	return 12;
}

