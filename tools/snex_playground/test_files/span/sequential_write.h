/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 9
  error: ""
  filename: "span/sequential_write"
END_TEST_DATA
*/

int z = 8;
double v = 9.0;
span<float, 3> d = { 1.0f, 2.0f, 3.0f};

int main(int input)
{
    {
	    auto& s = d[0];
	    s += 8.0f;
    }
    
    
    {
        auto& s = d[0];
        input += (int)s;
    }
    
    
	
	return input;
}

