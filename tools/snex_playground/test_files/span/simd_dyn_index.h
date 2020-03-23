/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 8
  output: 1
  error: ""
  filename: "span/simd_dyn_index"
END_TEST_DATA
*/


using T = double;

span<T, 4> d = { (T)1, (T)2, (T)3, (T)4 };

int main(int input)
{
    
    span<T, 4>::wrapped i = input;
	
    
	return d[i];
}

