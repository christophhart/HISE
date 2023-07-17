/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 3.0
  error: ""
  filename: "dyn/dyn_wrap_3"
END_TEST_DATA
*/

span<float, 6> s = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
dyn<float> d;

index::unsafe<0> i;

void assign()
{
    d.referTo(s, s.size());
}

float main(float input)
{
    assign();
    
    return d[i] + d[++i];
}

