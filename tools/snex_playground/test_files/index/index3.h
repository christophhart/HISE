/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: -1
  output: 16
  error: ""
  filename: "index/index3"
END_TEST_DATA
*/

using IndexType = index::wrapped<17, false>;

IndexType i;


int main(int input)
{
	return (int)--i;
}

