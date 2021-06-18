/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 12
  error: ""
  filename: "loop/while1"
END_TEST_DATA
*/

int below(int x)
{
  return x < 10;
}

int main(int input)
{
    while(below(input))
    {
        input += 4;
    }
    
    return input;
}

