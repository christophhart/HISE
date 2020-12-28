/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/false_function_call"
END_TEST_DATA
*/


int value3 = 9;

void doSomething()
{
	value3 = 12;
}

int main(int input)
{
	if(false)
	{
		doSomething();
	}
	
	return value3;
}

