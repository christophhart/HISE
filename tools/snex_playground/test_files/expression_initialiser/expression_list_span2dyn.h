/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "expression_initialiser/expression_list_span2dyn"
END_TEST_DATA
*/

span<float, 8> s = { 1.0f };

int main(int input)
{
	dyn<float> d = { s };
	
	int x = 0;
	
	for(auto& sample: d)
    {
        x += (int)sample;
    }
    
    return x;
}

