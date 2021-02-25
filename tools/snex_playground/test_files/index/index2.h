/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "index/index2"
END_TEST_DATA
*/

using IndexType = index::wrapped<12, true>;

  IndexType i;


int main(int input)
{

	i = 13;

	return (int)i;
}

