/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8;
  error: ""
  filename: "basic/complex_simple_register"
END_TEST_DATA
*/

struct X
{
    int getX()
    {
        return Math.max(2, x) + 2;
    }
    
    int x = 3;
};

int main(int input)
{
	// Use a initialiser list to overwrite the initial x value
	X obj = { 6 };
	
	return obj.getX();
}

