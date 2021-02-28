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
using WrapType = index::wrapped<5>;

SpanType d = { 1, 2, 3, 4, 5 };

int main(int input)
{
	WrapType i(input);
	return d[i];
}

