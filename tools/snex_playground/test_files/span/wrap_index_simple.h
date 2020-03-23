/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "span/wrap_index_simple"
END_TEST_DATA
*/

using SpanType = span<float, 5>;


SpanType d = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

SpanType::wrapped i;
span<float, 5>::wrapped j;

int main(int input)
{
	i = 4;
	++i;
	j = 8;

	return (int)d[i] + (int)d[j];
}

