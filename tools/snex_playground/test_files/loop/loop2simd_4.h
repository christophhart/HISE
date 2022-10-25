/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 22
  error: ""
  compile_flags: AutoVectorisation
  filename: "loop/loop2simd_4"
END_TEST_DATA
*/


span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};

int main(int input)
{
  
    auto& x = data.toSimd();
    
	 x[0] += 21.0f;
    
	//Console.dump();
   
    
	return data[0];
}

