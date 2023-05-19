/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 21
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
  X<3> obj2;

	return obj.i.get() + obj2.i.get();
}

