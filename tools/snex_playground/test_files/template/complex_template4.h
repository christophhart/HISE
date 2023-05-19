/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "template/complex_template4"
END_TEST_DATA
*/


span<int, 13> d = { 9 };

template <typename T> T getFirst(span<T, 13>& data)
{
    return data[0];
}


int main(int input)
{
  // check with span: maybe add ComplexType::getTemplateArguments...
    return getFirst(d);
}