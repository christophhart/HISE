/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "ramp.wav"
  output: "zero.wav"
  error: ""
  filename: "clear block"
END_TEST_DATA
*/

int main(block input)
{
	for(auto& s: input)
		s = 0.0f;
	
    return 1;
}

