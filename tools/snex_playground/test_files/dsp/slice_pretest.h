/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 24
  output: 19
  error: ""
  filename: "dsp/slice_pretest"
END_TEST_DATA
*/

int x = 19;

int main(int input)
{
	return Math.range(input, 2, x);
}

