/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "basic/inc_after_cond"
END_TEST_DATA
*/

int count = 12;

int main(int input)
{
	if(count >= 12)
	{
		count = 0;
	}
	
  count++;

	return count;
}

