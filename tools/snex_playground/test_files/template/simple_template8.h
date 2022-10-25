/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 14
  error: ""
  filename: "template/simple_template8"
END_TEST_DATA
*/


template <typename T> struct ArraySize
{
    T t;
    
    int getSize()
    {
        return t.size();
    }
};

ArraySize<span<int, 5>> d = { {9} };

int main(int input)
{
  return d.getSize() + d.t[0];
}

