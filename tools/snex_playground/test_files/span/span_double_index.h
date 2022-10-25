/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "span/span_double_index"
END_TEST_DATA
*/

span<span<int, 2>, 3> data = {{1, 2},{3, 4},{5, 6}};

index::unsafe<0> i;

int main(int input)
{
    i = 1;
	return data[i][1];
}

