/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "vop/vop2"
END_TEST_DATA
*/

span<float, 5> data = { 0.5f};

int main(int input)
{
	data *= -1.0f;
	return 12;
}

