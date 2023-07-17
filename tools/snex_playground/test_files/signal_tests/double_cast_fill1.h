/* Filters a dirac with a simple low pass

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "zero.wav"
  output: "one.wav"
  error: ""
  filename: "signal_tests/double_cast_fill1"
END_TEST_DATA
*/

int main(block input)
{
    double a = 1.0;
    
	for(auto& s: input)
    {
		s = a;
    }
	
    return 1;
}

