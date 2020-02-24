/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: block
  args: block
  input: "zero.wav"
  output: "fastramp.wav"
  error: ""
  filename: "zero2fastramp"
END_TEST_DATA
*/

block main(block input)
{
    float v = 0.0f;
    float delta = 16.0f / 1024.0f;
    
	for(auto& s: input)
    {
        s = Math.fmod(v, 1.0f);
        v += delta;
    }
	
    return input;
}

