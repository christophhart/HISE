/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: block
  args: block
  input: "ramp.wav"
  output: "zero.wav"
  error: ""
  filename: "clear block"
END_TEST_DATA
*/

block main(block input)
{
	for(auto& s: input)
		s = 0.0f;
	
    return input;
}

