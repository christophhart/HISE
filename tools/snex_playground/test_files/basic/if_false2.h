/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "basic/if_false2"
END_TEST_DATA
*/

int counter = 7;

int main(int input)
{
	if(input == 8)
		return counter;
		
	return counter;
}

