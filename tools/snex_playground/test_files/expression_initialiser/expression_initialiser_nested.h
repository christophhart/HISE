/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 30
  error: ""
  filename: "expression_initialiser/expression_initialiser_nested"
END_TEST_DATA
*/

struct X
{
    int x = 6;
    
    struct Y
    {
        double d1 = 12.0;
        double d2 = 2.0;
        
        double sum()
        {
            return d1 + d2;
        }
    };
    
    Y y1;
    
    float z = 25.0f;
};

double d = 7.0;

int main(int input)
{
	X obj = { 9, { d, d + 3.0 }, 4.0f };
	
	return obj.x + (int)(obj.y1.sum()) + (int)obj.z;
}

