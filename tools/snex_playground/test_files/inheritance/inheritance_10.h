/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "inheritance/inheritance_10"
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
		counter = 90;
	}
};

struct Derived: public Base
{
	Derived()
	{
		counter *= 3;
	} 
};

int main(int input)
{
	{
		Derived obj;
	}
	
	return counter;
}

