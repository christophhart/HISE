/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 6
  error: ""
  filename: "span/local_span_as_function_parameter"
END_TEST_DATA
*/

using TwoInts = span<int, 2>;

void clearSpan(TwoInts& data)
{
    data[0] = 3;
    data[1] = 3;
}

int main(int input)
{
	TwoInts c = { 9, 1 };
	
	clearSpan(c);
	
	return c[0] + c[1];
}

