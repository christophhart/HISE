/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 901
  error: ""
  filename: "basic/if_change"
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
	if(input == 12)
	{
		change();
		++counter;
	}
	
	return counter;
}

