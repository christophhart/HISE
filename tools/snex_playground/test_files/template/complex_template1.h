/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "template/complex_template1"
END_TEST_DATA
*/

struct Outer
{
    static const int C1 = 5;
    
    template <int C2> struct Inner
    {
        int get()
        {
            return C1 + C2;
        }
    };
};

Outer::Inner<3> d;

int main(int input)
{
	return d.get();
}

