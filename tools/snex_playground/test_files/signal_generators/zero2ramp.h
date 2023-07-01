/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "zero.wav"
  output: "ramp.wav"
  error: ""
  filename: "zero2ramp"
END_TEST_DATA
*/

int main(block input)
{
    float v = -1.0f;
    float delta = 2.0f / (float)(1024 - 1);

    for(auto& s: input)
    {
		s = v;
		v += delta;
    }
		
    return 1;
}

