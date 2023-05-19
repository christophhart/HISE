/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24;
  error: ""
  filename: "index/index4b"
END_TEST_DATA
*/

using IndexType = index::unsafe<0>;

int main(int input)
{
	IndexType i(input);
	IndexType j = input;
	return (int)i + (int)j;
}

