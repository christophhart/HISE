/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "inheritance/inheritance_5"
END_TEST_DATA
*/

template <int T> struct Base
{
	int getT()
	{
		return T;
	}
};

struct Derived: public Base<3>
{
	int d = 1;
};

Derived obj;

int main(int input)
{
	return obj.getT();
}

