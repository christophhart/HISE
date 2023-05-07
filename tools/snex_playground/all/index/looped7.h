/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "index/looped7"
END_TEST_DATA
*/

int main(int input)
{
	StereoSample sample;
	
	sample.loopRange[0] = 1;
	sample.loopRange[1] = 4;
	
	index::unscaled<float, index::looped<0>> idx(6.0f);

	//idx.setLoopRange(1, 3);

	sample.setLoopRange(idx);
	
	span<float, 5> data = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
	
	return (int)data[idx];
}

