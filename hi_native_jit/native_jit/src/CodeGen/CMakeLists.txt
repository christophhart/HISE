# NativeJIT/src/CodeGen

set(CPPFILES
  Allocator.cpp
  Assert.cpp
  CodeBuffer.cpp
  ExecutionBuffer.cpp
  FunctionBuffer.cpp
  FunctionSpecification.cpp
  JumpTable.cpp
  Register.cpp
  UnwindCode.cpp
  ValuePredicates.cpp
  X64CodeGenerator.cpp
)

set(WINDOWS_CPPFILES
)

set(POSIX_CPPFILES
)

set(PRIVATE_HFILES
  UnwindCode.h
)

set(PUBLIC_HFILES
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/AllocatorVector.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/BitOperations.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/CallingConvention.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/CodeBuffer.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/ExecutionBuffer.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/FunctionBuffer.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/FunctionSpecification.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/JumpTable.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/Register.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/ValuePredicates.h
  ${NativeJIT_SOURCE_DIR}/inc/NativeJIT/CodeGen/X64CodeGenerator.h
  ${NativeJIT_SOURCE_DIR}/inc/Temporary/Allocator.h
  ${NativeJIT_SOURCE_DIR}/inc/Temporary/AllocatorOperations.h
  ${NativeJIT_SOURCE_DIR}/inc/Temporary/Assert.h
  ${NativeJIT_SOURCE_DIR}/inc/Temporary/IAllocator.h
  ${NativeJIT_SOURCE_DIR}/inc/Temporary/StlAllocator.h
)

set(WINDOWS_PRIVATE_HFILES
)

set(WINDOWS_PUBLIC_HFILES
)

set(POSIX_PRIVATE_HFILES
)

set(POSIX_PUBLIC_HFILES
)

if (NATIVEJIT_PLATFORM_WINDOWS)
  set(CPPFILES ${CPPFILES} ${WINDOWS_CPPFILES})
  set(PUBLIC_HFILES ${PUBLIC_HFILES} ${WINDOWS_PUBLIC_HFILES})
  set(PRIVATE_HFILES ${PRIVATE_HFILES} ${WINDOWS_PRIVATE_HFILES})
else (NATIVEJIT_PLATFORM_WINDOWS)
  set(CPPFILES ${CPPFILES} ${POSIX_CPPFILES})
  set(PUBLIC_HFILES ${PUBLIC_HFILES} ${POSIX_PUBLIC_HFILES})
  set(PRIVATE_HFILES ${PRIVATE_HFILES} ${POSIX_PRIVATE_HFILES})
endif (NATIVEJIT_PLATFORM_WINDOWS)

source_group("inc/NativeJIT" FILES ${PUBLIC_HFILES})

add_library(CodeGen ${CPPFILES} ${PRIVATE_HFILES} ${PUBLIC_HFILES})

set_property(TARGET CodeGen PROPERTY FOLDER "${NATIVEJIT_PREFIX}src")
