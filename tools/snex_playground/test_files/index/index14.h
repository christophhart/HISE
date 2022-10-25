/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "index/index14"
END_TEST_DATA
*/

span<float, 64> data = { 2.0f };


using IndexType = index::lerp<index::normalised<float, index::wrapped<64, true>>>;

IndexType idx;

int main(int input)
{
	idx = 0.5f;
	auto x = data[idx];
	return (int)x;
}

