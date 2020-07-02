/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: block
  args: block
  input: "zero.wav"
  output: "dirac.wav"
  error: ""
  filename: "zero2dirac"
END_TEST_DATA
*/

block main(block input)
{
	input[0] = 1.0f;
	
    return input;
}

