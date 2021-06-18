/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 0
  error: ""
  filename: "destructor/de_break2"
END_TEST_DATA
*/

int counter = 0;

struct X
{
	X()
	{
		++counter;
	}
	
	~X()
	{
		--counter;
	}
};

int main(int input)
{
	for(int i = 0; i < input; i++)
	{
		X obj;
	
		if(i > 5)
			continue;
		
		X obj2;
	}
	
	return counter;
}

