/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/simple_template23"
END_TEST_DATA
*/


struct X
{
    double a = 1.0;
    double b = 2.0;
    
    
    void setA(double v)
    {
        
    }
    
    void setB(double v)
    {
        
    }
    
    void set(double v, int T)
    {
        if(T == 0)
            setA(v);
        if(T == 1)
            setB(v);
    }
    
};

X obj;

int main(int input)
{
	obj.set(20.0, 0);
	
	return obj.a;
}

