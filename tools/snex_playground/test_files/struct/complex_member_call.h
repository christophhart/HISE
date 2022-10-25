/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "struct/complex_member_call"
END_TEST_DATA
*/


struct X
{
    int x = 5;
    
    int getInner()
    {
        return x;
    }
    
    int getX(int b)
    {
        return b > 5 ? getInner() : 9; 
    }
    
    
};

int main(int input)
{
    X obj;
    
    return obj.getX(input);
}
