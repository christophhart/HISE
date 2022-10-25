/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "basic/inc_as_var"
END_TEST_DATA
*/

int x = 5;

int main(int input)
{
	int c = ++x;
	
	return x;
}

