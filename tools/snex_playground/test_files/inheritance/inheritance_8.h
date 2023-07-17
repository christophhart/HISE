/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 53
  error: ""
  filename: "inheritance/inheritance_8"
END_TEST_DATA
*/

struct dc
{
	DECLARE_NODE(dc);
	
	template <int P> void setParameter(double v)
	{
		value = (int)v;
	}

	template <int P> void processFrame(span<float, P>& d)
	{
		d[0] = (float)value;
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
	span<float, 1> data;

	obj.setParameter<0>(53.0);

	obj.processFrame(data);

	return (int)data[0];
}

