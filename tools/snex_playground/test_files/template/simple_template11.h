/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/simple_template11"
END_TEST_DATA
*/


template <typename T, int NumElements> struct PolyData
{
    span<T, NumElements>::wrapped index;
};

PolyData<int, 256> d;

int main(int input)
{
	 d.index = 258;
   return d.index;
}

