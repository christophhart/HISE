/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 18(19): %: illegal operation for vectors"
  filename: "vop/vop1"
END_TEST_DATA
*/

span<float, 5> data = { 0.5f};
span<float, 5> data2 = {0.8f};

int main(int input)
{
	data = data % 9.0f;	
}

