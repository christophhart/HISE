/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 120
  error: ""
  filename: "struct/struct_member_call"
END_TEST_DATA
*/


struct X
{
    int v = 120;
    
    int getX()
    {

        return v;
    }
};

X x;

int main(int input)
{
    //return x.v;

	  return x.getX();
}

