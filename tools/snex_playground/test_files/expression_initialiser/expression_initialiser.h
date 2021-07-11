/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: -12
  output: 26
  error: ""
  filename: "expression_initialiser/expression_initialiser"
END_TEST_DATA
*/

int x = 6;
float v = 9.0f;

struct X
{
    int value = 0;
    double z = 0.0f;
};

int main(int input)
{
  X obj = { x+8, Math.abs((double)input) };
  
  return obj.value + (int)obj.z;
}

