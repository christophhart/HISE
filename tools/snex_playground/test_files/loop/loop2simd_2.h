/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 44
  error: ""
  filename: "loop/loop2simd_2"
END_TEST_DATA
*/

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
    
    float y = 1.0f;
    
	for(auto& s: data)
    {
        s += y;
        
    }
    
    float x = 0.0f;
    
    for(auto& s: data)
    {
        x += s;
    }
    
	return (int)x;
}

