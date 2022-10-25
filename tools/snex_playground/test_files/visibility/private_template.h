/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: "Line 24(17): private any X::Inner::value is not accessible"
  filename: "visibility/private_template"
END_TEST_DATA
*/

struct X
{
	template <typename T> class Inner
	{
		T value = (T)5;
	};
};

int main(int input)
{
	X::Inner<int> obj;
	return obj.value;
}

