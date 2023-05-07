/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0.4
  output: 8
  error: ""
  filename: "index/index5"
END_TEST_DATA
*/

using IndexType = index::wrapped<17, false>;

using NormalisedType = index::normalised<float, IndexType>;

NormalisedType i;


int main(int input)
{
	i = 0.4f;
	return i.getIndex(0, 2);
}

