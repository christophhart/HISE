/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
  error: ""
  filename: "ra_pass/skip_mov"
END_TEST_DATA
*/

int x = 12;

int main(int input)
{
	
	x = 19;
	x = 20;
	
	return x;
}


