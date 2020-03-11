/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "wrap/simple_wrap_add"
END_TEST_DATA
*/

wrap<8> data = { 13 };


int main(int input)
{
	return data + 2;
}

