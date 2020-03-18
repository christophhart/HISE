/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "basic/scoped_local_override"
END_TEST_DATA
*/

int x = 1;

int main(int input)
{
    {
	    int x = x;
	    x = 9;
    }
    
	return x;
}

