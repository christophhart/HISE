/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "index/looped1"
END_TEST_DATA
*/

int main(int input)
{
	index::looped<0> idx(2);

	
	span<float, 3> data = { 1.0f, 2.0f, 3.0f };
	
	return data[idx];
}

