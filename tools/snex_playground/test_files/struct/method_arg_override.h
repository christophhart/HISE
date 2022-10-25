/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "struct/method_arg_override"
END_TEST_DATA
*/


struct X
{
    int x = 5;
    
    int getX(int x)
    {
        return x;
    }
};

X obj;

int main(int input)
{
    return obj.getX(input);
}
