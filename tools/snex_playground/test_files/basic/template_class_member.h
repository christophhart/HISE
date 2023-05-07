/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "basic/template_class_member"
END_TEST_DATA
*/

template <int C> struct X
{
	double uptime = 30.0;

	void doSomething(float& s)
	{
		s = (float)uptime;
	}
};

X<1> instance;

int main(int input)
{
	float value = 90.0f;

	instance.doSomething(value);
	
	return (int)value;
}

