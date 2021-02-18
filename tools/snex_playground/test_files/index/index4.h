/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: -1
  output: 12
  error: ""
  filename: "index/index4"
END_TEST_DATA
*/

using IndexType = index::clamped<17, true>;

using FloatIndexType = index::normalised<float, IndexType>;

float main(int input)
{
	FloatIndexType i(19.f);

	//i = 0.8f;

	Console.print(i);
	
	return 2.0f;
}

