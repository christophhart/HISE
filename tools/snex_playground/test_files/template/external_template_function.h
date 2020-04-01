/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "template/external_template_function"
END_TEST_DATA
*/

using SpanType = span<int, 4>;

SpanType d = { 1, 2, 3, 4 };

int main(int input)
{
	auto i = d.index<SpanType::wrapped>(6);
	
	return d[i];
	
}

