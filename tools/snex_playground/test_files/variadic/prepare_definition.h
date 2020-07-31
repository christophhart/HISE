/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 512
  error: ""
  filename: "variadic/prepare_definition"
END_TEST_DATA
*/

PrepareSpecs ps = { 44100.0, 512, 2 };

int main(int input)
{
	return ps.blockSize;
}

