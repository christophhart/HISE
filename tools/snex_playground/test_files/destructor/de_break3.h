/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 7
  error: ""
  filename: "destructor/de_break3"
END_TEST_DATA
*/

int counter = 7;

struct X
{
	X()
	{
		counter *= 2;
	}
	
	~X()
	{
		counter /= 2;
	}
};

int main(int input)
{
	if(input == ((Math.randomDouble() > 0.5) ? 12 : 8))
		return counter;
	

	{
		X obj;
		
		
		for(int i = 0; i < input; i++)
		{
			X obj;
			
			
		
			if(i > 5)
				continue;
			
			X obj2;
		}
	}
	
	return counter;
}

