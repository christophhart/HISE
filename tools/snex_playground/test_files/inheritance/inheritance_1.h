/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "inheritance/inheritance_1"
END_TEST_DATA
*/

struct Base
{
	int x = 9;
	int y = 10;
};

struct Derived: public Base
{
	int getX()
	{
		return x;
	}
};

int main(int input)
{
	Derived obj;
	
	return obj.getX();
}

