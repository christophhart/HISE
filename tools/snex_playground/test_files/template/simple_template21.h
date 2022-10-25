/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "template/simple_template21"
END_TEST_DATA
*/


template <int C1> struct X
{
    template <int C2> int multiply()
    {
        return C1 * C2;
    }
};

X<2> x2;

int main(int input)
{
    return x2.multiply<3>();
}

