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

span<int, 3> d = { 1, 2, 3};

int main(int input)
{
    {
	    auto& s = d[0];
	    s += 8;
    }
    
    
    {
        auto& s = d[0];
        input += s;
    }
    
    
    
    
    
	
	return input;
}

