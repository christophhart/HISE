/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 4
  error: ""
  filename: "basic/function_ref"
END_TEST_DATA
*/

// The & means reference, so the function
// will operate on the argument that was passed in
void setToFour(int& r)
{
	// the function was called with x,
	// so this will change x
    r = 4;
}

int x = 12;

int main(int input)
{	
	setToFour(x);
	
	return x;
}

