/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/simple_template22"
END_TEST_DATA
*/


template <typename T, int NumElements> struct X
{
    T& get()
    {
        return data[1];
    }
    
    span<T, NumElements> data = { 1, 2 };
};

X<int, 2> data;

int main(int input)
{
	auto v = data.get();
	
	return v;
}

