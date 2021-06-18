/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "index/looped5"
END_TEST_DATA
*/

int main(int input)
{
	index::looped<5> idx(11);

	idx.setLoopRange(1, 5);
	
	span<float, 5> data = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
	
	return (int)data[idx];
}

