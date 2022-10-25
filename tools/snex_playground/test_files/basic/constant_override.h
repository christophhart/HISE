/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "basic/constant_override"
END_TEST_DATA
*/

struct X
{
    static const int NumChannels = 3;
    
    int getX()
    {
        return NumChannels;
    }
};



int main(int input)
{
	X x;
	
	return NumChannels + x.getX();
}

