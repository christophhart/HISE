/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12.0f
  output: 3.0f
  error: ""
  filename: "dyn/dyn_make_index"
END_TEST_DATA
*/

span<float, 6> s = { 1.0f, 2.0f, 3.0f, 4.f, 5.0f, 6.f };
dyn<float> d;

using WrapType = dyn<float>::wrapped;

float main(float input)
{
    d = s;

    return d[d.index<WrapType>(2)];
}

