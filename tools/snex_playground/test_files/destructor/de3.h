/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 0
  error: ""
  filename: "destructor/de3"
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

int tut()
{
	Outer e;

	return 12;
}

int main(int input)
{
	{
		tut();
	}
	
	return counter;
}

