/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "sine.wav"
  output: "distored_sine.wav"
  error: ""
  filename: "distorted_sine"
END_TEST_DATA
*/



int main(block input)
{
    for(auto& s: input)
    {
        s = Math.range(Math.tanh(s * 8.0f), -1.0f, 1.0f);
    }
	
    return 1;
}

