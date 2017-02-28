# NativeJIT/src/NativeJIT

set(CPPFILES
  CallNode.cpp
  ExpressionNodeFactory.cpp
  ExpressionTree.cpp
  Node.cpp
)

set(PRIVATE_HFILES
)

set(PUBLIC_HFILES
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGenHelpers.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/ExecutionPreconditionTest.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/ExpressionNodeFactory.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/ExpressionNodeFactoryDecls.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/ExpressionTree.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/ExpressionTreeDecls.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Function.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Model.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/BinaryImmediateNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/BinaryNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/CallNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/CastNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ConditionalNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/FieldPointerNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ImmediateNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ImmediateNodeDecls.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/IndirectNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/Node.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/PackedMinMaxNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ParameterNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ReturnNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/ShldNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Nodes/StackVariableNode.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/Packed.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/TypePredicates.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/TypeConverter.h
)

source_group("inc/NativeJIT" FILES ${PUBLIC_HFILES})

add_library(NativeJIT ${CPPFILES} ${PRIVATE_HFILES} ${PUBLIC_HFILES})

set_property(TARGET NativeJIT PROPERTY FOLDER "${NATIVEJIT_PREFIX}src")

add_test(NAME NativeJITTest COMMAND NativeJITTest)
