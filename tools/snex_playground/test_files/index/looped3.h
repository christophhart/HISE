/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "index/looped3"
END_TEST_DATA
*/

int main(int input)
{
	index::unscaled<float, index::looped<0>> idx(3.0f);

	idx.setLoopRange(1, 3);
	
	span<float, 3> data = { 0.0f, 1.0f, 2.0f };
	
	return data[idx];
}

