/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 1.0f
  output: 1.0f
  error: ""
  filename: "span/chain processing of structs"
END_TEST_DATA
*/

//using T = float;

struct Multiplier
{
    float op(float in)
    {
        return in * gain;
    }
    
    float gain = 1.0f;
};

//span<Multiplier, 3> mchain;

float main(float input)
{
  /*
    mchain[0].gain = 0.5f;
    mchain[1].gain = 0.25f;
    mchain[2].gain = 2.0f;
    
	for(auto& g: mchain)
	    input = g.op(input);
	  */  
	return input;
}

