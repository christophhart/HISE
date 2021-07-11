/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 21
  error: ""
  filename: "template/template_overload1"
END_TEST_DATA
*/


template <typename Last> int add()
{
	return Last::value;
}


template <typename T1, typename T2> int add()
{
  return T1::value + T2::value;
}

struct C1
{
  static const int value = 5;
};

struct C2
{
  static const int value = 8;
};


int main(int input)
{
 	return add<C1, C2>() + add<C2>();
}

