/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "template/simple_template29"
END_TEST_DATA
*/

template <typename T> struct X
{
    template <int Index> T get()
    {
        return data[Index];
    }
    
    span<T, 3> data;
};

X<float> obj = { {1.0f, 2.0f, 7.0f} };

int main(int input)
{
	return obj.get<0>() + obj.get<2>();
}

