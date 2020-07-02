/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "variadic/variadic_parser"
END_TEST_DATA
*/

struct X
{
    int v = 0;
    
    void process(int input)
    {
        v = input;
    }
};

container::chain<X, container::chain<X, X>> c;

int main(int input)
{
    return 12;
}

