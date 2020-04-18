/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "template/template_static_member"
END_TEST_DATA
*/

template <typename T> int getSomething()
{
	return T::value;
}

struct C
{
	static const int value = 9;
};

int main(int input)
{
	return getSomething<C>();
}

