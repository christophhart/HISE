/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 4
  error: ""
  filename: "basic/function_ref_local"
END_TEST_DATA
*/

void setToFour(int& r)
{
    r = 4;
}



int main(int input)
{	
    int x = 12;
    
	setToFour(x);
	
	return x;
}

