/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "basic/continue_for"
END_TEST_DATA
*/

int counter = 0;

int main(int input)
{
	for(int i = 0; i < input; i++)
	{
		if(i > 6)
		continue;
	
		counter++;
	}
	
	return counter;
}

