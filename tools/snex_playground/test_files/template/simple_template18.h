/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "template/simple_template18"
END_TEST_DATA
*/

// Checks whether the const & ref modifiers will be 
// correctly detected during template argumenty type deduction..


struct X
{
    int x = 5;
};

template <typename T> void change(T& obj)
{
    obj.x = 8;
}

X x;

int main(int input)
{
	change(x);
	return x.x;
	
}

