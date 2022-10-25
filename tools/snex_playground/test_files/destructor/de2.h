/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "destructor/de2"
END_TEST_DATA
*/

int counter = 0;

struct ScopedX
{
	ScopedX()
	{
		
	
		counter++;
	}
	
	~ScopedX()
	{
		counter--;
	}
	
	int value = 9;
};

struct Outer 
{
	ScopedX a;
	ScopedX b;
};

int main(int input)
{
	Outer e;

	{
		Outer e;
		ScopedX funky;			
	}

	return counter;
}

