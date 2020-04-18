/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "ra_pass/skip_memory_write"
END_TEST_DATA
*/

span<float, 2> d = { 12.0f };

int main(int input)
{
	d[1] = 19.0f;
	d[1] = 12.0f;
	
	return 12;
}

