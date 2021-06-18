/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12600
  error: ""
  filename: "basic/for_loop_change"
END_TEST_DATA
*/

int sum = 0;
int counter = 0;

void change()
{
	counter = 900;
}

int main(int input)
{
	for(int i = 0; i < 14; i++)
	{
		change();
		sum += counter;
	}
	
	return sum;
}

