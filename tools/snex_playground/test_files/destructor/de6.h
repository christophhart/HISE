/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 14
  error: ""
  filename: "destructor/de6"
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
};

struct Outer 
{
	
};

int sum = 0;

int main(int input)
{
	for(int i = 0; i < 14; i++)
	{
		ScopedX a;
		sum += counter;
	}
	
	return sum;
}

