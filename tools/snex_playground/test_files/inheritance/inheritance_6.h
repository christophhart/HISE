/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 50
  error: ""
  filename: "inheritance/inheritance_6"
END_TEST_DATA
*/

struct dc
{
	template <int P> void setParameter(double v)
	{
		value = (int)v;
	}
	
	int value = 0;
};

struct Derived: public dc
{
	int d = 1;
};

Derived obj;

int main(int input)
{
	obj.setParameter<0>(50.0);

	return obj.value;
}

