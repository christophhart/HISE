/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "index/index1"
END_TEST_DATA
*/

using IndexType = index::wrapped<12, false>;

int main(int input)
{
	IndexType i(80);

	return (int)i;
}

