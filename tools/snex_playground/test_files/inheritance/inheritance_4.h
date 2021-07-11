/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "inheritance/inheritance_4"
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
	int getDoubleX()
	{
		return getX() * 2 + d;
	}
	
	int d = 1;
};

Derived obj;

int main(int input)
{
	return obj.getDoubleX();
}

