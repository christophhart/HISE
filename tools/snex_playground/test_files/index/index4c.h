/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12;
  error: ""
  filename: "index/index4c"
END_TEST_DATA
*/

using IndexType = index::unsafe<0>;

int main(int input)
{
	index::unsafe<0> i(input);
	return (int)i;
}

