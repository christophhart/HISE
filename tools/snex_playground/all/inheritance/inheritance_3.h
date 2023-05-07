/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 109
  error: ""
  filename: "inheritance/inheritance_3"
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

struct Base2
{
	int getValue()
	{
		return value;
	}
	

	int value = 90;
};

struct Derived: public Base,
				public Base2
{
	int funky = 90;
};

Derived obj;

int main(int input)
{
	return obj.getValue() + obj.getX() + obj.y;
}

