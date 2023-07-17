/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/console"
END_TEST_DATA
*/

int main(int input)
{
	Console.print(input);
	return input;
}

