/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 27
  error: ""
  filename: "span/nested_span_copy"
END_TEST_DATA
*/


span<span<int, 4>, 2> data = { {3}, {2} };

int main(int input)
{
	auto d = data[0];
	
	d[1] = 9;
	data[0][1] = 18;

	return d[1] + data[0][1];
}
