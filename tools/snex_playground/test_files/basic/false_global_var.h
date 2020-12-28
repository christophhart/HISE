/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/false_global_var"
END_TEST_DATA
*/


int value3 = 9;

int main(int input)
{
	if(false)
	{
		value3 = 12;
	}
	
	return value3;
}

