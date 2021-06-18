/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "inheritance/inheritance_9"
END_TEST_DATA
*/

int counter = 1;

struct Base
{
	Base()
	{
		counter = 2;
	}
	
	~Base()
	{
		
	}
};

struct Derived: public Base
{
	Derived()
	{
		counter *= 3;
	}

	~Derived()
	{
		
	}
};

int main(int input)
{
	{
		Derived obj;
	}
	
	return counter;
}

