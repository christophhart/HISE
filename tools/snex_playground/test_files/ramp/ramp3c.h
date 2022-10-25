/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 0.0
  error: ""
  filename: "ramp/ramp3c"
END_TEST_DATA
*/

struct ownramp
{
	// This is failing because the global initialiser doesn't work

	int stepsToDo = 0;
	float value = 0.0f;
	float delta = 0.25f;
	
	float advance()
	{
		if (stepsToDo == 0)
		{
			return value;
		}
		else
		{
			
			auto v = value;
			
			value += 0.25f;
			
			return v;
		}
	}
};


span<float, 8> d;

ownramp r;

float main(int input)
{
	

	return r.advance();	
}

