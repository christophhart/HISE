/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 16(11): Duplicate symbol int main::x"
  filename: "illegal/duplicate_variable"
END_TEST_DATA
*/

int main(int input)
{
	int x = 9;
	int x = 12;
	
	return x;
}

