/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "inheritance/inheritance_2"
END_TEST_DATA
*/

struct Base
{
	int getX()
	{
		return x;
	}
	
	int x = 9;
	int y = 10;
};

struct Derived: public Base
{
	
	int funky = 90;
};

Derived obj;

int main(int input)
{
	return obj.getX();
}

