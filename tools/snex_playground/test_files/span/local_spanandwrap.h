/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "span/local_spanandwrap"
END_TEST_DATA
*/

int main(int input)
{
	span<int, 3> d = { 1, 2, 3 };

  index::wrapped<3, false> i(4);

	return d[i];
}

