/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 14
  error: ""
  filename: "destructor/de4"
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

void tut()
{
	Outer e;
}

int main(int input)
{
	int sum = 0;

	for(int i = 0; i < 14; i++)
	{
		ScopedX a;
		tut();
		sum += counter;
	}
	
	return counter + sum;
}

