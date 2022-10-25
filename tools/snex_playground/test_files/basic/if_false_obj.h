/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 0
  output: 2.0
  error: ""
  filename: "basic/if_false_obj"
END_TEST_DATA
*/

float value = 2.0f;

struct ownramp
{
	float delta = 0.25f;
	
	float advance(int input)
	{
		if (input == 0)
		{
			return value;
		}
		else
		{
			value += 1.0f;
		
			return value;
		}
	}
};


ownramp r;

float main(int input)
{
	

	return r.advance(input);	
}

