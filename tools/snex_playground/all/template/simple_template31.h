/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "template/simple_template31"
END_TEST_DATA
*/

template <int C> struct X
{
    struct Inner
    {
	    int get() { return C; }
    };
    
    Inner i;
};



int main(int input)
{
	X<19> obj;

	return obj.i.get();
}

