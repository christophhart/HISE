/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 6
  output: 2
  error: ""
  filename: "span/span_make_index"
END_TEST_DATA
*/

using SpanType = span<int, 5>;
using WrapType = SpanType::wrapped;

SpanType d = { 1, 2, 3, 4, 5 };

int main(int input)
{
	return d[d.index<WrapType>(input)];
}

