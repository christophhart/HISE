/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "zero.wav"
  output: "sine.wav"
  error: ""
  filename: "zero2sine"
END_TEST_DATA
*/

int main(block input)
{
    double uptime = 0.0;
    double delta = (Math.PI * 2.0) / 1024.0;
    
	for(auto& s: input)
    {
		s = Math.sin(uptime);
		uptime += delta;
    }
	
    return 1;
}

