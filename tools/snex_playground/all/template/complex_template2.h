/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "template/complex_template2"
END_TEST_DATA
*/


template <int C1> struct Outer
{
    template <int C2> struct Inner
    {
        int get()
        {
            return C1 + C2;
        }
    };
};

Outer<5>::Inner<3> d;


int main(int input)
{
  return d.get();
}

