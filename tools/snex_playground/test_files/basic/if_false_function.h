/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/if_false_function"
END_TEST_DATA
*/

int counter = 9;

int main(int input)
{
	if(Math.randomDouble() > 2.0)
		return counter;
	
	return counter;
}

