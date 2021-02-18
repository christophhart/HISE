/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: -1
  output: 12
  error: ""
  filename: "index/index5"
END_TEST_DATA
*/

using IndexType = index::wrapped<17, false>;

using NormalisedType = index::normalised<float, IndexType>;

NormalisedType i;


int main(int input)
{
	return i.getIndex(input, 2);
}

