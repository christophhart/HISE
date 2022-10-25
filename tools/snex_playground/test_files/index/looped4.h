/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "index/looped4"
END_TEST_DATA
*/

int main(int input)
{
	index::lerp<index::unscaled<float, index::looped<0>>> idx(3.0f);

	idx.setLoopRange(1, 3);
	
	span<float, 3> data = { 1.0f, 2.0f, 3.0f };
	
	return (int)data[idx];
}

