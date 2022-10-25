/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 900
  error: ""
  filename: "basic/counter_change"
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
	++counter;

	change();
	
	return counter;
}

