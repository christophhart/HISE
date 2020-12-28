/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/if_function_call"
END_TEST_DATA
*/


bool handleModulation(int& d)
{
	d = 90;
	
	// Change this to a non-constant
	// and it will not remove the
	// function call...
	return false; // value3 != 9;
}

int value3 = 9;

void doSomething()
{
	value3 = 12;
}

int main(int input)
{
	if(handleModulation(input))
	{
		doSomething();
	}
	
	return value3;
}

