/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: -4
  error: ""
  filename: "vop/vop2"
END_TEST_DATA
*/

span<float, 5> data = { 4.5f};

int main(int input)
{
	data *= -1.0f;
	return data[0];
}

