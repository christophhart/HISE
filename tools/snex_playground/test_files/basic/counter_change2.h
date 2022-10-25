/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 901
  error: ""
  filename: "basic/counter_change2"
END_TEST_DATA
*/

int sum = 0;
int counter = 0;

int main(int input)
{
	counter = 900;
	++counter;

	return counter;
}

