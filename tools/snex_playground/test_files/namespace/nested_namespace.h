/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "namespace/nested_namespace"
END_TEST_DATA
*/


namespace Types
{
    namespace RealTypes
    {
        using IntegerType = int;    
    }
}

Types::RealTypes::IntegerType main(Types::RealTypes::IntegerType input)
{
	return input;
}

