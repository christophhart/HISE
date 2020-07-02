/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "basic/simple_if"
END_TEST_DATA
*/

int main(int input)
{
    if(input > 8)
        return 5;
        
    return 9;
}

