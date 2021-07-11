/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 0.0
  error: ""
  filename: "ramp/ramp3b"
END_TEST_DATA
*/

struct ownramp
{
	int stepsToDo = 4;
	float value = 0.0f;
	float delta = 0.25f;
	
	void dudel()
	{
		stepsToDo = 4;
	}
	
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
			--stepsToDo;
			
			return v;
		}
	}
};


span<float, 8> d;
ownramp r;

float main(int input)
{
	r.dudel();

	return r.advance();	
}

