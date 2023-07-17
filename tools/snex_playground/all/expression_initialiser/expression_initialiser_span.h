/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 10
  error: ""
  filename: "expression_initialiser/expression_initialiser_span"
END_TEST_DATA
*/

struct X
{
    int sum()
    {
        return v1 + (int)f1;
    }
    
    int v1 = 0;
    float f1 = 0.0f;
};

int i = 1;
float f = 4.0f;

int main(int input)
{
	span<X, 2> x = { {i, 2.0f }, {i+2, f } };
	
	return x[0].sum() + x[1].sum();
}

