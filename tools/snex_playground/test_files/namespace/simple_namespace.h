/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "namespace/simple_namespace"
END_TEST_DATA
*/


namespace Types
{
    using IntegerType = int;
}

Types::IntegerType main(Types::IntegerType input)
{
	return input;
}

