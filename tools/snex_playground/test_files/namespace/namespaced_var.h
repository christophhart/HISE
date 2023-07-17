/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 16
  error: ""
  filename: "namespace/namespaced_var"
END_TEST_DATA
*/


namespace Space
{
    int z = 8;
    
    int getFunky()
    {
	   return z * 2; 
    }
}

int main(int input)
{
	return Space::getFunky();
}

