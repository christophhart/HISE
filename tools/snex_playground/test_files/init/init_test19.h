/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
  error: ""
  filename: "init/init_test19"
END_TEST_DATA
*/

struct X
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 0.0;
};


template <typename T> struct Wrapper
{
	template <int P2> void setParameter(double v)
	{
		obj.setParameter<P2>(v);
	}
	

	T obj;
};


Wrapper<X> w;

int main(int input)
{
	w.setParameter<20>(20.0);
	
	

	return (int)w.obj.value;
}