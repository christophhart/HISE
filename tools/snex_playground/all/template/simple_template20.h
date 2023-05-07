/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 33
  error: ""
  filename: "template/simple_template20"
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
X<3> x3;

int main(int input)
{
    int ten = x2.multiply<2>() + x2.multiply<3>();
    int v23 = x2.multiply<4>() + x3.multiply<5>();
    
    return ten + v23;
    
}

