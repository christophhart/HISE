/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "template/simple_template9"
END_TEST_DATA
*/


template <typename S> struct X
{
    span<S, 5> data = { 4 };
};

X<int> d;


int main(int input)
{
	return (int)d.data[0];
    
}

