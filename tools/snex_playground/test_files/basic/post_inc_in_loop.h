/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/post_inc_in_loop"
END_TEST_DATA
*/

int counter = 0;

int main(int input)
{
	for(int i = 0; i < input; i++)
	{
		counter++;
	}
	
	return counter;
}

