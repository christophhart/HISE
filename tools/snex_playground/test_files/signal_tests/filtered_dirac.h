/* Filters a dirac with a simple low pass

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "dirac.wav"
  output: "filtered_dirac"
  error: ""
  filename: "signal_tests/filtered_dirac"
END_TEST_DATA
*/

int main(block input)
{
    double a = 0.7;
    double invA = 1.0 - a;
    
    float lastValue = 0.0f;
    
	for(auto& s: input)
    {
		float thisValue = a * lastValue + s * invA;
		
		s = thisValue;
		lastValue = s;
    }
	
    return 1;
}

