/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "basic/set_before_call"
END_TEST_DATA
*/

int x = 0;

int main(int input)
{
	x = 6;
	
	Math.random();

	return x;
	
}

