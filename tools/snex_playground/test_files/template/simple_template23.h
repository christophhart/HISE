/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 20
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
        a = v;
    }
    
    void setB(double v)
    {
        b = v;
    }
    
    template <int T> void set(double v)
    {
        if(T == 0)
            setA(v);
        if(T == 1)
            setB(v);
        
        return;
    }
    
};

X obj;

int main(int input)
{
	obj.set<0>(20.0);
	
	return (int)obj.a;
}

