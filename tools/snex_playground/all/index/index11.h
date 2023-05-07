/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 8.0
  error: ""
  filename: "index/index11"
END_TEST_DATA
*/

using IntType = index::clamped<2, false>;
using FloatType = index::normalised<float, IntType>;

index::lerp<FloatType> i;

span<float, 2> data = { 7.0f, 9.0f };

float main(int input)
{
	i = 0.25f;
	
	return data[i];
}

