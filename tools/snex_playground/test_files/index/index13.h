/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 12
  error: ""
  filename: "index/index13"
END_TEST_DATA
*/

using IntType = index::unsafe<3, false>;
using FloatType = index::unscaled<float, IntType>;

#if 0
using InterpolationType = index::hermite<FloatType>;
#else
using InterpolationType = index::lerp<FloatType>;
#endif


InterpolationType i;

span<span<float, 2>, 3> data = { {0.0f, 1.0f}, {2.0f, 19.0f}, {3.5f, 5.0f} };

float main(int input)
{
	i = 1.5f;
	
	return data[i][1];
}

