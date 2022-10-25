/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "template/simple_template30"
END_TEST_DATA
*/

template <typename T> struct X
{
    struct Inner
    {
	    int value = 9;
    };
    
    Inner i;
};



int main(int input)
{
	X<float> obj;

	return obj.i.value;
}

