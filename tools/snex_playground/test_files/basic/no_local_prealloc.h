/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/no_local_prealloc"
END_TEST_DATA
*/

int counter = 0;

void doSomething()
{
	int x = 0;
	
	counter = x + 5;
}

int main(int input)
{
	if(input == 0)
	{
		doSomething();
	}
	
	return 12;
}

