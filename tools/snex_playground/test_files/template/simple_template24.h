/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "template/simple_template24"
END_TEST_DATA
*/

template <int C> struct X
{
    int get()
    {
        return C;
    };
};

int main(int input)
{
	X<5> obj;
	
	return obj.get();
}

