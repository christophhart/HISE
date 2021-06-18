/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 340
  error: ""
  filename: "init/init_test24"
END_TEST_DATA
*/

struct MyObject
{
	MyObject()
	{
		v = 70;
	}
	
	int v = 40;
};

struct Nested
{
	Nested()
	{
		o1.v = 90;
		o2.v = 80;
	}
	
	int get() const
	{
		return o1.v + o2.v;
	}

	MyObject o1;
	MyObject o2;
};


Nested obj;

int main(int input)
{
	Nested obj2;
	
	return obj.get() + obj2.get();
}

