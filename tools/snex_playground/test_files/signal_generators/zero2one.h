/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "zero.wav"
  output: "one.wav"
  error: ""
  filename: "zero2one"
END_TEST_DATA
*/

int main(block input)
{
	for(auto& s: input)
		s = 1.0f;

	return 1;
}

