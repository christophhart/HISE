/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "basic/nested_this"
END_TEST_DATA
*/

struct Z
{
	
};

struct Funky 
{
	int getX()
	{
		return this->y;
	}
	

	int x = 12;
	int y = 9;
};

struct Outer
{
	int getValue()
	{
		return this->o2.getX();
	}

	Z obj1;
	Z obj2;

	span<int, 16> d = { 0};
	Funky o1 = { 18, 4 };
	Funky o2 = { 19, 6 };
};

Outer obj;

int main(int input)
{
	
	
	return obj.getValue();
	
}

