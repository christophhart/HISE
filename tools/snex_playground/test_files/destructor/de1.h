/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "destructor/de1"
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

int main(int input)
{
	{
		ScopedX a;
	}

	ScopedX b;
	
	
	

	return counter;
}

