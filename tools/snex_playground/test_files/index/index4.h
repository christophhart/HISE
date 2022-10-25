/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: -1
  output: 0.2
  error: ""
  filename: "index/index4"
END_TEST_DATA
*/

using IndexType = index::clamped<17, true>;

using FloatIndexType = index::normalised<float, IndexType>;

float main(int input)
{
	FloatIndexType i(0.2f);

	

	
	
	return (float)i;
}

