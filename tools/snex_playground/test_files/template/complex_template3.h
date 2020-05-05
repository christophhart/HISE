/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "template/complex_template3"
END_TEST_DATA
*/


template <int C> int getSize(ProcessData<C>& data)
{
    return data.getNumChannels();
}


int main(int input)
{
    ProcessData<5> d;
    
    return getSize(d);
}