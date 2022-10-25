/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "span/small_object_register"
END_TEST_DATA
*/

int main(int input)
{
	span<float, 1> d = { 3.5f };
	return (int)d[0];
}

