/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "span/access_2d"
END_TEST_DATA
*/


#if 0
index::wrapped<3> i;
index::wrapped<2> j;
span<span<int, 2>, 3> data = { { 1, 2 }, { 3, 4}, {5, 6} };

int main(int input)
{
    i = (int)1.2;
    j = (int)1.2;
    auto& v = data[i];
    
    
    return v[j]; 
}
#endif

span<int, 7> data = { 1, 2, 3, 4, 5, 6, 7 };

index::unsafe<0, false> i;

int main(int input)
{
    

    return data[i];
}

