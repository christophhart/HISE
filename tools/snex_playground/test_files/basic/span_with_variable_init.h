/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 23
  error: ""
  filename: "basic/span_with_variable_init"
END_TEST_DATA
*/

double x = 5.0;

int main(int input)
{
	span<double, 3> data = { x, x * 2.0, 8.0 };
	
	return (int)(data[0] + data[1] + data[2]);
}

