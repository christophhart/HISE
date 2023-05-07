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
	DECLARE_NODE(dc);
	
	template <int P> void setParameter(double v)
	{
		value = (int)v;
	}
	
	int value = 0;
};

struct Derived: public wrap::fix<1, dc>
{
	int d = 1;
};

Derived obj;

int main(int input)
{
	obj.getObject().setParameter<0>(50.0);

	return obj.getObject().value;
}

