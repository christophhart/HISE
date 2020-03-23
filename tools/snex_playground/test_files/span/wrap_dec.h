/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "span/wrap_dec"
END_TEST_DATA
*/

span<int, 8>::wrapped i;

int main(int input)
{
	
	
	return --i;
}

