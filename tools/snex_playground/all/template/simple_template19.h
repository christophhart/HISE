/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "template/simple_template19"
END_TEST_DATA
*/


template <int C1> struct X
{
    template <int C2> int multiply()
    {
        return C1 * C2;
    }
};

X<5> x;

int main(int input)
{
    return x.multiply<3>();
}

