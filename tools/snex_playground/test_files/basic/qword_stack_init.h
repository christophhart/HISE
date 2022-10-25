/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 291
  error: ""
  filename: "basic/qword_stack_init"
END_TEST_DATA
*/

int main(int input)
{
	span<int, 3> data = {0x123};
	
	return data[1];
}

