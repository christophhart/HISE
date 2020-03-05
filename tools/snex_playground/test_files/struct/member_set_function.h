/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "struct/member_set_function"
END_TEST_DATA
*/

struct X
{

}


using modparam = param::chain<param::plain<core::gain, 0>, param::plain<core::osc, 2>;
mod<core::peak, modParam>;




struct X
{
    int v = 12;
    
    void setV(int input)
    {
        v = input;
    }

    float gain;

    

};



chain<X, X, chain<X, X>> c;

c.parameters[0] = { "Gain", [c](double input)
{
    rfloat inputRange = { 312.0, 90.0, 10.0f };

    input = inputRange.convertTo0To1(input);

    {
      rfloat outputRange = { 12.0, 50.0f, 12.0f, 90.0f };
      c.get<2,1>().setV(outputRange.convertFrom0To1(input));
    }

    {
      rfloat outputRange = { 12.0, 50.0f, 1.0f, 90.0f };
      c.get<0,1>().setV(outputRange.convertFrom0To1(input));
    }
});






X x;

float gain;


container::chain<Gainer, Tanh, Distorter, chain<Pan, Pan>> c;

void init()
{


  registerParameter("Gain", [](double v)
  {
    rdouble r = { 20.0000, 1.0, 0.01 };
    r.setMidPoint(120.0f);

    gain = (float)r.convertTo0To1(v);
  });
}


int main(int input)
{
    

    




	x.setV(90);
    return x.v;
    
}






[1,2, 3, 4], [5, 6, 7, 8]


=> [1, 5], [2, 4], [3, 6], [4, 8]