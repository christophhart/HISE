/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 21
  error: ""
  filename: "template/simple_template7"
END_TEST_DATA
*/

template <int First, int Second=9> struct X
{
    int get()
    {
        return First + Second;
    }
};

X<12> c;

int main(int input)
{
    return c.get();
}

