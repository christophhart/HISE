/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: block
  input: "fastramp.wav"
  output: "filtered_fastramp.wav"
  error: ""
  filename: "filtered_fastramp"
END_TEST_DATA
*/



int main(block input)
{
    double a = 0.94;
    double invA = 1.0 - a;
    
    float lastValue = 0.0f;
    
    for(auto& s: input)
    {
        float thisValue = a * lastValue + invA * s;
        s = thisValue;
        lastValue = s;
    }
	
    return 1;
}

