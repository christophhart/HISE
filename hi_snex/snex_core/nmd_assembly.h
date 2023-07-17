/* This is a platform independent C89 x86 assembler and disassembler library.

Features:
 - Support for x86(16/32/64). Intel and AT&T syntax.
 - No libc, dynamic memory allocation, static/global variables/state/context or runtime initialization.
 - Thread-safe by design.
 - No header files need to be included.

Setup:
Define the 'NMD_ASSEMBLY_IMPLEMENTATION' macro in one source file before the include statement to instantiate the implementation.
#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"

Interfaces(i.e the functions you call from your application):
 - The assembler is implemented by the following function:
	Assembles an instruction from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using the '\n'(new line) character.
	Parameters:
	 - string          [in]         A pointer to a string that represents one or more instructions in assembly language.
	 - buffer          [out]        A pointer to a buffer that receives the encoded instructions.
	 - buffer_size     [in]         The size of the buffer in bytes.
	 - runtime_address [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
	 - mode            [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
	 - count           [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed(or zero for unlimited instructions), and on output is the number of instructions parsed. This parameter may be zero.
	size_t nmd_x86_assemble(const char* string, void* buffer, size_t buffer_size, uint64_t runtime_address, NMD_X86_MODE mode, size_t* const count);

 - The disassembler is composed of a decoder and a formatter implemented by these two functions respectively:
	- Decodes an instruction. Returns true if the instruction is valid, false otherwise.
	  Parameters:
	   - buffer      [in]  A pointer to a buffer containing an encoded instruction.
	   - buffer_size [in]  The size of the buffer in bytes.
	   - instruction [out] A pointer to a variable of type 'nmd_x86_instruction' that receives information about the instruction.
	   - mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
	   - flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
	  bool nmd_x86_decode(const void* buffer, size_t buffer_size, nmd_x86_instruction* instruction, NMD_X86_MODE mode, uint32_t flags);

	- Formats an instruction. This function may access invalid memory(thus causing a crash) if you modify 'instruction' manually.
	  Parameters:
	   - instruction     [in]  A pointer to a variable of type 'nmd_x86_instruction' describing the instruction to be formatted.
	   - buffer          [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
	   - runtime_address [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
	   - flags           [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
	  void nmd_x86_format(const nmd_x86_instruction* instruction, char buffer[], uint64_t runtime_address, uint32_t flags);

 - The length disassembler is implemented by the following function:
	Returns the length of the instruction if it is valid, zero otherwise.
	Parameters:
	 - buffer      [in] A pointer to a buffer containing an encoded instruction.
	 - buffer_size [in] The size of the buffer in bytes.
	 - mode        [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
	size_t nmd_x86_ldisasm(const void* buffer, size_t buffer_size, NMD_X86_MODE mode);

Enabling and disabling features of the decoder at compile-time:
To dynamically choose which features are used by the decoder, use the 'flags' parameter of nmd_x86_decode(). The less features specified in the mask, the
faster the decoder runs. By default all features are available, some can be completely disabled at compile time(thus reducing code size and increasing code speed) by defining
the following macros:
 - 'NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK': the decoder does not check if the instruction is invalid.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID': the decoder does not fill the 'id' variable.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS': the decoder does not fill the variables related to cpu fags.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS': the decoder does not fill the 'num_operands' and 'operands' variable.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_GROUP': the decoder does not fill the 'group' variable.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_VEX': the decoder does not support VEX instructions.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_EVEX': the decoder does not support EVEX instructions.
 - 'NMD_ASSEMBLY_DISABLE_DECODER_3DNOW': the decoder does not support 3DNow! instructions.

Enabling and disabling features of the formatter at compile-time:
To dynamically choose which features are used by the formatter, use the 'flags' parameter of nmd_x86_format(). The less features specified in the mask, the
faster the function runs. By default all features are available, some can be completely disabled at compile time(thus reducing code size and increasing code speed) by defining
the following macros:
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE': the formatter does not support pointer size.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES: the formatter does not support instruction bytes. You may define the 'NMD_X86_FORMATTER_NUM_PADDING_BYTES' macro to be the number of bytes used as space padding.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX: the formatter does not support AT&T syntax.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE: the formatter does not support uppercase.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES: the formatter does not support comma spaces.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES: the formatter does not support operator spaces.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_VEX': the formatter does not support VEX instructions.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX': the formatter does not support EVEX instructions.
 - 'NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW': the formatter does not support 3DNow! instructions.

Enabling and disabling features of the length disassembler at compile-time:
Use the following macros to disable features at compile-time:
 - 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK': the length disassembler does not check if the instruction is invalid.
 - 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX': the length disassembler does not support VEX instructions.
 - 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW': the length disassembler does not support 3DNow! instructions.

Fixed width integer types:
By default the library includes <stdint.h> and <stddef.h> to include int types.
If these header-files are not available in your environment you may define the 'NMD_DEFINE_INT_TYPES' macro so the library will define them.
By defining the 'NMD_IGNORE_INT_TYPES' macro, the library will neither include nor define int types.

You may define the 'NMD_ASSEMBLY_PRIVATE' macro to mark all functions as static so that they're not visible to other translation units.

Common helper functions:
Some 'nmd' libraries utilize the same functions such as '_nmd_assembly_get_num_digits' and '_nmd_assembly_get_num_digits_hex'.
All libraries include the same implementation which internally are defined as '_nmd_[LIBRARY_NAME]_[FUNCTION_NAME]' to avoid name conflits between them.
When the function is called a macro is used in the form '_NMD_FUNCTION_NAME', this allows another function to override the default implementation, so in
the case that two 'nmd' libraries need the same function, only one implementation will be used by both libraries. You may also provide your implemetation:
Example:
size_t my_get_num_digits(int x);
#define _NMD_GET_NUM_DIGITS my_get_num_digits

Shared helper functions used by this library:
 - _NMD_GET_NUM_DIGITS()
 - _NMD_GET_NUM_DIGITS_HEX()

Conventions:
 - Every identifier uses snake case.
 - Enums and macros are uppercase, every other identifier is lowercase.
 - Non-internal identifiers start with the 'NMD_' prefix.
 - Internal identifiers start with the '_NMD_' prefix.

TODO:
 Short-Term
  - Implement instruction set extensions to the decoder : VEX, EVEX, MVEX, 3DNOW, XOP.
  - Implement x86 assembler
 Long-Term
  - Add support for other architectures(ARM, MIPS and PowerPC ?).

References:
 - Intel 64 and IA-32 Architectures. Software Developer's Manual Volume 2 (2A, 2B, 2C & 2D): Instruction Set Reference, A-Z.
   - Chapter 2 Instruction Format.
   - Chapter 3-5 Instruction set reference.
   - Appendix A Opcode Map.
   - Appendix B.16 Instruction and Formats and Encoding.
 - 3DNow! Technology Manual.
 - AMD Extensions to the 3DNow! and MMX Instruction Sets Manual.
 - Intel Architecture Instruction Set Extensions and Future Features Programming Reference.
 - Capstone Engine.
 - Zydis Disassembler.
 - VIA PadLock Programming Guide.
 - Control registers - Wikipedia.

Contributors(This may not be a complete list):
 - Nomade: Founder and maintainer.
 - Darkratos: Bug reporting and feature suggesting.
*/

#ifndef NMD_ASSEMBLY_H
#define NMD_ASSEMBLY_H

#pragma warning( push )
#pragma warning( disable : 4505 )

#ifndef _NMD_DEFINE_INT_TYPES
#ifdef NMD_DEFINE_INT_TYPES
#define _NMD_DEFINE_INT_TYPES
#ifndef __cplusplus
#define bool  _Bool
#define false 0
#define true  1
#endif /* __cplusplus */
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;
#if defined(_WIN64) && defined(_MSC_VER)
typedef unsigned __int64 size_t;
typedef __int64          ptrdiff_t;
#elif (defined(_WIN32) || defined(WIN32)) && defined(_MSC_VER)
typedef unsigned __int32 size_t
typedef __int32          ptrdiff_t;
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__ppc64__)
typedef unsigned long size_t
typedef long          ptrdiff_t
#else
typedef unsigned int size_t
typedef int          ptrdiff_t
#endif
#else
typedef unsigned long size_t
typedef long          ptrdiff_t
#endif

#else /* NMD_DEFINE_INT_TYPES */
#ifndef NMD_IGNORE_INT_TYPES
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#endif /* NMD_IGNORE_INT_TYPES */
#endif /* NMD_DEFINE_INT_TYPES */
#endif /* _NMD_DEFINE_INT_TYPES */

#ifndef _NMD_GET_NUM_DIGITS
#define _NMD_GET_NUM_DIGITS _nmd_assembly_get_num_digits
#endif /* _NMD_GET_NUM_DIGITS */

#ifndef _NMD_GET_NUM_DIGITS_HEX
#define _NMD_GET_NUM_DIGITS_HEX _nmd_assembly_get_num_digits_hex
#endif /* _NMD_GET_NUM_DIGITS_HEX */

#ifndef NMD_X86_FORMATTER_NUM_PADDING_BYTES
#define NMD_X86_FORMATTER_NUM_PADDING_BYTES 10
#endif /* NMD_X86_FORMATTER_NUM_PADDING_BYTES */

#define NMD_X86_INVALID_RUNTIME_ADDRESS ((uint64_t)(-1))
#define NMD_X86_MAXIMUM_INSTRUCTION_LENGTH 15
#define NMD_X86_MAXIMUM_NUM_OPERANDS 10

/* Define the api macro to potentially change functions's attributes. */
#ifndef NMD_ASSEMBLY_API
#ifdef NMD_ASSEMBLY_PRIVATE
#define NMD_ASSEMBLY_API static
#else
#define NMD_ASSEMBLY_API
#endif /* NMD_ASSEMBLY_PRIVATE */
#endif /* NMD_ASSEMBLY_API */

/* These flags specify how the formatter should work. */
enum NMD_X86_FORMATTER_FLAGS
{
	NMD_X86_FORMAT_FLAGS_HEX = (1 << 0),  /* If set, numbers are displayed in hex base, otherwise they are displayed in decimal base. */
	NMD_X86_FORMAT_FLAGS_POINTER_SIZE = (1 << 1),  /* Pointer sizes(e.g. 'dword ptr', 'byte ptr') are displayed. */
	NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE = (1 << 2),  /* If set, only segment overrides using prefixes(e.g. '2EH', '64H') are displayed, otherwise a segment is always present before a memory operand. */
	NMD_X86_FORMAT_FLAGS_COMMA_SPACES = (1 << 3),  /* A space is placed after a comma. */
	NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES = (1 << 4),  /* A space is placed before and after the '+' and '-' characters. */
	NMD_X86_FORMAT_FLAGS_UPPERCASE = (1 << 5),  /* The string is uppercase. */
	NMD_X86_FORMAT_FLAGS_0X_PREFIX = (1 << 6),  /* Hexadecimal numbers have the '0x'('0X' if uppercase) prefix. */
	NMD_X86_FORMAT_FLAGS_H_SUFFIX = (1 << 7),  /* Hexadecimal numbers have the 'h'('H' if uppercase') suffix. */
	NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID = (1 << 8),  /* If the HEX flag is set and either the prefix or suffix flag is also set, numbers less than 10 are displayed with preffix or suffix. */
	NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE = (1 << 9),  /* If the HEX flag is set and the UPPERCASE flag is not set, hexadecimal numbers are displayed in lowercase. */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW = (1 << 10), /* If set, signed numbers are displayed as they are represented in memory(e.g. -1 = 0xFFFFFFFF). */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX = (1 << 11), /* If set and NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW is also set, the number's hexadecimal representation is displayed in parenthesis. */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC = (1 << 12), /* Same as NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX, but the number is displayed in decimal base. */
	NMD_X86_FORMAT_FLAGS_SCALE_ONE = (1 << 13), /* If set, scale one is displayed. E.g. add byte ptr [eax+eax*1], al. */
	NMD_X86_FORMAT_FLAGS_BYTES = (1 << 14), /* The instruction's bytes are displayed before the instructions. */
	NMD_X86_FORMAT_FLAGS_ATT_SYNTAX = (1 << 15), /* AT&T syntax is used instead of Intel's. */

	/* The formatter's default formatting style. */
	NMD_X86_FORMAT_FLAGS_DEFAULT = (NMD_X86_FORMAT_FLAGS_HEX | NMD_X86_FORMAT_FLAGS_H_SUFFIX | NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC),
};

enum NMD_X86_DECODER_FLAGS
{
	NMD_X86_DECODER_FLAGS_VALIDITY_CHECK = (1 << 0), /* The decoder checks if the instruction is valid. */
	NMD_X86_DECODER_FLAGS_INSTRUCTION_ID = (1 << 1), /* The decoder fills the 'id' variable. */
	NMD_X86_DECODER_FLAGS_CPU_FLAGS = (1 << 2), /* The decoder fills the variables related to cpu flags. */
	NMD_X86_DECODER_FLAGS_OPERANDS = (1 << 3), /* The decoder fills the 'num_operands' and 'operands' variable. */
	NMD_X86_DECODER_FLAGS_GROUP = (1 << 4), /* The decoder fills 'group' variable. */
	NMD_X86_DECODER_FLAGS_VEX = (1 << 5), /* The decoder parses VEX instructions. */
	NMD_X86_DECODER_FLAGS_EVEX = (1 << 6), /* The decoder parses EVEX instructions. */
	NMD_X86_DECODER_FLAGS_3DNOW = (1 << 7), /* The decoder parses 3DNow! instructions. */

	/* These are not actual features, but rather masks of features. */
	NMD_X86_DECODER_FLAGS_NONE = 0,
	NMD_X86_DECODER_FLAGS_MINIMAL = (NMD_X86_DECODER_FLAGS_VALIDITY_CHECK | NMD_X86_DECODER_FLAGS_VEX | NMD_X86_DECODER_FLAGS_EVEX), /* Mask that specifies minimal features to provide acurate results in any environment. */
	NMD_X86_DECODER_FLAGS_ALL = (1 << 8) - 1, /* Mask that specifies all features. */
};

enum NMD_X86_PREFIXES
{
	NMD_X86_PREFIXES_NONE = 0,
	NMD_X86_PREFIXES_ES_SEGMENT_OVERRIDE = (1 << 0),
	NMD_X86_PREFIXES_CS_SEGMENT_OVERRIDE = (1 << 1),
	NMD_X86_PREFIXES_SS_SEGMENT_OVERRIDE = (1 << 2),
	NMD_X86_PREFIXES_DS_SEGMENT_OVERRIDE = (1 << 3),
	NMD_X86_PREFIXES_FS_SEGMENT_OVERRIDE = (1 << 4),
	NMD_X86_PREFIXES_GS_SEGMENT_OVERRIDE = (1 << 5),
	NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE = (1 << 6),
	NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE = (1 << 7),
	NMD_X86_PREFIXES_LOCK = (1 << 8),
	NMD_X86_PREFIXES_REPEAT_NOT_ZERO = (1 << 9),
	NMD_X86_PREFIXES_REPEAT = (1 << 10),
	NMD_X86_PREFIXES_REX_W = (1 << 11),
	NMD_X86_PREFIXES_REX_R = (1 << 12),
	NMD_X86_PREFIXES_REX_X = (1 << 13),
	NMD_X86_PREFIXES_REX_B = (1 << 14)
};

enum NMD_X86_IMM
{
	NMD_X86_IMM_NONE = 0,
	NMD_X86_IMM8 = 1,
	NMD_X86_IMM16 = 2,
	NMD_X86_IMM32 = 4,
	NMD_X86_IMM48 = 6,
	NMD_X86_IMM64 = 8,
	NMD_X86_IMM_ANY = (NMD_X86_IMM8 | NMD_X86_IMM16 | NMD_X86_IMM32 | NMD_X86_IMM64)
};

enum NMD_X86_DISP
{
	NMD_X86_DISP_NONE = 0,
	NMD_X86_DISP8 = 1,
	NMD_X86_DISP16 = 2,
	NMD_X86_DISP32 = 4,
	NMD_X86_DISP64 = 8,
	NMD_X86_DISP_ANY = (NMD_X86_DISP8 | NMD_X86_DISP16 | NMD_X86_DISP32)
};

typedef union nmd_x86_modrm
{
	struct
	{
		uint8_t rm : 3;
		uint8_t reg : 3;
		uint8_t mod : 2;
	} fields;
	uint8_t modrm;
} nmd_x86_modrm;

typedef union nmd_x86_sib
{
	struct
	{
		uint8_t base : 3;
		uint8_t index : 3;
		uint8_t scale : 2;
	} fields;
	uint8_t sib;
} nmd_x86_sib;

typedef enum NMD_X86_MODE
{
	NMD_X86_MODE_NONE = 0, /* Invalid mode. */
	NMD_X86_MODE_16 = 2,
	NMD_X86_MODE_32 = 4,
	NMD_X86_MODE_64 = 8,
} NMD_X86_MODE;

enum NMD_X86_OPCODE_MAP
{
	NMD_X86_OPCODE_MAP_NONE = 0,
	NMD_X86_OPCODE_MAP_DEFAULT,
	NMD_X86_OPCODE_MAP_0F,
	NMD_X86_OPCODE_MAP_0F38,
	NMD_X86_OPCODE_MAP_0F3A,
	NMD_X86_OPCODE_MAP_0F0F
};

enum NMD_X86_ENCODING
{
	NMD_X86_ENCODING_NONE = 0,
	NMD_X86_ENCODING_LEGACY,  /* Legacy encoding. */
	NMD_X86_ENCODING_VEX,     /* Intel's VEX(vector extensions) coding scheme. */
	NMD_X86_ENCODING_EVEX,    /* Intel's EVEX(Enhanced vector extension) coding scheme. */
	NMD_X86_ENCODING_3DNOW,   /* AMD's 3DNow! extension. */
	NMD_X86_ENCODING_XOP,     /* AMD's XOP(eXtended Operations) instruction set. */
	/* NMD_X86_ENCODING_MVEX,    MVEX used by Intel's "Xeon Phi" ISA. */
};

typedef struct nmd_x86_vex
{
	bool R : 1;
	bool X : 1;
	bool B : 1;
	bool L : 1;
	bool W : 1;
	uint8_t pp : 2;
	uint8_t m_mmmm : 5;
	uint8_t vvvv : 4;
	uint8_t vex[3]; /* The full vex prefix. vex[0] is either C4h(3-byte VEX) or C5h(2-byte VEX).*/
} nmd_x86_vex;

enum NMD_GROUP
{
	NMD_GROUP_NONE = 0, /* The instruction is not part of any group. */

	NMD_GROUP_JUMP = (1 << 0), /* All jump instructions. */
	NMD_GROUP_CALL = (1 << 1), /* Call instruction. */
	NMD_GROUP_RET = (1 << 2), /* Return instruction. */
	NMD_GROUP_INT = (1 << 3), /* Interrupt instruction. */
	NMD_GROUP_PRIVILEGE = (1 << 4), /* Privileged instruction. */
	NMD_GROUP_CONDITIONAL_BRANCH = (1 << 5), /* Conditional branch instruction. */
	NMD_GROUP_UNCONDITIONAL_BRANCH = (1 << 6), /* Unconditional branch instruction. */
	NMD_GROUP_RELATIVE_ADDRESSING = (1 << 7), /* Relative addressing instruction. */

	/* These are not actual groups, but rather masks of groups. */
	NMD_GROUP_BRANCH = (NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_UNCONDITIONAL_BRANCH), /* Mask used to check if the instruction is a branch instruction. */
	NMD_GROUP_ANY = (1 << 8) - 1, /* Mask used to check if the instruction is part of any group. */
};

/* The enums for a some classes of registers always start at a multiple of eight */
typedef enum NMD_X86_REG
{
	NMD_X86_REG_NONE = 0,

	NMD_X86_REG_IP = 8,
	NMD_X86_REG_EIP = 9,
	NMD_X86_REG_RIP = 10,

	NMD_X86_REG_AL = 16,
	NMD_X86_REG_CL = 17,
	NMD_X86_REG_DL = 18,
	NMD_X86_REG_BL = 19,
	NMD_X86_REG_AH = 20,
	NMD_X86_REG_CH = 21,
	NMD_X86_REG_DH = 22,
	NMD_X86_REG_BH = 23,

	NMD_X86_REG_AX = 24,
	NMD_X86_REG_CX = 25,
	NMD_X86_REG_DX = 26,
	NMD_X86_REG_BX = 27,
	NMD_X86_REG_SP = 28,
	NMD_X86_REG_BP = 29,
	NMD_X86_REG_SI = 30,
	NMD_X86_REG_DI = 31,

	NMD_X86_REG_EAX = 32,
	NMD_X86_REG_ECX = 33,
	NMD_X86_REG_EDX = 34,
	NMD_X86_REG_EBX = 35,
	NMD_X86_REG_ESP = 36,
	NMD_X86_REG_EBP = 37,
	NMD_X86_REG_ESI = 38,
	NMD_X86_REG_EDI = 39,

	NMD_X86_REG_RAX = 40,
	NMD_X86_REG_RCX = 41,
	NMD_X86_REG_RDX = 42,
	NMD_X86_REG_RBX = 43,
	NMD_X86_REG_RSP = 44,
	NMD_X86_REG_RBP = 45,
	NMD_X86_REG_RSI = 46,
	NMD_X86_REG_RDI = 47,

	NMD_X86_REG_R8 = 48,
	NMD_X86_REG_R9 = 49,
	NMD_X86_REG_R10 = 50,
	NMD_X86_REG_R11 = 51,
	NMD_X86_REG_R12 = 52,
	NMD_X86_REG_R13 = 53,
	NMD_X86_REG_R14 = 54,
	NMD_X86_REG_R15 = 55,

	NMD_X86_REG_R8B = 56,
	NMD_X86_REG_R9B = 57,
	NMD_X86_REG_R10B = 58,
	NMD_X86_REG_R11B = 59,
	NMD_X86_REG_R12B = 60,
	NMD_X86_REG_R13B = 61,
	NMD_X86_REG_R14B = 62,
	NMD_X86_REG_R15B = 63,

	NMD_X86_REG_R8W = 64,
	NMD_X86_REG_R9W = 65,
	NMD_X86_REG_R10W = 66,
	NMD_X86_REG_R11W = 67,
	NMD_X86_REG_R12W = 68,
	NMD_X86_REG_R13W = 69,
	NMD_X86_REG_R14W = 70,
	NMD_X86_REG_R15W = 71,

	NMD_X86_REG_R8D = 72,
	NMD_X86_REG_R9D = 73,
	NMD_X86_REG_R10D = 74,
	NMD_X86_REG_R11D = 75,
	NMD_X86_REG_R12D = 76,
	NMD_X86_REG_R13D = 77,
	NMD_X86_REG_R14D = 78,
	NMD_X86_REG_R15D = 79,

	NMD_X86_REG_ES = 80,
	NMD_X86_REG_CS = 81,
	NMD_X86_REG_SS = 82,
	NMD_X86_REG_DS = 83,
	NMD_X86_REG_FS = 84,
	NMD_X86_REG_GS = 85,

	NMD_X86_REG_CR0 = 88,
	NMD_X86_REG_CR1 = 89,
	NMD_X86_REG_CR2 = 90,
	NMD_X86_REG_CR3 = 91,
	NMD_X86_REG_CR4 = 92,
	NMD_X86_REG_CR5 = 93,
	NMD_X86_REG_CR6 = 94,
	NMD_X86_REG_CR7 = 95,
	NMD_X86_REG_CR8 = 96,
	NMD_X86_REG_CR9 = 97,
	NMD_X86_REG_CR10 = 98,
	NMD_X86_REG_CR11 = 99,
	NMD_X86_REG_CR12 = 100,
	NMD_X86_REG_CR13 = 101,
	NMD_X86_REG_CR14 = 102,
	NMD_X86_REG_CR15 = 103,

	NMD_X86_REG_DR0 = 104,
	NMD_X86_REG_DR1 = 105,
	NMD_X86_REG_DR2 = 106,
	NMD_X86_REG_DR3 = 107,
	NMD_X86_REG_DR4 = 108,
	NMD_X86_REG_DR5 = 109,
	NMD_X86_REG_DR6 = 110,
	NMD_X86_REG_DR7 = 111,
	NMD_X86_REG_DR8 = 112,
	NMD_X86_REG_DR9 = 113,
	NMD_X86_REG_DR10 = 114,
	NMD_X86_REG_DR11 = 115,
	NMD_X86_REG_DR12 = 116,
	NMD_X86_REG_DR13 = 117,
	NMD_X86_REG_DR14 = 118,
	NMD_X86_REG_DR15 = 119,

	NMD_X86_REG_MM0 = 120,
	NMD_X86_REG_MM1 = 121,
	NMD_X86_REG_MM2 = 122,
	NMD_X86_REG_MM3 = 123,
	NMD_X86_REG_MM4 = 124,
	NMD_X86_REG_MM5 = 125,
	NMD_X86_REG_MM6 = 126,
	NMD_X86_REG_MM7 = 127,

	NMD_X86_REG_XMM0 = 128,
	NMD_X86_REG_XMM1 = 129,
	NMD_X86_REG_XMM2 = 130,
	NMD_X86_REG_XMM3 = 131,
	NMD_X86_REG_XMM4 = 132,
	NMD_X86_REG_XMM5 = 133,
	NMD_X86_REG_XMM6 = 134,
	NMD_X86_REG_XMM7 = 135,
	NMD_X86_REG_XMM8 = 136,
	NMD_X86_REG_XMM9 = 137,
	NMD_X86_REG_XMM10 = 138,
	NMD_X86_REG_XMM11 = 139,
	NMD_X86_REG_XMM12 = 140,
	NMD_X86_REG_XMM13 = 141,
	NMD_X86_REG_XMM14 = 142,
	NMD_X86_REG_XMM15 = 143,
	NMD_X86_REG_XMM16 = 144,
	NMD_X86_REG_XMM17 = 145,
	NMD_X86_REG_XMM18 = 146,
	NMD_X86_REG_XMM19 = 147,
	NMD_X86_REG_XMM20 = 148,
	NMD_X86_REG_XMM21 = 149,
	NMD_X86_REG_XMM22 = 150,
	NMD_X86_REG_XMM23 = 151,
	NMD_X86_REG_XMM24 = 152,
	NMD_X86_REG_XMM25 = 153,
	NMD_X86_REG_XMM26 = 154,
	NMD_X86_REG_XMM27 = 155,
	NMD_X86_REG_XMM28 = 156,
	NMD_X86_REG_XMM29 = 157,
	NMD_X86_REG_XMM30 = 158,
	NMD_X86_REG_XMM31 = 159,

	NMD_X86_REG_YMM0 = 160,
	NMD_X86_REG_YMM1 = 161,
	NMD_X86_REG_YMM2 = 162,
	NMD_X86_REG_YMM3 = 163,
	NMD_X86_REG_YMM4 = 164,
	NMD_X86_REG_YMM5 = 165,
	NMD_X86_REG_YMM6 = 166,
	NMD_X86_REG_YMM7 = 167,
	NMD_X86_REG_YMM8 = 168,
	NMD_X86_REG_YMM9 = 169,
	NMD_X86_REG_YMM10 = 170,
	NMD_X86_REG_YMM11 = 171,
	NMD_X86_REG_YMM12 = 172,
	NMD_X86_REG_YMM13 = 173,
	NMD_X86_REG_YMM14 = 174,
	NMD_X86_REG_YMM15 = 175,
	NMD_X86_REG_YMM16 = 176,
	NMD_X86_REG_YMM17 = 177,
	NMD_X86_REG_YMM18 = 178,
	NMD_X86_REG_YMM19 = 179,
	NMD_X86_REG_YMM20 = 180,
	NMD_X86_REG_YMM21 = 181,
	NMD_X86_REG_YMM22 = 182,
	NMD_X86_REG_YMM23 = 183,
	NMD_X86_REG_YMM24 = 184,
	NMD_X86_REG_YMM25 = 185,
	NMD_X86_REG_YMM26 = 186,
	NMD_X86_REG_YMM27 = 187,
	NMD_X86_REG_YMM28 = 188,
	NMD_X86_REG_YMM29 = 189,
	NMD_X86_REG_YMM30 = 190,
	NMD_X86_REG_YMM31 = 191,

	NMD_X86_REG_ZMM0 = 192,
	NMD_X86_REG_ZMM1 = 193,
	NMD_X86_REG_ZMM2 = 194,
	NMD_X86_REG_ZMM3 = 195,
	NMD_X86_REG_ZMM4 = 196,
	NMD_X86_REG_ZMM5 = 197,
	NMD_X86_REG_ZMM6 = 198,
	NMD_X86_REG_ZMM7 = 199,
	NMD_X86_REG_ZMM8 = 200,
	NMD_X86_REG_ZMM9 = 201,
	NMD_X86_REG_ZMM10 = 202,
	NMD_X86_REG_ZMM11 = 203,
	NMD_X86_REG_ZMM12 = 204,
	NMD_X86_REG_ZMM13 = 205,
	NMD_X86_REG_ZMM14 = 206,
	NMD_X86_REG_ZMM15 = 207,
	NMD_X86_REG_ZMM16 = 208,
	NMD_X86_REG_ZMM17 = 209,
	NMD_X86_REG_ZMM18 = 210,
	NMD_X86_REG_ZMM19 = 211,
	NMD_X86_REG_ZMM20 = 212,
	NMD_X86_REG_ZMM21 = 213,
	NMD_X86_REG_ZMM22 = 214,
	NMD_X86_REG_ZMM23 = 215,
	NMD_X86_REG_ZMM24 = 216,
	NMD_X86_REG_ZMM25 = 217,
	NMD_X86_REG_ZMM26 = 218,
	NMD_X86_REG_ZMM27 = 219,
	NMD_X86_REG_ZMM28 = 220,
	NMD_X86_REG_ZMM29 = 221,
	NMD_X86_REG_ZMM30 = 222,
	NMD_X86_REG_ZMM31 = 223,

	NMD_X86_REG_K0 = 224,
	NMD_X86_REG_K1 = 225,
	NMD_X86_REG_K2 = 226,
	NMD_X86_REG_K3 = 227,
	NMD_X86_REG_K4 = 228,
	NMD_X86_REG_K5 = 229,
	NMD_X86_REG_K6 = 230,
	NMD_X86_REG_K7 = 231,

	NMD_X86_REG_ST0 = 232,
	NMD_X86_REG_ST1 = 233,
	NMD_X86_REG_ST2 = 234,
	NMD_X86_REG_ST3 = 235,
	NMD_X86_REG_ST4 = 236,
	NMD_X86_REG_ST5 = 237,
	NMD_X86_REG_ST6 = 238,
	NMD_X86_REG_ST7 = 239,
} NMD_X86_REG;

/*
Credits to the capstone engine:
Some members of the enum are organized in such a way because the instruction's id parsing component of the decoder can take advantage of it.
If an instruction as marked as 'padding', it means that it's being used to fill holes between instructions organized in a special way for optimization reasons.
*/
enum NMD_X86_INSTRUCTION
{
	NMD_X86_INSTRUCTION_INVALID = 0,

	/* Optimized for opcode extension group 1. */
	NMD_X86_INSTRUCTION_ADD,
	NMD_X86_INSTRUCTION_OR,
	NMD_X86_INSTRUCTION_ADC,
	NMD_X86_INSTRUCTION_SBB,
	NMD_X86_INSTRUCTION_AND,
	NMD_X86_INSTRUCTION_SUB,
	NMD_X86_INSTRUCTION_XOR,
	NMD_X86_INSTRUCTION_CMP,

	/* Optimized for opcode extension group 2. */
	NMD_X86_INSTRUCTION_ROL,
	NMD_X86_INSTRUCTION_ROR,
	NMD_X86_INSTRUCTION_RCL,
	NMD_X86_INSTRUCTION_RCR,
	NMD_X86_INSTRUCTION_SHL,
	NMD_X86_INSTRUCTION_SHR,
	NMD_X86_INSTRUCTION_AAA, /* padding */
	NMD_X86_INSTRUCTION_SAR,

	/* Optimized for opcode extension group 3. */
	NMD_X86_INSTRUCTION_TEST,
	NMD_X86_INSTRUCTION_BLSFILL, /* pading */
	NMD_X86_INSTRUCTION_NOT,
	NMD_X86_INSTRUCTION_NEG,
	NMD_X86_INSTRUCTION_MUL,
	NMD_X86_INSTRUCTION_IMUL,
	NMD_X86_INSTRUCTION_DIV,
	NMD_X86_INSTRUCTION_IDIV,

	/* Optimized for opcode extension group 5. */
	NMD_X86_INSTRUCTION_INC,
	NMD_X86_INSTRUCTION_DEC,
	NMD_X86_INSTRUCTION_CALL,
	NMD_X86_INSTRUCTION_LCALL,
	NMD_X86_INSTRUCTION_JMP,
	NMD_X86_INSTRUCTION_LJMP,
	NMD_X86_INSTRUCTION_PUSH,

	/* Optimized for the 7th row of the 1 byte opcode map and the 8th row of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_JO,
	NMD_X86_INSTRUCTION_JNO,
	NMD_X86_INSTRUCTION_JB,
	NMD_X86_INSTRUCTION_JNB,
	NMD_X86_INSTRUCTION_JZ,
	NMD_X86_INSTRUCTION_JNZ,
	NMD_X86_INSTRUCTION_JBE,
	NMD_X86_INSTRUCTION_JA,
	NMD_X86_INSTRUCTION_JS,
	NMD_X86_INSTRUCTION_JNS,
	NMD_X86_INSTRUCTION_JP,
	NMD_X86_INSTRUCTION_JNP,
	NMD_X86_INSTRUCTION_JL,
	NMD_X86_INSTRUCTION_JGE,
	NMD_X86_INSTRUCTION_JLE,
	NMD_X86_INSTRUCTION_JG,

	/* Optimized for escape opcodes with D8 as first byte. */
	NMD_X86_INSTRUCTION_FADD,
	NMD_X86_INSTRUCTION_FMUL,
	NMD_X86_INSTRUCTION_FCOM,
	NMD_X86_INSTRUCTION_FCOMP,
	NMD_X86_INSTRUCTION_FSUB,
	NMD_X86_INSTRUCTION_FSUBR,
	NMD_X86_INSTRUCTION_FDIV,
	NMD_X86_INSTRUCTION_FDIVR,

	/* Optimized for escape opcodes with D9 as first byte. */
	NMD_X86_INSTRUCTION_FLD,
	NMD_X86_INSTRUCTION_ADOX, /* padding */
	NMD_X86_INSTRUCTION_FST,
	NMD_X86_INSTRUCTION_FSTP,
	NMD_X86_INSTRUCTION_FLDENV,
	NMD_X86_INSTRUCTION_FLDCW,
	NMD_X86_INSTRUCTION_FNSTENV,
	NMD_X86_INSTRUCTION_FNSTCW,

	NMD_X86_INSTRUCTION_FCHS,
	NMD_X86_INSTRUCTION_FABS,
	NMD_X86_INSTRUCTION_AAS, /* padding */
	NMD_X86_INSTRUCTION_ADCX, /* padding */
	NMD_X86_INSTRUCTION_FTST,
	NMD_X86_INSTRUCTION_FXAM,
	NMD_X86_INSTRUCTION_RET, /* padding */
	NMD_X86_INSTRUCTION_ENTER, /* padding */
	NMD_X86_INSTRUCTION_FLD1,
	NMD_X86_INSTRUCTION_FLDL2T,
	NMD_X86_INSTRUCTION_FLDL2E,
	NMD_X86_INSTRUCTION_FLDPI,
	NMD_X86_INSTRUCTION_FLDLG2,
	NMD_X86_INSTRUCTION_FLDLN2,
	NMD_X86_INSTRUCTION_FLDZ,
	NMD_X86_INSTRUCTION_FNOP, /* padding */
	NMD_X86_INSTRUCTION_F2XM1,
	NMD_X86_INSTRUCTION_FYL2X,
	NMD_X86_INSTRUCTION_FPTAN,
	NMD_X86_INSTRUCTION_FPATAN,
	NMD_X86_INSTRUCTION_FXTRACT,
	NMD_X86_INSTRUCTION_FPREM1,
	NMD_X86_INSTRUCTION_FDECSTP,
	NMD_X86_INSTRUCTION_FINCSTP,
	NMD_X86_INSTRUCTION_FPREM,
	NMD_X86_INSTRUCTION_FYL2XP1,
	NMD_X86_INSTRUCTION_FSQRT,
	NMD_X86_INSTRUCTION_FSINCOS,
	NMD_X86_INSTRUCTION_FRNDINT,
	NMD_X86_INSTRUCTION_FSCALE,
	NMD_X86_INSTRUCTION_FSIN,
	NMD_X86_INSTRUCTION_FCOS,

	/* Optimized for escape opcodes with DA as first byte. */
	NMD_X86_INSTRUCTION_FIADD,
	NMD_X86_INSTRUCTION_FIMUL,
	NMD_X86_INSTRUCTION_FICOM,
	NMD_X86_INSTRUCTION_FICOMP,
	NMD_X86_INSTRUCTION_FISUB,
	NMD_X86_INSTRUCTION_FISUBR,
	NMD_X86_INSTRUCTION_FIDIV,
	NMD_X86_INSTRUCTION_FIDIVR,

	NMD_X86_INSTRUCTION_FCMOVB,
	NMD_X86_INSTRUCTION_FCMOVE,
	NMD_X86_INSTRUCTION_FCMOVBE,
	NMD_X86_INSTRUCTION_FCMOVU,

	/* Optimized for escape opcodes with DB/DF as first byte. */
	NMD_X86_INSTRUCTION_FILD,
	NMD_X86_INSTRUCTION_FISTTP,
	NMD_X86_INSTRUCTION_FIST,
	NMD_X86_INSTRUCTION_FISTP,
	NMD_X86_INSTRUCTION_FBLD,
	NMD_X86_INSTRUCTION_AESKEYGENASSIST, /* padding */
	NMD_X86_INSTRUCTION_FBSTP,
	NMD_X86_INSTRUCTION_ANDN, /* padding */

	NMD_X86_INSTRUCTION_FCMOVNB,
	NMD_X86_INSTRUCTION_FCMOVNE,
	NMD_X86_INSTRUCTION_FCMOVNBE,
	NMD_X86_INSTRUCTION_FCMOVNU,
	NMD_X86_INSTRUCTION_FNCLEX,
	NMD_X86_INSTRUCTION_FUCOMI,
	NMD_X86_INSTRUCTION_FCOMI,

	/* Optimized for escape opcodes with DE as first byte. */
	NMD_X86_INSTRUCTION_FADDP,
	NMD_X86_INSTRUCTION_FMULP,
	NMD_X86_INSTRUCTION_MOVAPD, /* padding */
	NMD_X86_INSTRUCTION_BNDCN, /* padding */
	NMD_X86_INSTRUCTION_FSUBRP,
	NMD_X86_INSTRUCTION_FSUBP,
	NMD_X86_INSTRUCTION_FDIVRP,
	NMD_X86_INSTRUCTION_FDIVP,

	/* Optimized for the 15th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_INT1,
	NMD_X86_INSTRUCTION_BSR, /* padding */
	NMD_X86_INSTRUCTION_ADDSUBPD, /* padding */
	NMD_X86_INSTRUCTION_HLT,
	NMD_X86_INSTRUCTION_CMC,
	NMD_X86_INSTRUCTION_ADDSUBPS, /* padding */
	NMD_X86_INSTRUCTION_BLENDVPD, /* padding*/
	NMD_X86_INSTRUCTION_CLC,
	NMD_X86_INSTRUCTION_STC,
	NMD_X86_INSTRUCTION_CLI,
	NMD_X86_INSTRUCTION_STI,
	NMD_X86_INSTRUCTION_CLD,
	NMD_X86_INSTRUCTION_STD,

	/* Optimized for the 13th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_AAM,
	NMD_X86_INSTRUCTION_AAD,
	NMD_X86_INSTRUCTION_SALC,
	NMD_X86_INSTRUCTION_XLAT,

	/* Optimized for the 14th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_LOOPNE,
	NMD_X86_INSTRUCTION_LOOPE,
	NMD_X86_INSTRUCTION_LOOP,
	NMD_X86_INSTRUCTION_JRCXZ,

	/* Optimized for opcode extension group 6. */
	NMD_X86_INSTRUCTION_SLDT,
	NMD_X86_INSTRUCTION_STR,
	NMD_X86_INSTRUCTION_LLDT,
	NMD_X86_INSTRUCTION_LTR,
	NMD_X86_INSTRUCTION_VERR,
	NMD_X86_INSTRUCTION_VERW,

	/* Optimized for opcode extension group 7. */
	NMD_X86_INSTRUCTION_SGDT,
	NMD_X86_INSTRUCTION_SIDT,
	NMD_X86_INSTRUCTION_LGDT,
	NMD_X86_INSTRUCTION_LIDT,
	NMD_X86_INSTRUCTION_SMSW,
	NMD_X86_INSTRUCTION_CLWB, /* padding */
	NMD_X86_INSTRUCTION_LMSW,
	NMD_X86_INSTRUCTION_INVLPG,

	NMD_X86_INSTRUCTION_VMCALL,
	NMD_X86_INSTRUCTION_VMLAUNCH,
	NMD_X86_INSTRUCTION_VMRESUME,
	NMD_X86_INSTRUCTION_VMXOFF,

	NMD_X86_INSTRUCTION_MONITOR,
	NMD_X86_INSTRUCTION_MWAIT,
	NMD_X86_INSTRUCTION_CLAC,
	NMD_X86_INSTRUCTION_STAC,
	NMD_X86_INSTRUCTION_CBW, /* padding */
	NMD_X86_INSTRUCTION_CMPSB, /* padding */
	NMD_X86_INSTRUCTION_CMPSQ, /* padding */
	NMD_X86_INSTRUCTION_ENCLS,

	NMD_X86_INSTRUCTION_XGETBV,
	NMD_X86_INSTRUCTION_XSETBV,
	NMD_X86_INSTRUCTION_ARPL, /* padding */
	NMD_X86_INSTRUCTION_BEXTR, /* padding */
	NMD_X86_INSTRUCTION_VMFUNC,
	NMD_X86_INSTRUCTION_XEND,
	NMD_X86_INSTRUCTION_XTEST,
	NMD_X86_INSTRUCTION_ENCLU,

	NMD_X86_INSTRUCTION_VMRUN,
	NMD_X86_INSTRUCTION_VMMCALL,
	NMD_X86_INSTRUCTION_VMLOAD,
	NMD_X86_INSTRUCTION_VMSAVE,
	NMD_X86_INSTRUCTION_STGI,
	NMD_X86_INSTRUCTION_CLGI,
	NMD_X86_INSTRUCTION_SKINIT,
	NMD_X86_INSTRUCTION_INVLPGA,

	/* Optimized for the row 0x0 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_LAR,
	NMD_X86_INSTRUCTION_LSL,
	NMD_X86_INSTRUCTION_BLCFILL, /* padding */
	NMD_X86_INSTRUCTION_SYSCALL,
	NMD_X86_INSTRUCTION_CLTS,
	NMD_X86_INSTRUCTION_SYSRET,
	NMD_X86_INSTRUCTION_INVD,
	NMD_X86_INSTRUCTION_WBINVD,
	NMD_X86_INSTRUCTION_BLCI, /* padding */
	NMD_X86_INSTRUCTION_UD2,
	NMD_X86_INSTRUCTION_PREFETCHW,
	NMD_X86_INSTRUCTION_FEMMS,

	/* Optimized for the row 0x3 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_WRMSR,
	NMD_X86_INSTRUCTION_RDTSC,
	NMD_X86_INSTRUCTION_RDMSR,
	NMD_X86_INSTRUCTION_RDPMC,
	NMD_X86_INSTRUCTION_SYSENTER,
	NMD_X86_INSTRUCTION_SYSEXIT,
	NMD_X86_INSTRUCTION_BLCIC, /* padding */
	NMD_X86_INSTRUCTION_GETSEC,

	/* Optimized for the row 0x4 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_CMOVO,
	NMD_X86_INSTRUCTION_CMOVNO,
	NMD_X86_INSTRUCTION_CMOVB,
	NMD_X86_INSTRUCTION_CMOVAE,
	NMD_X86_INSTRUCTION_CMOVE,
	NMD_X86_INSTRUCTION_CMOVNE,
	NMD_X86_INSTRUCTION_CMOVBE,
	NMD_X86_INSTRUCTION_CMOVA,
	NMD_X86_INSTRUCTION_CMOVS,
	NMD_X86_INSTRUCTION_CMOVNS,
	NMD_X86_INSTRUCTION_CMOVP,
	NMD_X86_INSTRUCTION_CMOVNP,
	NMD_X86_INSTRUCTION_CMOVL,
	NMD_X86_INSTRUCTION_CMOVGE,
	NMD_X86_INSTRUCTION_CMOVLE,
	NMD_X86_INSTRUCTION_CMOVG,

	/* Optimized for the row 0x9 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_SETO,
	NMD_X86_INSTRUCTION_SETNO,
	NMD_X86_INSTRUCTION_SETB,
	NMD_X86_INSTRUCTION_SETAE,
	NMD_X86_INSTRUCTION_SETE,
	NMD_X86_INSTRUCTION_SETNE,
	NMD_X86_INSTRUCTION_SETBE,
	NMD_X86_INSTRUCTION_SETA,
	NMD_X86_INSTRUCTION_SETS,
	NMD_X86_INSTRUCTION_SETNS,
	NMD_X86_INSTRUCTION_SETP,
	NMD_X86_INSTRUCTION_SETNP,
	NMD_X86_INSTRUCTION_SETL,
	NMD_X86_INSTRUCTION_SETGE,
	NMD_X86_INSTRUCTION_SETLE,
	NMD_X86_INSTRUCTION_SETG,

	/* Optimized for the row 0xb of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_LSS,
	NMD_X86_INSTRUCTION_BTR,
	NMD_X86_INSTRUCTION_LFS,
	NMD_X86_INSTRUCTION_LGS,

	NMD_X86_INSTRUCTION_BT,
	NMD_X86_INSTRUCTION_BTC,
	NMD_X86_INSTRUCTION_BTS,

	/* Optimized for the row 0x0 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PSHUFB,
	NMD_X86_INSTRUCTION_PHADDW,
	NMD_X86_INSTRUCTION_PHADDD,
	NMD_X86_INSTRUCTION_PHADDSW,
	NMD_X86_INSTRUCTION_PMADDUBSW,
	NMD_X86_INSTRUCTION_PHSUBW,
	NMD_X86_INSTRUCTION_PHSUBD,
	NMD_X86_INSTRUCTION_PHSUBSW,
	NMD_X86_INSTRUCTION_PSIGNB,
	NMD_X86_INSTRUCTION_PSIGNW,
	NMD_X86_INSTRUCTION_PSIGND,
	NMD_X86_INSTRUCTION_PMULHRSW,

	/* Optimized for the row 0x1 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PABSB,
	NMD_X86_INSTRUCTION_PABSW,
	NMD_X86_INSTRUCTION_PABSD,

	/* Optimized for the row 0x2 of the 3 byte opcode map(38). */
	NMD_X86_INSTRUCTION_PMOVSXBW,
	NMD_X86_INSTRUCTION_PMOVSXBD,
	NMD_X86_INSTRUCTION_PMOVSXBQ,
	NMD_X86_INSTRUCTION_PMOVSXWD,
	NMD_X86_INSTRUCTION_PMOVSXWQ,
	NMD_X86_INSTRUCTION_PMOVZXDQ,
	NMD_X86_INSTRUCTION_CPUID, /* padding */
	NMD_X86_INSTRUCTION_BLCMSK, /* padding */
	NMD_X86_INSTRUCTION_PMULDQ,
	NMD_X86_INSTRUCTION_PCMPEQQ,
	NMD_X86_INSTRUCTION_MOVNTDQA,
	NMD_X86_INSTRUCTION_PACKUSDW,

	/* Optimized for the row 0x3 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PMOVZXBW,
	NMD_X86_INSTRUCTION_PMOVZXBD,
	NMD_X86_INSTRUCTION_PMOVZXBQ,
	NMD_X86_INSTRUCTION_PMOVZXWD,
	NMD_X86_INSTRUCTION_PMOVZXWQ,
	NMD_X86_INSTRUCTION_PMOVSXDQ,
	NMD_X86_INSTRUCTION_BLCS, /* padding */
	NMD_X86_INSTRUCTION_PCMPGTQ,
	NMD_X86_INSTRUCTION_PMINSB,
	NMD_X86_INSTRUCTION_PMINSD,
	NMD_X86_INSTRUCTION_PMINUW,
	NMD_X86_INSTRUCTION_PMINUD,
	NMD_X86_INSTRUCTION_PMAXSB,
	NMD_X86_INSTRUCTION_PMAXSD,
	NMD_X86_INSTRUCTION_PMAXUW,
	NMD_X86_INSTRUCTION_PMAXUD,

	/* Optimized for the row 0x8 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_INVEPT,
	NMD_X86_INSTRUCTION_INVVPID,
	NMD_X86_INSTRUCTION_INVPCID,

	/* Optimized for the row 0xc of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_SHA1NEXTE,
	NMD_X86_INSTRUCTION_SHA1MSG1,
	NMD_X86_INSTRUCTION_SHA1MSG2,
	NMD_X86_INSTRUCTION_SHA256RNDS2,
	NMD_X86_INSTRUCTION_SHA256MSG1,
	NMD_X86_INSTRUCTION_SHA256MSG2,

	/* Optimized for the row 0xd of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_AESIMC,
	NMD_X86_INSTRUCTION_AESENC,
	NMD_X86_INSTRUCTION_AESENCLAST,
	NMD_X86_INSTRUCTION_AESDEC,
	NMD_X86_INSTRUCTION_AESDECLAST,

	/* Optimized for the row 0x0 of the 3 byte opcode map(3Ah). */
	NMD_X86_INSTRUCTION_ROUNDPS,
	NMD_X86_INSTRUCTION_ROUNDPD,
	NMD_X86_INSTRUCTION_ROUNDSS,
	NMD_X86_INSTRUCTION_ROUNDSD,
	NMD_X86_INSTRUCTION_BLENDPS,
	NMD_X86_INSTRUCTION_BLENDPD,
	NMD_X86_INSTRUCTION_PBLENDW,
	NMD_X86_INSTRUCTION_PALIGNR,

	/* Optimized for the row 0x4 of the 3 byte opcode map(3A). */
	NMD_X86_INSTRUCTION_DPPS,
	NMD_X86_INSTRUCTION_DPPD,
	NMD_X86_INSTRUCTION_MPSADBW,
	NMD_X86_INSTRUCTION_VPCMPGTQ, /* padding */
	NMD_X86_INSTRUCTION_PCLMULQDQ,

	/* Optimized for the row 0x6 of the 3 byte opcode map(3A). */
	NMD_X86_INSTRUCTION_PCMPESTRM,
	NMD_X86_INSTRUCTION_PCMPESTRI,
	NMD_X86_INSTRUCTION_PCMPISTRM,
	NMD_X86_INSTRUCTION_PCMPISTRI,

	/* Optimized for the rows 0xd, 0xe and 0xf of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_PSRLW,
	NMD_X86_INSTRUCTION_PSRLD,
	NMD_X86_INSTRUCTION_PSRLQ,
	NMD_X86_INSTRUCTION_PADDQ,
	NMD_X86_INSTRUCTION_PMULLW,
	NMD_X86_INSTRUCTION_BOUND, /* padding */
	NMD_X86_INSTRUCTION_PMOVMSKB,
	NMD_X86_INSTRUCTION_PSUBUSB,
	NMD_X86_INSTRUCTION_PSUBUSW,
	NMD_X86_INSTRUCTION_PMINUB,
	NMD_X86_INSTRUCTION_PAND,
	NMD_X86_INSTRUCTION_PADDUSB,
	NMD_X86_INSTRUCTION_PADDUSW,
	NMD_X86_INSTRUCTION_PMAXUB,
	NMD_X86_INSTRUCTION_PANDN,
	NMD_X86_INSTRUCTION_PAVGB,
	NMD_X86_INSTRUCTION_PSRAW,
	NMD_X86_INSTRUCTION_PSRAD,
	NMD_X86_INSTRUCTION_PAVGW,
	NMD_X86_INSTRUCTION_PMULHUW,
	NMD_X86_INSTRUCTION_PMULHW,
	NMD_X86_INSTRUCTION_CQO, /* padding */
	NMD_X86_INSTRUCTION_CRC32, /* padding */
	NMD_X86_INSTRUCTION_PSUBSB,
	NMD_X86_INSTRUCTION_PSUBSW,
	NMD_X86_INSTRUCTION_PMINSW,
	NMD_X86_INSTRUCTION_POR,
	NMD_X86_INSTRUCTION_PADDSB,
	NMD_X86_INSTRUCTION_PADDSW,
	NMD_X86_INSTRUCTION_PMAXSW,
	NMD_X86_INSTRUCTION_PXOR,
	NMD_X86_INSTRUCTION_LDDQU,
	NMD_X86_INSTRUCTION_PSLLW,
	NMD_X86_INSTRUCTION_PSLLD,
	NMD_X86_INSTRUCTION_PSLLQ,
	NMD_X86_INSTRUCTION_PMULUDQ,
	NMD_X86_INSTRUCTION_PMADDWD,
	NMD_X86_INSTRUCTION_PSADBW,
	NMD_X86_INSTRUCTION_BSWAP, /* padding */
	NMD_X86_INSTRUCTION_PSUBB,
	NMD_X86_INSTRUCTION_PSUBW,
	NMD_X86_INSTRUCTION_PSUBD,
	NMD_X86_INSTRUCTION_PSUBQ,
	NMD_X86_INSTRUCTION_PADDB,
	NMD_X86_INSTRUCTION_PADDW,
	NMD_X86_INSTRUCTION_PADDD,

	/* Optimized for the row 0xc of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_MOVNTI,
	NMD_X86_INSTRUCTION_PINSRW,
	NMD_X86_INSTRUCTION_PEXTRW,

	/* Optimized for opcode extension group 15. */
	NMD_X86_INSTRUCTION_FXSAVE,
	NMD_X86_INSTRUCTION_FXRSTOR,
	NMD_X86_INSTRUCTION_LDMXCSR,
	NMD_X86_INSTRUCTION_STMXCSR,
	NMD_X86_INSTRUCTION_XSAVE,
	NMD_X86_INSTRUCTION_XRSTOR,
	NMD_X86_INSTRUCTION_XSAVEOPT,
	NMD_X86_INSTRUCTION_CLFLUSH,

	NMD_X86_INSTRUCTION_RDFSBASE,
	NMD_X86_INSTRUCTION_RDGSBASE,
	NMD_X86_INSTRUCTION_WRFSBASE,
	NMD_X86_INSTRUCTION_WRGSBASE,
	NMD_X86_INSTRUCTION_CMPXCHG, /* padding */
	NMD_X86_INSTRUCTION_LFENCE,
	NMD_X86_INSTRUCTION_MFENCE,
	NMD_X86_INSTRUCTION_SFENCE,

	NMD_X86_INSTRUCTION_PCMPEQB,
	NMD_X86_INSTRUCTION_PCMPEQW,
	NMD_X86_INSTRUCTION_PCMPEQD,

	/* Optimized for the row 0x5 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_MOVMSKPS,
	NMD_X86_INSTRUCTION_SQRTPS,
	NMD_X86_INSTRUCTION_RSQRTPS,
	NMD_X86_INSTRUCTION_RCPPS,
	NMD_X86_INSTRUCTION_ANDPS,
	NMD_X86_INSTRUCTION_ANDNPS,
	NMD_X86_INSTRUCTION_ORPS,
	NMD_X86_INSTRUCTION_XORPS,
	NMD_X86_INSTRUCTION_ADDPS,
	NMD_X86_INSTRUCTION_MULPS,
	NMD_X86_INSTRUCTION_CVTPS2PD,
	NMD_X86_INSTRUCTION_CVTDQ2PS,
	NMD_X86_INSTRUCTION_SUBPS,
	NMD_X86_INSTRUCTION_MINPS,
	NMD_X86_INSTRUCTION_DIVPS,
	NMD_X86_INSTRUCTION_MAXPS,

	NMD_X86_INSTRUCTION_MOVMSKPD,
	NMD_X86_INSTRUCTION_SQRTPD,
	NMD_X86_INSTRUCTION_BNDLDX, /* padding */
	NMD_X86_INSTRUCTION_BNDSTX, /* padding */
	NMD_X86_INSTRUCTION_ANDPD,
	NMD_X86_INSTRUCTION_ANDNPD,
	NMD_X86_INSTRUCTION_ORPD,
	NMD_X86_INSTRUCTION_XORPD,
	NMD_X86_INSTRUCTION_ADDPD,
	NMD_X86_INSTRUCTION_MULPD,
	NMD_X86_INSTRUCTION_CVTPD2PS,
	NMD_X86_INSTRUCTION_CVTPS2DQ,
	NMD_X86_INSTRUCTION_SUBPD,
	NMD_X86_INSTRUCTION_MINPD,
	NMD_X86_INSTRUCTION_DIVPD,
	NMD_X86_INSTRUCTION_MAXPD,

	NMD_X86_INSTRUCTION_BNDMOV,  /* padding */
	NMD_X86_INSTRUCTION_SQRTSS,
	NMD_X86_INSTRUCTION_RSQRTSS,
	NMD_X86_INSTRUCTION_RCPSS,
	NMD_X86_INSTRUCTION_CMPXCHG16B, /* padding */
	NMD_X86_INSTRUCTION_DAA, /* padding */
	NMD_X86_INSTRUCTION_CWD, /* padding */
	NMD_X86_INSTRUCTION_INSD, /* padding */
	NMD_X86_INSTRUCTION_ADDSS,
	NMD_X86_INSTRUCTION_MULSS,
	NMD_X86_INSTRUCTION_CVTSS2SD,
	NMD_X86_INSTRUCTION_CVTTPS2DQ,
	NMD_X86_INSTRUCTION_SUBSS,
	NMD_X86_INSTRUCTION_MINSS,
	NMD_X86_INSTRUCTION_DIVSS,
	NMD_X86_INSTRUCTION_MAXSS,

	NMD_X86_INSTRUCTION_BNDCL, /* padding */
	NMD_X86_INSTRUCTION_SQRTSD,
	NMD_X86_INSTRUCTION_BNDCU, /* padding */
	NMD_X86_INSTRUCTION_BNDMK, /* padding */
	NMD_X86_INSTRUCTION_CMPXCHG8B, /* padding */
	NMD_X86_INSTRUCTION_DAS, /* padding */
	NMD_X86_INSTRUCTION_CWDE, /* padding */
	NMD_X86_INSTRUCTION_INSW, /* padding */
	NMD_X86_INSTRUCTION_ADDSD,
	NMD_X86_INSTRUCTION_MULSD,
	NMD_X86_INSTRUCTION_CVTSD2SS,
	NMD_X86_INSTRUCTION_FCOMIP, /* padding */
	NMD_X86_INSTRUCTION_SUBSD,
	NMD_X86_INSTRUCTION_MINSD,
	NMD_X86_INSTRUCTION_DIVSD,
	NMD_X86_INSTRUCTION_MAXSD,

	/* Optimized for the row 0x6 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_PUNPCKLBW,
	NMD_X86_INSTRUCTION_PUNPCKLWD,
	NMD_X86_INSTRUCTION_PUNPCKLDQ,
	NMD_X86_INSTRUCTION_PACKSSWB,
	NMD_X86_INSTRUCTION_PCMPGTB,
	NMD_X86_INSTRUCTION_PCMPGTW,
	NMD_X86_INSTRUCTION_PCMPGTD,
	NMD_X86_INSTRUCTION_PACKUSWB,
	NMD_X86_INSTRUCTION_PUNPCKHBW,
	NMD_X86_INSTRUCTION_PUNPCKHWD,
	NMD_X86_INSTRUCTION_PUNPCKHDQ,
	NMD_X86_INSTRUCTION_PACKSSDW,
	NMD_X86_INSTRUCTION_PUNPCKLQDQ,
	NMD_X86_INSTRUCTION_PUNPCKHQDQ,

	/* Optimized for AVX instructions. */
	NMD_X86_INSTRUCTION_VPSHUFB,    /* 00 */
	NMD_X86_INSTRUCTION_VPHADDW,    /* 01 */
	NMD_X86_INSTRUCTION_VPHADDD,    /* 02 */
	NMD_X86_INSTRUCTION_VPHADDSW,   /* 03 */
	NMD_X86_INSTRUCTION_VPMADDUBSW, /* 04 */
	NMD_X86_INSTRUCTION_VPHSUBW,    /* 05 */
	NMD_X86_INSTRUCTION_VPHSUBD,    /* 06 */
	NMD_X86_INSTRUCTION_VPHSUBSW,   /* 07 */
	NMD_X86_INSTRUCTION_VPSIGNB,    /* 08 */
	NMD_X86_INSTRUCTION_VPSIGNW,    /* 09 */
	NMD_X86_INSTRUCTION_VPSIGND,    /* 0A dup */
	NMD_X86_INSTRUCTION_VPMULHRSW,  /* 0B dup */

	NMD_X86_INSTRUCTION_VPHADDWQ,
	NMD_X86_INSTRUCTION_VPHADDDQ,
	NMD_X86_INSTRUCTION_BLSI,
	NMD_X86_INSTRUCTION_BLSIC,
	NMD_X86_INSTRUCTION_BLSMSK,
	NMD_X86_INSTRUCTION_BLSR,
	NMD_X86_INSTRUCTION_BSF,
	NMD_X86_INSTRUCTION_BZHI,
	NMD_X86_INSTRUCTION_CDQ,
	NMD_X86_INSTRUCTION_CDQE,
	NMD_X86_INSTRUCTION_CLFLUSHOPT,
	NMD_X86_INSTRUCTION_CMPSW,
	NMD_X86_INSTRUCTION_COMISD,
	NMD_X86_INSTRUCTION_COMISS,
	NMD_X86_INSTRUCTION_CVTDQ2PD,
	NMD_X86_INSTRUCTION_CVTPD2DQ,
	NMD_X86_INSTRUCTION_CVTSD2SI,
	NMD_X86_INSTRUCTION_CVTSI2SD,
	NMD_X86_INSTRUCTION_CVTSI2SS,
	NMD_X86_INSTRUCTION_CVTSS2SI,
	NMD_X86_INSTRUCTION_CVTTPD2DQ,
	NMD_X86_INSTRUCTION_CVTTSD2SI,
	NMD_X86_INSTRUCTION_CVTTSS2SI,
	NMD_X86_INSTRUCTION_DATA16,
	NMD_X86_INSTRUCTION_EXTRACTPS,
	NMD_X86_INSTRUCTION_EXTRQ,
	NMD_X86_INSTRUCTION_FCOMPP,
	NMD_X86_INSTRUCTION_FFREE,
	NMD_X86_INSTRUCTION_FNINIT,
	NMD_X86_INSTRUCTION_FNSTSW,
	NMD_X86_INSTRUCTION_FFREEP,
	NMD_X86_INSTRUCTION_FRSTOR,
	NMD_X86_INSTRUCTION_FNSAVE,
	NMD_X86_INSTRUCTION_FSETPM,
	NMD_X86_INSTRUCTION_FXRSTOR64,
	NMD_X86_INSTRUCTION_FXSAVE64,
	NMD_X86_INSTRUCTION_MOVAPS,
	NMD_X86_INSTRUCTION_VMOVAPD,
	NMD_X86_INSTRUCTION_VMOVAPS,
	NMD_X86_INSTRUCTION_HADDPD,
	NMD_X86_INSTRUCTION_HADDPS,
	NMD_X86_INSTRUCTION_HSUBPD,
	NMD_X86_INSTRUCTION_HSUBPS,
	NMD_X86_INSTRUCTION_IN,
	NMD_X86_INSTRUCTION_INSB,
	NMD_X86_INSTRUCTION_INSERTPS,
	NMD_X86_INSTRUCTION_INSERTQ,
	NMD_X86_INSTRUCTION_INT,
	NMD_X86_INSTRUCTION_INT3,
	NMD_X86_INSTRUCTION_INTO,
	NMD_X86_INSTRUCTION_IRET,
	NMD_X86_INSTRUCTION_IRETD,
	NMD_X86_INSTRUCTION_IRETQ,
	NMD_X86_INSTRUCTION_UCOMISD,
	NMD_X86_INSTRUCTION_UCOMISS,
	NMD_X86_INSTRUCTION_VCOMISD,
	NMD_X86_INSTRUCTION_VCOMISS,
	NMD_X86_INSTRUCTION_VCVTSD2SS,
	NMD_X86_INSTRUCTION_VCVTSI2SD,
	NMD_X86_INSTRUCTION_VCVTSI2SS,
	NMD_X86_INSTRUCTION_VCVTSS2SD,
	NMD_X86_INSTRUCTION_VCVTTSD2SI,
	NMD_X86_INSTRUCTION_VCVTTSD2USI,
	NMD_X86_INSTRUCTION_VCVTTSS2SI,
	NMD_X86_INSTRUCTION_VCVTTSS2USI,
	NMD_X86_INSTRUCTION_VCVTUSI2SD,
	NMD_X86_INSTRUCTION_VCVTUSI2SS,
	NMD_X86_INSTRUCTION_VUCOMISD,
	NMD_X86_INSTRUCTION_VUCOMISS,
	NMD_X86_INSTRUCTION_JCXZ,
	NMD_X86_INSTRUCTION_JECXZ,
	NMD_X86_INSTRUCTION_KANDB,
	NMD_X86_INSTRUCTION_KANDD,
	NMD_X86_INSTRUCTION_KANDNB,
	NMD_X86_INSTRUCTION_KANDND,
	NMD_X86_INSTRUCTION_KANDNQ,
	NMD_X86_INSTRUCTION_KANDNW,
	NMD_X86_INSTRUCTION_KANDQ,
	NMD_X86_INSTRUCTION_KANDW,
	NMD_X86_INSTRUCTION_KMOVB,
	NMD_X86_INSTRUCTION_KMOVD,
	NMD_X86_INSTRUCTION_KMOVQ,
	NMD_X86_INSTRUCTION_KMOVW,
	NMD_X86_INSTRUCTION_KNOTB,
	NMD_X86_INSTRUCTION_KNOTD,
	NMD_X86_INSTRUCTION_KNOTQ,
	NMD_X86_INSTRUCTION_KNOTW,
	NMD_X86_INSTRUCTION_KORB,
	NMD_X86_INSTRUCTION_KORD,
	NMD_X86_INSTRUCTION_KORQ,
	NMD_X86_INSTRUCTION_KORTESTB,
	NMD_X86_INSTRUCTION_KORTESTD,
	NMD_X86_INSTRUCTION_KORTESTQ,
	NMD_X86_INSTRUCTION_KORTESTW,
	NMD_X86_INSTRUCTION_KORW,
	NMD_X86_INSTRUCTION_KSHIFTLB,
	NMD_X86_INSTRUCTION_KSHIFTLD,
	NMD_X86_INSTRUCTION_KSHIFTLQ,
	NMD_X86_INSTRUCTION_KSHIFTLW,
	NMD_X86_INSTRUCTION_KSHIFTRB,
	NMD_X86_INSTRUCTION_KSHIFTRD,
	NMD_X86_INSTRUCTION_KSHIFTRQ,
	NMD_X86_INSTRUCTION_KSHIFTRW,
	NMD_X86_INSTRUCTION_KUNPCKBW,
	NMD_X86_INSTRUCTION_KXNORB,
	NMD_X86_INSTRUCTION_KXNORD,
	NMD_X86_INSTRUCTION_KXNORQ,
	NMD_X86_INSTRUCTION_KXNORW,
	NMD_X86_INSTRUCTION_KXORB,
	NMD_X86_INSTRUCTION_KXORD,
	NMD_X86_INSTRUCTION_KXORQ,
	NMD_X86_INSTRUCTION_KXORW,
	NMD_X86_INSTRUCTION_LAHF,
	NMD_X86_INSTRUCTION_LDS,
	NMD_X86_INSTRUCTION_LEA,
	NMD_X86_INSTRUCTION_LEAVE,
	NMD_X86_INSTRUCTION_LES,
	NMD_X86_INSTRUCTION_LODSB,
	NMD_X86_INSTRUCTION_LODSD,
	NMD_X86_INSTRUCTION_LODSQ,
	NMD_X86_INSTRUCTION_LODSW,
	NMD_X86_INSTRUCTION_RETF,
	NMD_X86_INSTRUCTION_XADD,
	NMD_X86_INSTRUCTION_LZCNT,
	NMD_X86_INSTRUCTION_MASKMOVDQU,
	NMD_X86_INSTRUCTION_CVTPD2PI,
	NMD_X86_INSTRUCTION_CVTPI2PD,
	NMD_X86_INSTRUCTION_CVTPI2PS,
	NMD_X86_INSTRUCTION_CVTPS2PI,
	NMD_X86_INSTRUCTION_CVTTPD2PI,
	NMD_X86_INSTRUCTION_CVTTPS2PI,
	NMD_X86_INSTRUCTION_EMMS,
	NMD_X86_INSTRUCTION_MASKMOVQ,
	NMD_X86_INSTRUCTION_MOVD,
	NMD_X86_INSTRUCTION_MOVDQ2Q,
	NMD_X86_INSTRUCTION_MOVNTQ,
	NMD_X86_INSTRUCTION_MOVQ2DQ,
	NMD_X86_INSTRUCTION_MOVQ,
	NMD_X86_INSTRUCTION_PSHUFW,
	NMD_X86_INSTRUCTION_MONTMUL,
	NMD_X86_INSTRUCTION_MOV,
	NMD_X86_INSTRUCTION_MOVABS,
	NMD_X86_INSTRUCTION_MOVBE,
	NMD_X86_INSTRUCTION_MOVDDUP,
	NMD_X86_INSTRUCTION_MOVDQA,
	NMD_X86_INSTRUCTION_MOVDQU,
	NMD_X86_INSTRUCTION_MOVHLPS,
	NMD_X86_INSTRUCTION_MOVHPD,
	NMD_X86_INSTRUCTION_MOVHPS,
	NMD_X86_INSTRUCTION_MOVLHPS,
	NMD_X86_INSTRUCTION_MOVLPD,
	NMD_X86_INSTRUCTION_MOVLPS,
	NMD_X86_INSTRUCTION_MOVNTDQ,
	NMD_X86_INSTRUCTION_MOVNTPD,
	NMD_X86_INSTRUCTION_MOVNTPS,
	NMD_X86_INSTRUCTION_MOVNTSD,
	NMD_X86_INSTRUCTION_MOVNTSS,
	NMD_X86_INSTRUCTION_MOVSB,
	NMD_X86_INSTRUCTION_MOVSD,
	NMD_X86_INSTRUCTION_MOVSHDUP,
	NMD_X86_INSTRUCTION_MOVSLDUP,
	NMD_X86_INSTRUCTION_MOVSQ,
	NMD_X86_INSTRUCTION_MOVSS,
	NMD_X86_INSTRUCTION_MOVSW,
	NMD_X86_INSTRUCTION_MOVSX,
	NMD_X86_INSTRUCTION_MOVSXD,
	NMD_X86_INSTRUCTION_MOVUPD,
	NMD_X86_INSTRUCTION_MOVUPS,
	NMD_X86_INSTRUCTION_MOVZX,
	NMD_X86_INSTRUCTION_MULX,
	NMD_X86_INSTRUCTION_NOP,
	NMD_X86_INSTRUCTION_OUT,
	NMD_X86_INSTRUCTION_OUTSB,
	NMD_X86_INSTRUCTION_OUTSD,
	NMD_X86_INSTRUCTION_OUTSW,
	NMD_X86_INSTRUCTION_PAUSE,
	NMD_X86_INSTRUCTION_PAVGUSB,
	NMD_X86_INSTRUCTION_PBLENDVB,
	NMD_X86_INSTRUCTION_PCOMMIT,
	NMD_X86_INSTRUCTION_PDEP,
	NMD_X86_INSTRUCTION_PEXT,
	NMD_X86_INSTRUCTION_PEXTRB,
	NMD_X86_INSTRUCTION_PEXTRD,
	NMD_X86_INSTRUCTION_PEXTRQ,
	NMD_X86_INSTRUCTION_PF2ID,
	NMD_X86_INSTRUCTION_PF2IW,
	NMD_X86_INSTRUCTION_PFACC,
	NMD_X86_INSTRUCTION_PFADD,
	NMD_X86_INSTRUCTION_BLENDVPS, /* padding*/
	NMD_X86_INSTRUCTION_PFCMPEQ,
	NMD_X86_INSTRUCTION_PFCMPGE,
	NMD_X86_INSTRUCTION_PFCMPGT,
	NMD_X86_INSTRUCTION_PFMAX,
	NMD_X86_INSTRUCTION_PFMIN,
	NMD_X86_INSTRUCTION_PFMUL,
	NMD_X86_INSTRUCTION_PFNACC,
	NMD_X86_INSTRUCTION_PFPNACC,
	NMD_X86_INSTRUCTION_PFRCPIT1,
	NMD_X86_INSTRUCTION_PFRCPIT2,
	NMD_X86_INSTRUCTION_PFRCP,
	NMD_X86_INSTRUCTION_PFRSQIT1,
	NMD_X86_INSTRUCTION_PFRSQRT,
	NMD_X86_INSTRUCTION_PFSUBR,
	NMD_X86_INSTRUCTION_PFSUB,
	NMD_X86_INSTRUCTION_PHMINPOSUW,
	NMD_X86_INSTRUCTION_PI2FD,
	NMD_X86_INSTRUCTION_PI2FW,
	NMD_X86_INSTRUCTION_PINSRB,
	NMD_X86_INSTRUCTION_PINSRD,
	NMD_X86_INSTRUCTION_PINSRQ,
	NMD_X86_INSTRUCTION_PMULHRW,
	NMD_X86_INSTRUCTION_PMULLD,
	NMD_X86_INSTRUCTION_POP,
	NMD_X86_INSTRUCTION_POPA,
	NMD_X86_INSTRUCTION_POPAD,
	NMD_X86_INSTRUCTION_POPCNT,
	NMD_X86_INSTRUCTION_POPF,
	NMD_X86_INSTRUCTION_POPFD,
	NMD_X86_INSTRUCTION_POPFQ,
	NMD_X86_INSTRUCTION_PREFETCH,
	NMD_X86_INSTRUCTION_PREFETCHNTA,
	NMD_X86_INSTRUCTION_PREFETCHT0,
	NMD_X86_INSTRUCTION_PREFETCHT1,
	NMD_X86_INSTRUCTION_PREFETCHT2,
	NMD_X86_INSTRUCTION_PSHUFD,
	NMD_X86_INSTRUCTION_PSHUFHW,
	NMD_X86_INSTRUCTION_PSHUFLW,
	NMD_X86_INSTRUCTION_PSLLDQ,
	NMD_X86_INSTRUCTION_PSRLDQ,
	NMD_X86_INSTRUCTION_PSWAPD,
	NMD_X86_INSTRUCTION_PTEST,
	NMD_X86_INSTRUCTION_PUSHA,
	NMD_X86_INSTRUCTION_PUSHAD,
	NMD_X86_INSTRUCTION_PUSHF,
	NMD_X86_INSTRUCTION_PUSHFD,
	NMD_X86_INSTRUCTION_PUSHFQ,
	NMD_X86_INSTRUCTION_RDRAND,
	NMD_X86_INSTRUCTION_RDPID,
	NMD_X86_INSTRUCTION_RDSEED,
	NMD_X86_INSTRUCTION_RDTSCP,
	NMD_X86_INSTRUCTION_RORX,
	NMD_X86_INSTRUCTION_RSM,
	NMD_X86_INSTRUCTION_SAHF,
	NMD_X86_INSTRUCTION_SAL,
	NMD_X86_INSTRUCTION_SARX,
	NMD_X86_INSTRUCTION_SCASB,
	NMD_X86_INSTRUCTION_SCASD,
	NMD_X86_INSTRUCTION_SCASQ,
	NMD_X86_INSTRUCTION_SCASW,
	NMD_X86_INSTRUCTION_SHA1RNDS4,
	NMD_X86_INSTRUCTION_SHLD,
	NMD_X86_INSTRUCTION_SHLX,
	NMD_X86_INSTRUCTION_SHRD,
	NMD_X86_INSTRUCTION_SHRX,
	NMD_X86_INSTRUCTION_SHUFPD,
	NMD_X86_INSTRUCTION_SHUFPS,
	NMD_X86_INSTRUCTION_STOSB,
	NMD_X86_INSTRUCTION_STOSD,
	NMD_X86_INSTRUCTION_STOSQ,
	NMD_X86_INSTRUCTION_STOSW,
	NMD_X86_INSTRUCTION_FSTPNCE,
	NMD_X86_INSTRUCTION_FXCH,
	NMD_X86_INSTRUCTION_SWAPGS,
	NMD_X86_INSTRUCTION_T1MSKC,
	NMD_X86_INSTRUCTION_TZCNT,
	NMD_X86_INSTRUCTION_TZMSK,
	NMD_X86_INSTRUCTION_FUCOMIP,
	NMD_X86_INSTRUCTION_FUCOMPP,
	NMD_X86_INSTRUCTION_FUCOMP,
	NMD_X86_INSTRUCTION_FUCOM,
	NMD_X86_INSTRUCTION_UD1,
	NMD_X86_INSTRUCTION_UNPCKHPD,
	NMD_X86_INSTRUCTION_UNPCKHPS,
	NMD_X86_INSTRUCTION_UNPCKLPD,
	NMD_X86_INSTRUCTION_UNPCKLPS,
	NMD_X86_INSTRUCTION_VADDPD,
	NMD_X86_INSTRUCTION_VADDPS,
	NMD_X86_INSTRUCTION_VADDSD,
	NMD_X86_INSTRUCTION_VADDSS,
	NMD_X86_INSTRUCTION_VADDSUBPD,
	NMD_X86_INSTRUCTION_VADDSUBPS,
	NMD_X86_INSTRUCTION_VAESDECLAST,
	NMD_X86_INSTRUCTION_VAESDEC,
	NMD_X86_INSTRUCTION_VAESENCLAST,
	NMD_X86_INSTRUCTION_VAESENC,
	NMD_X86_INSTRUCTION_VAESIMC,
	NMD_X86_INSTRUCTION_VAESKEYGENASSIST,
	NMD_X86_INSTRUCTION_VALIGND,
	NMD_X86_INSTRUCTION_VALIGNQ,
	NMD_X86_INSTRUCTION_VANDNPD,
	NMD_X86_INSTRUCTION_VANDNPS,
	NMD_X86_INSTRUCTION_VANDPD,
	NMD_X86_INSTRUCTION_VANDPS,
	NMD_X86_INSTRUCTION_VBLENDMPD,
	NMD_X86_INSTRUCTION_VBLENDMPS,
	NMD_X86_INSTRUCTION_VBLENDPD,
	NMD_X86_INSTRUCTION_VBLENDPS,
	NMD_X86_INSTRUCTION_VBLENDVPD,
	NMD_X86_INSTRUCTION_VBLENDVPS,
	NMD_X86_INSTRUCTION_VBROADCASTF128,
	NMD_X86_INSTRUCTION_VBROADCASTI32X4,
	NMD_X86_INSTRUCTION_VBROADCASTI64X4,
	NMD_X86_INSTRUCTION_VBROADCASTSD,
	NMD_X86_INSTRUCTION_VBROADCASTSS,
	NMD_X86_INSTRUCTION_VCOMPRESSPD,
	NMD_X86_INSTRUCTION_VCOMPRESSPS,
	NMD_X86_INSTRUCTION_VCVTDQ2PD,
	NMD_X86_INSTRUCTION_VCVTDQ2PS,
	NMD_X86_INSTRUCTION_VCVTPD2DQX,
	NMD_X86_INSTRUCTION_VCVTPD2DQ,
	NMD_X86_INSTRUCTION_VCVTPD2PSX,
	NMD_X86_INSTRUCTION_VCVTPD2PS,
	NMD_X86_INSTRUCTION_VCVTPD2UDQ,
	NMD_X86_INSTRUCTION_VCVTPH2PS,
	NMD_X86_INSTRUCTION_VCVTPS2DQ,
	NMD_X86_INSTRUCTION_VCVTPS2PD,
	NMD_X86_INSTRUCTION_VCVTPS2PH,
	NMD_X86_INSTRUCTION_VCVTPS2UDQ,
	NMD_X86_INSTRUCTION_VCVTSD2SI,
	NMD_X86_INSTRUCTION_VCVTSD2USI,
	NMD_X86_INSTRUCTION_VCVTSS2SI,
	NMD_X86_INSTRUCTION_VCVTSS2USI,
	NMD_X86_INSTRUCTION_VCVTTPD2DQX,
	NMD_X86_INSTRUCTION_VCVTTPD2DQ,
	NMD_X86_INSTRUCTION_VCVTTPD2UDQ,
	NMD_X86_INSTRUCTION_VCVTTPS2DQ,
	NMD_X86_INSTRUCTION_VCVTTPS2UDQ,
	NMD_X86_INSTRUCTION_VCVTUDQ2PD,
	NMD_X86_INSTRUCTION_VCVTUDQ2PS,
	NMD_X86_INSTRUCTION_VDIVPD,
	NMD_X86_INSTRUCTION_VDIVPS,
	NMD_X86_INSTRUCTION_VDIVSD,
	NMD_X86_INSTRUCTION_VDIVSS,
	NMD_X86_INSTRUCTION_VDPPD,
	NMD_X86_INSTRUCTION_VDPPS,
	NMD_X86_INSTRUCTION_VEXP2PD,
	NMD_X86_INSTRUCTION_VEXP2PS,
	NMD_X86_INSTRUCTION_VEXPANDPD,
	NMD_X86_INSTRUCTION_VEXPANDPS,
	NMD_X86_INSTRUCTION_VEXTRACTF128,
	NMD_X86_INSTRUCTION_VEXTRACTF32X4,
	NMD_X86_INSTRUCTION_VEXTRACTF64X4,
	NMD_X86_INSTRUCTION_VEXTRACTI128,
	NMD_X86_INSTRUCTION_VEXTRACTI32X4,
	NMD_X86_INSTRUCTION_VEXTRACTI64X4,
	NMD_X86_INSTRUCTION_VEXTRACTPS,
	NMD_X86_INSTRUCTION_VFMADD132PD,
	NMD_X86_INSTRUCTION_VFMADD132PS,
	NMD_X86_INSTRUCTION_VFMADDPD,
	NMD_X86_INSTRUCTION_VFMADD213PD,
	NMD_X86_INSTRUCTION_VFMADD231PD,
	NMD_X86_INSTRUCTION_VFMADDPS,
	NMD_X86_INSTRUCTION_VFMADD213PS,
	NMD_X86_INSTRUCTION_VFMADD231PS,
	NMD_X86_INSTRUCTION_VFMADDSD,
	NMD_X86_INSTRUCTION_VFMADD213SD,
	NMD_X86_INSTRUCTION_VFMADD132SD,
	NMD_X86_INSTRUCTION_VFMADD231SD,
	NMD_X86_INSTRUCTION_VFMADDSS,
	NMD_X86_INSTRUCTION_VFMADD213SS,
	NMD_X86_INSTRUCTION_VFMADD132SS,
	NMD_X86_INSTRUCTION_VFMADD231SS,
	NMD_X86_INSTRUCTION_VFMADDSUB132PD,
	NMD_X86_INSTRUCTION_VFMADDSUB132PS,
	NMD_X86_INSTRUCTION_VFMADDSUBPD,
	NMD_X86_INSTRUCTION_VFMADDSUB213PD,
	NMD_X86_INSTRUCTION_VFMADDSUB231PD,
	NMD_X86_INSTRUCTION_VFMADDSUBPS,
	NMD_X86_INSTRUCTION_VFMADDSUB213PS,
	NMD_X86_INSTRUCTION_VFMADDSUB231PS,
	NMD_X86_INSTRUCTION_VFMSUB132PD,
	NMD_X86_INSTRUCTION_VFMSUB132PS,
	NMD_X86_INSTRUCTION_VFMSUBADD132PD,
	NMD_X86_INSTRUCTION_VFMSUBADD132PS,
	NMD_X86_INSTRUCTION_VFMSUBADDPD,
	NMD_X86_INSTRUCTION_VFMSUBADD213PD,
	NMD_X86_INSTRUCTION_VFMSUBADD231PD,
	NMD_X86_INSTRUCTION_VFMSUBADDPS,
	NMD_X86_INSTRUCTION_VFMSUBADD213PS,
	NMD_X86_INSTRUCTION_VFMSUBADD231PS,
	NMD_X86_INSTRUCTION_VFMSUBPD,
	NMD_X86_INSTRUCTION_VFMSUB213PD,
	NMD_X86_INSTRUCTION_VFMSUB231PD,
	NMD_X86_INSTRUCTION_VFMSUBPS,
	NMD_X86_INSTRUCTION_VFMSUB213PS,
	NMD_X86_INSTRUCTION_VFMSUB231PS,
	NMD_X86_INSTRUCTION_VFMSUBSD,
	NMD_X86_INSTRUCTION_VFMSUB213SD,
	NMD_X86_INSTRUCTION_VFMSUB132SD,
	NMD_X86_INSTRUCTION_VFMSUB231SD,
	NMD_X86_INSTRUCTION_VFMSUBSS,
	NMD_X86_INSTRUCTION_VFMSUB213SS,
	NMD_X86_INSTRUCTION_VFMSUB132SS,
	NMD_X86_INSTRUCTION_VFMSUB231SS,
	NMD_X86_INSTRUCTION_VFNMADD132PD,
	NMD_X86_INSTRUCTION_VFNMADD132PS,
	NMD_X86_INSTRUCTION_VFNMADDPD,
	NMD_X86_INSTRUCTION_VFNMADD213PD,
	NMD_X86_INSTRUCTION_VFNMADD231PD,
	NMD_X86_INSTRUCTION_VFNMADDPS,
	NMD_X86_INSTRUCTION_VFNMADD213PS,
	NMD_X86_INSTRUCTION_VFNMADD231PS,
	NMD_X86_INSTRUCTION_VFNMADDSD,
	NMD_X86_INSTRUCTION_VFNMADD213SD,
	NMD_X86_INSTRUCTION_VFNMADD132SD,
	NMD_X86_INSTRUCTION_VFNMADD231SD,
	NMD_X86_INSTRUCTION_VFNMADDSS,
	NMD_X86_INSTRUCTION_VFNMADD213SS,
	NMD_X86_INSTRUCTION_VFNMADD132SS,
	NMD_X86_INSTRUCTION_VFNMADD231SS,
	NMD_X86_INSTRUCTION_VFNMSUB132PD,
	NMD_X86_INSTRUCTION_VFNMSUB132PS,
	NMD_X86_INSTRUCTION_VFNMSUBPD,
	NMD_X86_INSTRUCTION_VFNMSUB213PD,
	NMD_X86_INSTRUCTION_VFNMSUB231PD,
	NMD_X86_INSTRUCTION_VFNMSUBPS,
	NMD_X86_INSTRUCTION_VFNMSUB213PS,
	NMD_X86_INSTRUCTION_VFNMSUB231PS,
	NMD_X86_INSTRUCTION_VFNMSUBSD,
	NMD_X86_INSTRUCTION_VFNMSUB213SD,
	NMD_X86_INSTRUCTION_VFNMSUB132SD,
	NMD_X86_INSTRUCTION_VFNMSUB231SD,
	NMD_X86_INSTRUCTION_VFNMSUBSS,
	NMD_X86_INSTRUCTION_VFNMSUB213SS,
	NMD_X86_INSTRUCTION_VFNMSUB132SS,
	NMD_X86_INSTRUCTION_VFNMSUB231SS,
	NMD_X86_INSTRUCTION_VFRCZPD,
	NMD_X86_INSTRUCTION_VFRCZPS,
	NMD_X86_INSTRUCTION_VFRCZSD,
	NMD_X86_INSTRUCTION_VFRCZSS,
	NMD_X86_INSTRUCTION_VORPD,
	NMD_X86_INSTRUCTION_VORPS,
	NMD_X86_INSTRUCTION_VXORPD,
	NMD_X86_INSTRUCTION_VXORPS,
	NMD_X86_INSTRUCTION_VGATHERDPD,
	NMD_X86_INSTRUCTION_VGATHERDPS,
	NMD_X86_INSTRUCTION_VGATHERPF0DPD,
	NMD_X86_INSTRUCTION_VGATHERPF0DPS,
	NMD_X86_INSTRUCTION_VGATHERPF0QPD,
	NMD_X86_INSTRUCTION_VGATHERPF0QPS,
	NMD_X86_INSTRUCTION_VGATHERPF1DPD,
	NMD_X86_INSTRUCTION_VGATHERPF1DPS,
	NMD_X86_INSTRUCTION_VGATHERPF1QPD,
	NMD_X86_INSTRUCTION_VGATHERPF1QPS,
	NMD_X86_INSTRUCTION_VGATHERQPD,
	NMD_X86_INSTRUCTION_VGATHERQPS,
	NMD_X86_INSTRUCTION_VHADDPD,
	NMD_X86_INSTRUCTION_VHADDPS,
	NMD_X86_INSTRUCTION_VHSUBPD,
	NMD_X86_INSTRUCTION_VHSUBPS,
	NMD_X86_INSTRUCTION_VINSERTF128,
	NMD_X86_INSTRUCTION_VINSERTF32X4,
	NMD_X86_INSTRUCTION_VINSERTF32X8,
	NMD_X86_INSTRUCTION_VINSERTF64X2,
	NMD_X86_INSTRUCTION_VINSERTF64X4,
	NMD_X86_INSTRUCTION_VINSERTI128,
	NMD_X86_INSTRUCTION_VINSERTI32X4,
	NMD_X86_INSTRUCTION_VINSERTI32X8,
	NMD_X86_INSTRUCTION_VINSERTI64X2,
	NMD_X86_INSTRUCTION_VINSERTI64X4,
	NMD_X86_INSTRUCTION_VINSERTPS,
	NMD_X86_INSTRUCTION_VLDDQU,
	NMD_X86_INSTRUCTION_VLDMXCSR,
	NMD_X86_INSTRUCTION_VMASKMOVDQU,
	NMD_X86_INSTRUCTION_VMASKMOVPD,
	NMD_X86_INSTRUCTION_VMASKMOVPS,
	NMD_X86_INSTRUCTION_VMAXPD,
	NMD_X86_INSTRUCTION_VMAXPS,
	NMD_X86_INSTRUCTION_VMAXSD,
	NMD_X86_INSTRUCTION_VMAXSS,
	NMD_X86_INSTRUCTION_VMCLEAR,
	NMD_X86_INSTRUCTION_VMINPD,
	NMD_X86_INSTRUCTION_VMINPS,
	NMD_X86_INSTRUCTION_VMINSD,
	NMD_X86_INSTRUCTION_VMINSS,
	NMD_X86_INSTRUCTION_VMOVQ,
	NMD_X86_INSTRUCTION_VMOVDDUP,
	NMD_X86_INSTRUCTION_VMOVD,
	NMD_X86_INSTRUCTION_VMOVDQA32,
	NMD_X86_INSTRUCTION_VMOVDQA64,
	NMD_X86_INSTRUCTION_VMOVDQA,
	NMD_X86_INSTRUCTION_VMOVDQU16,
	NMD_X86_INSTRUCTION_VMOVDQU32,
	NMD_X86_INSTRUCTION_VMOVDQU64,
	NMD_X86_INSTRUCTION_VMOVDQU8,
	NMD_X86_INSTRUCTION_VMOVDQU,
	NMD_X86_INSTRUCTION_VMOVHLPS,
	NMD_X86_INSTRUCTION_VMOVHPD,
	NMD_X86_INSTRUCTION_VMOVHPS,
	NMD_X86_INSTRUCTION_VMOVLHPS,
	NMD_X86_INSTRUCTION_VMOVLPD,
	NMD_X86_INSTRUCTION_VMOVLPS,
	NMD_X86_INSTRUCTION_VMOVMSKPD,
	NMD_X86_INSTRUCTION_VMOVMSKPS,
	NMD_X86_INSTRUCTION_VMOVNTDQA,
	NMD_X86_INSTRUCTION_VMOVNTDQ,
	NMD_X86_INSTRUCTION_VMOVNTPD,
	NMD_X86_INSTRUCTION_VMOVNTPS,
	NMD_X86_INSTRUCTION_VMOVSD,
	NMD_X86_INSTRUCTION_VMOVSHDUP,
	NMD_X86_INSTRUCTION_VMOVSLDUP,
	NMD_X86_INSTRUCTION_VMOVSS,
	NMD_X86_INSTRUCTION_VMOVUPD,
	NMD_X86_INSTRUCTION_VMOVUPS,
	NMD_X86_INSTRUCTION_VMPSADBW,
	NMD_X86_INSTRUCTION_VMPTRLD,
	NMD_X86_INSTRUCTION_VMPTRST,
	NMD_X86_INSTRUCTION_VMREAD,
	NMD_X86_INSTRUCTION_VMULPD,
	NMD_X86_INSTRUCTION_VMULPS,
	NMD_X86_INSTRUCTION_VMULSD,
	NMD_X86_INSTRUCTION_VMULSS,
	NMD_X86_INSTRUCTION_VMWRITE,
	NMD_X86_INSTRUCTION_VMXON,
	NMD_X86_INSTRUCTION_VPABSB,
	NMD_X86_INSTRUCTION_VPABSD,
	NMD_X86_INSTRUCTION_VPABSQ,
	NMD_X86_INSTRUCTION_VPABSW,
	NMD_X86_INSTRUCTION_VPACKSSDW,
	NMD_X86_INSTRUCTION_VPACKSSWB,
	NMD_X86_INSTRUCTION_VPACKUSDW,
	NMD_X86_INSTRUCTION_VPACKUSWB,
	NMD_X86_INSTRUCTION_VPADDB,
	NMD_X86_INSTRUCTION_VPADDD,
	NMD_X86_INSTRUCTION_VPADDQ,
	NMD_X86_INSTRUCTION_VPADDSB,
	NMD_X86_INSTRUCTION_VPADDSW,
	NMD_X86_INSTRUCTION_VPADDUSB,
	NMD_X86_INSTRUCTION_VPADDUSW,
	NMD_X86_INSTRUCTION_VPADDW,
	NMD_X86_INSTRUCTION_VPALIGNR,
	NMD_X86_INSTRUCTION_VPANDD,
	NMD_X86_INSTRUCTION_VPANDND,
	NMD_X86_INSTRUCTION_VPANDNQ,
	NMD_X86_INSTRUCTION_VPANDN,
	NMD_X86_INSTRUCTION_VPANDQ,
	NMD_X86_INSTRUCTION_VPAND,
	NMD_X86_INSTRUCTION_VPAVGB,
	NMD_X86_INSTRUCTION_VPAVGW,
	NMD_X86_INSTRUCTION_VPBLENDD,
	NMD_X86_INSTRUCTION_VPBLENDMB,
	NMD_X86_INSTRUCTION_VPBLENDMD,
	NMD_X86_INSTRUCTION_VPBLENDMQ,
	NMD_X86_INSTRUCTION_VPBLENDMW,
	NMD_X86_INSTRUCTION_VPBLENDVB,
	NMD_X86_INSTRUCTION_VPBLENDW,
	NMD_X86_INSTRUCTION_VPBROADCASTB,
	NMD_X86_INSTRUCTION_VPBROADCASTD,
	NMD_X86_INSTRUCTION_VPBROADCASTMB2Q,
	NMD_X86_INSTRUCTION_VPBROADCASTMW2D,
	NMD_X86_INSTRUCTION_VPBROADCASTQ,
	NMD_X86_INSTRUCTION_VPBROADCASTW,
	NMD_X86_INSTRUCTION_VPCLMULQDQ,
	NMD_X86_INSTRUCTION_VPCMOV,
	NMD_X86_INSTRUCTION_VPCMPB,
	NMD_X86_INSTRUCTION_VPCMPD,
	NMD_X86_INSTRUCTION_VPCMPEQB,
	NMD_X86_INSTRUCTION_VPCMPEQD,
	NMD_X86_INSTRUCTION_VPCMPEQQ,
	NMD_X86_INSTRUCTION_VPCMPEQW,
	NMD_X86_INSTRUCTION_VPCMPESTRI,
	NMD_X86_INSTRUCTION_VPCMPESTRM,
	NMD_X86_INSTRUCTION_VPCMPGTB,
	NMD_X86_INSTRUCTION_VPCMPGTD,
	NMD_X86_INSTRUCTION_VPCMPGTW,
	NMD_X86_INSTRUCTION_VPCMPISTRI,
	NMD_X86_INSTRUCTION_VPCMPISTRM,
	NMD_X86_INSTRUCTION_VPCMPQ,
	NMD_X86_INSTRUCTION_VPCMPUB,
	NMD_X86_INSTRUCTION_VPCMPUD,
	NMD_X86_INSTRUCTION_VPCMPUQ,
	NMD_X86_INSTRUCTION_VPCMPUW,
	NMD_X86_INSTRUCTION_VPCMPW,
	NMD_X86_INSTRUCTION_VPCOMB,
	NMD_X86_INSTRUCTION_VPCOMD,
	NMD_X86_INSTRUCTION_VPCOMPRESSD,
	NMD_X86_INSTRUCTION_VPCOMPRESSQ,
	NMD_X86_INSTRUCTION_VPCOMQ,
	NMD_X86_INSTRUCTION_VPCOMUB,
	NMD_X86_INSTRUCTION_VPCOMUD,
	NMD_X86_INSTRUCTION_VPCOMUQ,
	NMD_X86_INSTRUCTION_VPCOMUW,
	NMD_X86_INSTRUCTION_VPCOMW,
	NMD_X86_INSTRUCTION_VPCONFLICTD,
	NMD_X86_INSTRUCTION_VPCONFLICTQ,
	NMD_X86_INSTRUCTION_VPERM2F128,
	NMD_X86_INSTRUCTION_VPERM2I128,
	NMD_X86_INSTRUCTION_VPERMD,
	NMD_X86_INSTRUCTION_VPERMI2D,
	NMD_X86_INSTRUCTION_VPERMI2PD,
	NMD_X86_INSTRUCTION_VPERMI2PS,
	NMD_X86_INSTRUCTION_VPERMI2Q,
	NMD_X86_INSTRUCTION_VPERMIL2PD,
	NMD_X86_INSTRUCTION_VPERMIL2PS,
	NMD_X86_INSTRUCTION_VPERMILPD,
	NMD_X86_INSTRUCTION_VPERMILPS,
	NMD_X86_INSTRUCTION_VPERMPD,
	NMD_X86_INSTRUCTION_VPERMPS,
	NMD_X86_INSTRUCTION_VPERMQ,
	NMD_X86_INSTRUCTION_VPERMT2D,
	NMD_X86_INSTRUCTION_VPERMT2PD,
	NMD_X86_INSTRUCTION_VPERMT2PS,
	NMD_X86_INSTRUCTION_VPERMT2Q,
	NMD_X86_INSTRUCTION_VPEXPANDD,
	NMD_X86_INSTRUCTION_VPEXPANDQ,
	NMD_X86_INSTRUCTION_VPEXTRB,
	NMD_X86_INSTRUCTION_VPEXTRD,
	NMD_X86_INSTRUCTION_VPEXTRQ,
	NMD_X86_INSTRUCTION_VPEXTRW,
	NMD_X86_INSTRUCTION_VPGATHERDD,
	NMD_X86_INSTRUCTION_VPGATHERDQ,
	NMD_X86_INSTRUCTION_VPGATHERQD,
	NMD_X86_INSTRUCTION_VPGATHERQQ,
	NMD_X86_INSTRUCTION_VPHADDBD,
	NMD_X86_INSTRUCTION_VPHADDBQ,
	NMD_X86_INSTRUCTION_VPHADDBW,
	NMD_X86_INSTRUCTION_VPHADDUBD,
	NMD_X86_INSTRUCTION_VPHADDUBQ,
	NMD_X86_INSTRUCTION_VPHADDUBW,
	NMD_X86_INSTRUCTION_VPHADDUDQ,
	NMD_X86_INSTRUCTION_VPHADDUWD,
	NMD_X86_INSTRUCTION_VPHADDUWQ,
	NMD_X86_INSTRUCTION_VPHADDWD,
	NMD_X86_INSTRUCTION_VPHMINPOSUW,
	NMD_X86_INSTRUCTION_VPHSUBBW,
	NMD_X86_INSTRUCTION_VPHSUBDQ,

	NMD_X86_INSTRUCTION_VPHSUBWD,

	NMD_X86_INSTRUCTION_VPINSRB,
	NMD_X86_INSTRUCTION_VPINSRD,
	NMD_X86_INSTRUCTION_VPINSRQ,
	NMD_X86_INSTRUCTION_VPINSRW,
	NMD_X86_INSTRUCTION_VPLZCNTD,
	NMD_X86_INSTRUCTION_VPLZCNTQ,
	NMD_X86_INSTRUCTION_VPMACSDD,
	NMD_X86_INSTRUCTION_VPMACSDQH,
	NMD_X86_INSTRUCTION_VPMACSDQL,
	NMD_X86_INSTRUCTION_VPMACSSDD,
	NMD_X86_INSTRUCTION_VPMACSSDQH,
	NMD_X86_INSTRUCTION_VPMACSSDQL,
	NMD_X86_INSTRUCTION_VPMACSSWD,
	NMD_X86_INSTRUCTION_VPMACSSWW,
	NMD_X86_INSTRUCTION_VPMACSWD,
	NMD_X86_INSTRUCTION_VPMACSWW,
	NMD_X86_INSTRUCTION_VPMADCSSWD,
	NMD_X86_INSTRUCTION_VPMADCSWD,
	NMD_X86_INSTRUCTION_VPMADDWD,
	NMD_X86_INSTRUCTION_VPMASKMOVD,
	NMD_X86_INSTRUCTION_VPMASKMOVQ,
	NMD_X86_INSTRUCTION_VPMAXSB,
	NMD_X86_INSTRUCTION_VPMAXSD,
	NMD_X86_INSTRUCTION_VPMAXSQ,
	NMD_X86_INSTRUCTION_VPMAXSW,
	NMD_X86_INSTRUCTION_VPMAXUB,
	NMD_X86_INSTRUCTION_VPMAXUD,
	NMD_X86_INSTRUCTION_VPMAXUQ,
	NMD_X86_INSTRUCTION_VPMAXUW,
	NMD_X86_INSTRUCTION_VPMINSB,
	NMD_X86_INSTRUCTION_VPMINSD,
	NMD_X86_INSTRUCTION_VPMINSQ,
	NMD_X86_INSTRUCTION_VPMINSW,
	NMD_X86_INSTRUCTION_VPMINUB,
	NMD_X86_INSTRUCTION_VPMINUD,
	NMD_X86_INSTRUCTION_VPMINUQ,
	NMD_X86_INSTRUCTION_VPMINUW,
	NMD_X86_INSTRUCTION_VPMOVDB,
	NMD_X86_INSTRUCTION_VPMOVDW,
	NMD_X86_INSTRUCTION_VPMOVM2B,
	NMD_X86_INSTRUCTION_VPMOVM2D,
	NMD_X86_INSTRUCTION_VPMOVM2Q,
	NMD_X86_INSTRUCTION_VPMOVM2W,
	NMD_X86_INSTRUCTION_VPMOVMSKB,
	NMD_X86_INSTRUCTION_VPMOVQB,
	NMD_X86_INSTRUCTION_VPMOVQD,
	NMD_X86_INSTRUCTION_VPMOVQW,
	NMD_X86_INSTRUCTION_VPMOVSDB,
	NMD_X86_INSTRUCTION_VPMOVSDW,
	NMD_X86_INSTRUCTION_VPMOVSQB,
	NMD_X86_INSTRUCTION_VPMOVSQD,
	NMD_X86_INSTRUCTION_VPMOVSQW,
	NMD_X86_INSTRUCTION_VPMOVSXBD,
	NMD_X86_INSTRUCTION_VPMOVSXBQ,
	NMD_X86_INSTRUCTION_VPMOVSXBW,
	NMD_X86_INSTRUCTION_VPMOVSXDQ,
	NMD_X86_INSTRUCTION_VPMOVSXWD,
	NMD_X86_INSTRUCTION_VPMOVSXWQ,
	NMD_X86_INSTRUCTION_VPMOVUSDB,
	NMD_X86_INSTRUCTION_VPMOVUSDW,
	NMD_X86_INSTRUCTION_VPMOVUSQB,
	NMD_X86_INSTRUCTION_VPMOVUSQD,
	NMD_X86_INSTRUCTION_VPMOVUSQW,
	NMD_X86_INSTRUCTION_VPMOVZXBD,
	NMD_X86_INSTRUCTION_VPMOVZXBQ,
	NMD_X86_INSTRUCTION_VPMOVZXBW,
	NMD_X86_INSTRUCTION_VPMOVZXDQ,
	NMD_X86_INSTRUCTION_VPMOVZXWD,
	NMD_X86_INSTRUCTION_VPMOVZXWQ,
	NMD_X86_INSTRUCTION_VPMULDQ,
	NMD_X86_INSTRUCTION_VPMULHUW,
	NMD_X86_INSTRUCTION_VPMULHW,
	NMD_X86_INSTRUCTION_VPMULLD,
	NMD_X86_INSTRUCTION_VPMULLQ,
	NMD_X86_INSTRUCTION_VPMULLW,
	NMD_X86_INSTRUCTION_VPMULUDQ,
	NMD_X86_INSTRUCTION_VPORD,
	NMD_X86_INSTRUCTION_VPORQ,
	NMD_X86_INSTRUCTION_VPOR,
	NMD_X86_INSTRUCTION_VPPERM,
	NMD_X86_INSTRUCTION_VPROTB,
	NMD_X86_INSTRUCTION_VPROTD,
	NMD_X86_INSTRUCTION_VPROTQ,
	NMD_X86_INSTRUCTION_VPROTW,
	NMD_X86_INSTRUCTION_VPSADBW,
	NMD_X86_INSTRUCTION_VPSCATTERDD,
	NMD_X86_INSTRUCTION_VPSCATTERDQ,
	NMD_X86_INSTRUCTION_VPSCATTERQD,
	NMD_X86_INSTRUCTION_VPSCATTERQQ,
	NMD_X86_INSTRUCTION_VPSHAB,
	NMD_X86_INSTRUCTION_VPSHAD,
	NMD_X86_INSTRUCTION_VPSHAQ,
	NMD_X86_INSTRUCTION_VPSHAW,
	NMD_X86_INSTRUCTION_VPSHLB,
	NMD_X86_INSTRUCTION_VPSHLD,
	NMD_X86_INSTRUCTION_VPSHLQ,
	NMD_X86_INSTRUCTION_VPSHLW,
	NMD_X86_INSTRUCTION_VPSHUFD,
	NMD_X86_INSTRUCTION_VPSHUFHW,
	NMD_X86_INSTRUCTION_VPSHUFLW,
	NMD_X86_INSTRUCTION_VPSLLDQ,
	NMD_X86_INSTRUCTION_VPSLLD,
	NMD_X86_INSTRUCTION_VPSLLQ,
	NMD_X86_INSTRUCTION_VPSLLVD,
	NMD_X86_INSTRUCTION_VPSLLVQ,
	NMD_X86_INSTRUCTION_VPSLLW,
	NMD_X86_INSTRUCTION_VPSRAD,
	NMD_X86_INSTRUCTION_VPSRAQ,
	NMD_X86_INSTRUCTION_VPSRAVD,
	NMD_X86_INSTRUCTION_VPSRAVQ,
	NMD_X86_INSTRUCTION_VPSRAW,
	NMD_X86_INSTRUCTION_VPSRLDQ,
	NMD_X86_INSTRUCTION_VPSRLD,
	NMD_X86_INSTRUCTION_VPSRLQ,
	NMD_X86_INSTRUCTION_VPSRLVD,
	NMD_X86_INSTRUCTION_VPSRLVQ,
	NMD_X86_INSTRUCTION_VPSRLW,
	NMD_X86_INSTRUCTION_VPSUBB,
	NMD_X86_INSTRUCTION_VPSUBD,
	NMD_X86_INSTRUCTION_VPSUBQ,
	NMD_X86_INSTRUCTION_VPSUBSB,
	NMD_X86_INSTRUCTION_VPSUBSW,
	NMD_X86_INSTRUCTION_VPSUBUSB,
	NMD_X86_INSTRUCTION_VPSUBUSW,
	NMD_X86_INSTRUCTION_VPSUBW,
	NMD_X86_INSTRUCTION_VPTESTMD,
	NMD_X86_INSTRUCTION_VPTESTMQ,
	NMD_X86_INSTRUCTION_VPTESTNMD,
	NMD_X86_INSTRUCTION_VPTESTNMQ,
	NMD_X86_INSTRUCTION_VPTEST,
	NMD_X86_INSTRUCTION_VPUNPCKHBW,
	NMD_X86_INSTRUCTION_VPUNPCKHDQ,
	NMD_X86_INSTRUCTION_VPUNPCKHQDQ,
	NMD_X86_INSTRUCTION_VPUNPCKHWD,
	NMD_X86_INSTRUCTION_VPUNPCKLBW,
	NMD_X86_INSTRUCTION_VPUNPCKLDQ,
	NMD_X86_INSTRUCTION_VPUNPCKLQDQ,
	NMD_X86_INSTRUCTION_VPUNPCKLWD,
	NMD_X86_INSTRUCTION_VPXORD,
	NMD_X86_INSTRUCTION_VPXORQ,
	NMD_X86_INSTRUCTION_VPXOR,
	NMD_X86_INSTRUCTION_VRCP14PD,
	NMD_X86_INSTRUCTION_VRCP14PS,
	NMD_X86_INSTRUCTION_VRCP14SD,
	NMD_X86_INSTRUCTION_VRCP14SS,
	NMD_X86_INSTRUCTION_VRCP28PD,
	NMD_X86_INSTRUCTION_VRCP28PS,
	NMD_X86_INSTRUCTION_VRCP28SD,
	NMD_X86_INSTRUCTION_VRCP28SS,
	NMD_X86_INSTRUCTION_VRCPPS,
	NMD_X86_INSTRUCTION_VRCPSS,
	NMD_X86_INSTRUCTION_VRNDSCALEPD,
	NMD_X86_INSTRUCTION_VRNDSCALEPS,
	NMD_X86_INSTRUCTION_VRNDSCALESD,
	NMD_X86_INSTRUCTION_VRNDSCALESS,
	NMD_X86_INSTRUCTION_VROUNDPD,
	NMD_X86_INSTRUCTION_VROUNDPS,
	NMD_X86_INSTRUCTION_VROUNDSD,
	NMD_X86_INSTRUCTION_VROUNDSS,
	NMD_X86_INSTRUCTION_VRSQRT14PD,
	NMD_X86_INSTRUCTION_VRSQRT14PS,
	NMD_X86_INSTRUCTION_VRSQRT14SD,
	NMD_X86_INSTRUCTION_VRSQRT14SS,
	NMD_X86_INSTRUCTION_VRSQRT28PD,
	NMD_X86_INSTRUCTION_VRSQRT28PS,
	NMD_X86_INSTRUCTION_VRSQRT28SD,
	NMD_X86_INSTRUCTION_VRSQRT28SS,
	NMD_X86_INSTRUCTION_VRSQRTPS,
	NMD_X86_INSTRUCTION_VRSQRTSS,
	NMD_X86_INSTRUCTION_VSCATTERDPD,
	NMD_X86_INSTRUCTION_VSCATTERDPS,
	NMD_X86_INSTRUCTION_VSCATTERPF0DPD,
	NMD_X86_INSTRUCTION_VSCATTERPF0DPS,
	NMD_X86_INSTRUCTION_VSCATTERPF0QPD,
	NMD_X86_INSTRUCTION_VSCATTERPF0QPS,
	NMD_X86_INSTRUCTION_VSCATTERPF1DPD,
	NMD_X86_INSTRUCTION_VSCATTERPF1DPS,
	NMD_X86_INSTRUCTION_VSCATTERPF1QPD,
	NMD_X86_INSTRUCTION_VSCATTERPF1QPS,
	NMD_X86_INSTRUCTION_VSCATTERQPD,
	NMD_X86_INSTRUCTION_VSCATTERQPS,
	NMD_X86_INSTRUCTION_VSHUFPD,
	NMD_X86_INSTRUCTION_VSHUFPS,
	NMD_X86_INSTRUCTION_VSQRTPD,
	NMD_X86_INSTRUCTION_VSQRTPS,
	NMD_X86_INSTRUCTION_VSQRTSD,
	NMD_X86_INSTRUCTION_VSQRTSS,
	NMD_X86_INSTRUCTION_VSTMXCSR,
	NMD_X86_INSTRUCTION_VSUBPD,
	NMD_X86_INSTRUCTION_VSUBPS,
	NMD_X86_INSTRUCTION_VSUBSD,
	NMD_X86_INSTRUCTION_VSUBSS,
	NMD_X86_INSTRUCTION_VTESTPD,
	NMD_X86_INSTRUCTION_VTESTPS,
	NMD_X86_INSTRUCTION_VUNPCKHPD,
	NMD_X86_INSTRUCTION_VUNPCKHPS,
	NMD_X86_INSTRUCTION_VUNPCKLPD,
	NMD_X86_INSTRUCTION_VUNPCKLPS,
	NMD_X86_INSTRUCTION_VZEROALL,
	NMD_X86_INSTRUCTION_VZEROUPPER,
	NMD_X86_INSTRUCTION_FWAIT,
	NMD_X86_INSTRUCTION_XABORT,
	NMD_X86_INSTRUCTION_XACQUIRE,
	NMD_X86_INSTRUCTION_XBEGIN,
	NMD_X86_INSTRUCTION_XCHG,
	NMD_X86_INSTRUCTION_XCRYPTCBC,
	NMD_X86_INSTRUCTION_XCRYPTCFB,
	NMD_X86_INSTRUCTION_XCRYPTCTR,
	NMD_X86_INSTRUCTION_XCRYPTECB,
	NMD_X86_INSTRUCTION_XCRYPTOFB,
	NMD_X86_INSTRUCTION_XRELEASE,
	NMD_X86_INSTRUCTION_XRSTOR64,
	NMD_X86_INSTRUCTION_XRSTORS,
	NMD_X86_INSTRUCTION_XRSTORS64,
	NMD_X86_INSTRUCTION_XSAVE64,
	NMD_X86_INSTRUCTION_XSAVEC,
	NMD_X86_INSTRUCTION_XSAVEC64,
	NMD_X86_INSTRUCTION_XSAVEOPT64,
	NMD_X86_INSTRUCTION_XSAVES,
	NMD_X86_INSTRUCTION_XSAVES64,
	NMD_X86_INSTRUCTION_XSHA1,
	NMD_X86_INSTRUCTION_XSHA256,
	NMD_X86_INSTRUCTION_XSTORE,
	NMD_X86_INSTRUCTION_FDISI8087_NOP,
	NMD_X86_INSTRUCTION_FENI8087_NOP,

	/* pseudo instructions */
	NMD_X86_INSTRUCTION_CMPSS,
	NMD_X86_INSTRUCTION_CMPEQSS,
	NMD_X86_INSTRUCTION_CMPLTSS,
	NMD_X86_INSTRUCTION_CMPLESS,
	NMD_X86_INSTRUCTION_CMPUNORDSS,
	NMD_X86_INSTRUCTION_CMPNEQSS,
	NMD_X86_INSTRUCTION_CMPNLTSS,
	NMD_X86_INSTRUCTION_CMPNLESS,
	NMD_X86_INSTRUCTION_CMPORDSS,

	NMD_X86_INSTRUCTION_CMPSD,
	NMD_X86_INSTRUCTION_CMPEQSD,
	NMD_X86_INSTRUCTION_CMPLTSD,
	NMD_X86_INSTRUCTION_CMPLESD,
	NMD_X86_INSTRUCTION_CMPUNORDSD,
	NMD_X86_INSTRUCTION_CMPNEQSD,
	NMD_X86_INSTRUCTION_CMPNLTSD,
	NMD_X86_INSTRUCTION_CMPNLESD,
	NMD_X86_INSTRUCTION_CMPORDSD,

	NMD_X86_INSTRUCTION_CMPPS,
	NMD_X86_INSTRUCTION_CMPEQPS,
	NMD_X86_INSTRUCTION_CMPLTPS,
	NMD_X86_INSTRUCTION_CMPLEPS,
	NMD_X86_INSTRUCTION_CMPUNORDPS,
	NMD_X86_INSTRUCTION_CMPNEQPS,
	NMD_X86_INSTRUCTION_CMPNLTPS,
	NMD_X86_INSTRUCTION_CMPNLEPS,
	NMD_X86_INSTRUCTION_CMPORDPS,

	NMD_X86_INSTRUCTION_CMPPD,
	NMD_X86_INSTRUCTION_CMPEQPD,
	NMD_X86_INSTRUCTION_CMPLTPD,
	NMD_X86_INSTRUCTION_CMPLEPD,
	NMD_X86_INSTRUCTION_CMPUNORDPD,
	NMD_X86_INSTRUCTION_CMPNEQPD,
	NMD_X86_INSTRUCTION_CMPNLTPD,
	NMD_X86_INSTRUCTION_CMPNLEPD,
	NMD_X86_INSTRUCTION_CMPORDPD,

	NMD_X86_INSTRUCTION_VCMPSS,
	NMD_X86_INSTRUCTION_VCMPEQSS,
	NMD_X86_INSTRUCTION_VCMPLTSS,
	NMD_X86_INSTRUCTION_VCMPLESS,
	NMD_X86_INSTRUCTION_VCMPUNORDSS,
	NMD_X86_INSTRUCTION_VCMPNEQSS,
	NMD_X86_INSTRUCTION_VCMPNLTSS,
	NMD_X86_INSTRUCTION_VCMPNLESS,
	NMD_X86_INSTRUCTION_VCMPORDSS,
	NMD_X86_INSTRUCTION_VCMPEQ_UQSS,
	NMD_X86_INSTRUCTION_VCMPNGESS,
	NMD_X86_INSTRUCTION_VCMPNGTSS,
	NMD_X86_INSTRUCTION_VCMPFALSESS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQSS,
	NMD_X86_INSTRUCTION_VCMPGESS,
	NMD_X86_INSTRUCTION_VCMPGTSS,
	NMD_X86_INSTRUCTION_VCMPTRUESS,
	NMD_X86_INSTRUCTION_VCMPEQ_OSSS,
	NMD_X86_INSTRUCTION_VCMPLT_OQSS,
	NMD_X86_INSTRUCTION_VCMPLE_OQSS,
	NMD_X86_INSTRUCTION_VCMPUNORD_SSS,
	NMD_X86_INSTRUCTION_VCMPNEQ_USSS,
	NMD_X86_INSTRUCTION_VCMPNLT_UQSS,
	NMD_X86_INSTRUCTION_VCMPNLE_UQSS,
	NMD_X86_INSTRUCTION_VCMPORD_SSS,
	NMD_X86_INSTRUCTION_VCMPEQ_USSS,
	NMD_X86_INSTRUCTION_VCMPNGE_UQSS,
	NMD_X86_INSTRUCTION_VCMPNGT_UQSS,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSSS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSSS,
	NMD_X86_INSTRUCTION_VCMPGE_OQSS,
	NMD_X86_INSTRUCTION_VCMPGT_OQSS,
	NMD_X86_INSTRUCTION_VCMPTRUE_USSS,

	NMD_X86_INSTRUCTION_VCMPSD,
	NMD_X86_INSTRUCTION_VCMPEQSD,
	NMD_X86_INSTRUCTION_VCMPLTSD,
	NMD_X86_INSTRUCTION_VCMPLESD,
	NMD_X86_INSTRUCTION_VCMPUNORDSD,
	NMD_X86_INSTRUCTION_VCMPNEQSD,
	NMD_X86_INSTRUCTION_VCMPNLTSD,
	NMD_X86_INSTRUCTION_VCMPNLESD,
	NMD_X86_INSTRUCTION_VCMPORDSD,
	NMD_X86_INSTRUCTION_VCMPEQ_UQSD,
	NMD_X86_INSTRUCTION_VCMPNGESD,
	NMD_X86_INSTRUCTION_VCMPNGTSD,
	NMD_X86_INSTRUCTION_VCMPFALSESD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQSD,
	NMD_X86_INSTRUCTION_VCMPGESD,
	NMD_X86_INSTRUCTION_VCMPGTSD,
	NMD_X86_INSTRUCTION_VCMPTRUESD,
	NMD_X86_INSTRUCTION_VCMPEQ_OSSD,
	NMD_X86_INSTRUCTION_VCMPLT_OQSD,
	NMD_X86_INSTRUCTION_VCMPLE_OQSD,
	NMD_X86_INSTRUCTION_VCMPUNORD_SSD,
	NMD_X86_INSTRUCTION_VCMPNEQ_USSD,
	NMD_X86_INSTRUCTION_VCMPNLT_UQSD,
	NMD_X86_INSTRUCTION_VCMPNLE_UQSD,
	NMD_X86_INSTRUCTION_VCMPORD_SSD,
	NMD_X86_INSTRUCTION_VCMPEQ_USSD,
	NMD_X86_INSTRUCTION_VCMPNGE_UQSD,
	NMD_X86_INSTRUCTION_VCMPNGT_UQSD,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSSD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSSD,
	NMD_X86_INSTRUCTION_VCMPGE_OQSD,
	NMD_X86_INSTRUCTION_VCMPGT_OQSD,
	NMD_X86_INSTRUCTION_VCMPTRUE_USSD,

	NMD_X86_INSTRUCTION_VCMPPS,
	NMD_X86_INSTRUCTION_VCMPEQPS,
	NMD_X86_INSTRUCTION_VCMPLTPS,
	NMD_X86_INSTRUCTION_VCMPLEPS,
	NMD_X86_INSTRUCTION_VCMPUNORDPS,
	NMD_X86_INSTRUCTION_VCMPNEQPS,
	NMD_X86_INSTRUCTION_VCMPNLTPS,
	NMD_X86_INSTRUCTION_VCMPNLEPS,
	NMD_X86_INSTRUCTION_VCMPORDPS,
	NMD_X86_INSTRUCTION_VCMPEQ_UQPS,
	NMD_X86_INSTRUCTION_VCMPNGEPS,
	NMD_X86_INSTRUCTION_VCMPNGTPS,
	NMD_X86_INSTRUCTION_VCMPFALSEPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQPS,
	NMD_X86_INSTRUCTION_VCMPGEPS,
	NMD_X86_INSTRUCTION_VCMPGTPS,
	NMD_X86_INSTRUCTION_VCMPTRUEPS,
	NMD_X86_INSTRUCTION_VCMPEQ_OSPS,
	NMD_X86_INSTRUCTION_VCMPLT_OQPS,
	NMD_X86_INSTRUCTION_VCMPLE_OQPS,
	NMD_X86_INSTRUCTION_VCMPUNORD_SPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_USPS,
	NMD_X86_INSTRUCTION_VCMPNLT_UQPS,
	NMD_X86_INSTRUCTION_VCMPNLE_UQPS,
	NMD_X86_INSTRUCTION_VCMPORD_SPS,
	NMD_X86_INSTRUCTION_VCMPEQ_USPS,
	NMD_X86_INSTRUCTION_VCMPNGE_UQPS,
	NMD_X86_INSTRUCTION_VCMPNGT_UQPS,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSPS,
	NMD_X86_INSTRUCTION_VCMPGE_OQPS,
	NMD_X86_INSTRUCTION_VCMPGT_OQPS,
	NMD_X86_INSTRUCTION_VCMPTRUE_USPS,

	NMD_X86_INSTRUCTION_VCMPPD,
	NMD_X86_INSTRUCTION_VCMPEQPD,
	NMD_X86_INSTRUCTION_VCMPLTPD,
	NMD_X86_INSTRUCTION_VCMPLEPD,
	NMD_X86_INSTRUCTION_VCMPUNORDPD,
	NMD_X86_INSTRUCTION_VCMPNEQPD,
	NMD_X86_INSTRUCTION_VCMPNLTPD,
	NMD_X86_INSTRUCTION_VCMPNLEPD,
	NMD_X86_INSTRUCTION_VCMPORDPD,
	NMD_X86_INSTRUCTION_VCMPEQ_UQPD,
	NMD_X86_INSTRUCTION_VCMPNGEPD,
	NMD_X86_INSTRUCTION_VCMPNGTPD,
	NMD_X86_INSTRUCTION_VCMPFALSEPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQPD,
	NMD_X86_INSTRUCTION_VCMPGEPD,
	NMD_X86_INSTRUCTION_VCMPGTPD,
	NMD_X86_INSTRUCTION_VCMPTRUEPD,
	NMD_X86_INSTRUCTION_VCMPEQ_OSPD,
	NMD_X86_INSTRUCTION_VCMPLT_OQPD,
	NMD_X86_INSTRUCTION_VCMPLE_OQPD,
	NMD_X86_INSTRUCTION_VCMPUNORD_SPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_USPD,
	NMD_X86_INSTRUCTION_VCMPNLT_UQPD,
	NMD_X86_INSTRUCTION_VCMPNLE_UQPD,
	NMD_X86_INSTRUCTION_VCMPORD_SPD,
	NMD_X86_INSTRUCTION_VCMPEQ_USPD,
	NMD_X86_INSTRUCTION_VCMPNGE_UQPD,
	NMD_X86_INSTRUCTION_VCMPNGT_UQPD,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSPD,
	NMD_X86_INSTRUCTION_VCMPGE_OQPD,
	NMD_X86_INSTRUCTION_VCMPGT_OQPD,
	NMD_X86_INSTRUCTION_VCMPTRUE_USPD,

	NMD_X86_INSTRUCTION_UD0,
	NMD_X86_INSTRUCTION_ENDBR32,
	NMD_X86_INSTRUCTION_ENDBR64,
};

enum NMD_X86_OPERAND_TYPE
{
	NMD_X86_OPERAND_TYPE_NONE = 0,
	NMD_X86_OPERAND_TYPE_REGISTER,
	NMD_X86_OPERAND_TYPE_MEMORY,
	NMD_X86_OPERAND_TYPE_IMMEDIATE,
};

typedef struct nmd_x86_memory_operand
{
	uint8_t segment;     /* The segment register. A member of 'NMD_X86_REG'. */
	uint8_t base;        /* The base register. A member of 'NMD_X86_REG'. */
	uint8_t index;       /* The index register. A member of 'NMD_X86_REG'. */
	uint8_t scale;       /* Scale(1, 2, 4 or 8). */
	int64_t disp;        /* Displacement. */
} nmd_x86_memory_operand;

enum NMD_X86_OPERAND_ACTION
{
	NMD_X86_OPERAND_ACTION_NONE = 0, /* The operand is neither read from nor written to. */

	NMD_X86_OPERAND_ACTION_READ = (1 << 0), /* The operand is read. */
	NMD_X86_OPERAND_ACTION_WRITE = (1 << 1), /* The operand is modified. */
	NMD_X86_OPERAND_ACTION_CONDREAD = (1 << 2), /* The operand may be read depending on some condition(conditional read). */
	NMD_X86_OPERAND_ACTION_CONDWRITE = (1 << 3), /* The operand may be modified depending on some condition(conditional write). */

	/* These are not actual actions, but rather masks of actions. */
	NMD_X86_OPERAND_ACTION_READWRITE = (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_WRITE),
	NMD_X86_OPERAND_ACTION_ANY_READ = (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDREAD),
	NMD_X86_OPERAND_ACTION_ANY_WRITE = (NMD_X86_OPERAND_ACTION_WRITE | NMD_X86_OPERAND_ACTION_CONDWRITE)
};

typedef struct nmd_x86_operand
{
	uint8_t type;                  /* The operand's type. A member of 'NMD_X86_OPERAND_TYPE'. */
	/* uint8_t size;                The operand's size. (I don't really know what this `size` variable represents or how it would be useful) */
	bool is_implicit;              /* If true, the operand does not appear on the intruction's formatted form. */
	uint8_t action;                /* The action on the operand. A member of 'NMD_X86_OPERAND_ACTION'. */
	union {                        /* The operand's "raw" data. */
		uint8_t reg;               /* Register operand. A variable of type 'NMD_X86_REG' */
		int64_t imm;               /* Immediate operand. */
		nmd_x86_memory_operand mem;  /* Memory operand. */
	} fields;
} nmd_x86_operand;

typedef union nmd_x86_cpu_flags
{
	struct
	{
		uint8_t CF : 1; /* Bit  0.    Carry Flag (CF) */
		uint8_t b1 : 1; /* Bit  1.    Reserved */
		uint8_t PF : 1; /* Bit  2.    Parity Flag (PF) */
		uint8_t B3 : 1; /* Bit  3.    Reserved */
		uint8_t AF : 1; /* Bit  4.    Auxiliary Carry Flag (AF) */
		uint8_t B5 : 1; /* Bit  5.    Reserved */
		uint8_t ZF : 1; /* Bit  6.    Zero flag(ZF) */
		uint8_t SF : 1; /* Bit  7.    Sign flag(SF) */
		uint8_t TF : 1; /* Bit  8.    Trap flag(TF) */
		uint8_t IF : 1; /* Bit  9.    Interrupt Enable Flag (IF) */
		uint8_t DF : 1; /* Bit 10.    Direction Flag (DF) */
		uint8_t OF : 1; /* Bit 11.    Overflow Flag (OF) */
		uint8_t IOPL : 2; /* Bit 12,13. I/O Privilege Level (IOPL) */
		uint8_t NT : 1; /* Bit 14.    Nested Task (NT) */
		uint8_t B15 : 1; /* Bit 15.    Reserved */
		uint8_t RF : 1; /* Bit 16.    Resume Flag (RF) */
		uint8_t VM : 1; /* Bit 17.    Virtual-8086 Mode (VM) */
		uint8_t AC : 1; /* Bit 18.    Alignment Check / Access Control (AC) */
		uint8_t VIF : 1; /* Bit 19.    Virtual Interrupt Flag (VIF) */
		uint8_t VIP : 1; /* Bit 20.    Virtual Interrupt Pending (VIP) */
		uint8_t ID : 1; /* Bit 21.    ID Flag(ID) */
		uint8_t B22 : 1; /* Bit 22.    Reserved */
		uint8_t B23 : 1; /* Bit 23.    Reserved */
		uint8_t B24 : 1; /* Bit 24.    Reserved */
		uint8_t B25 : 1; /* Bit 25.    Reserved */
		uint8_t B26 : 1; /* Bit 26.    Reserved */
		uint8_t B27 : 1; /* Bit 27.    Reserved */
		uint8_t B28 : 1; /* Bit 28.    Reserved */
		uint8_t B29 : 1; /* Bit 29.    Reserved */
		uint8_t B30 : 1; /* Bit 30.    Reserved */
		uint8_t B31 : 1; /* Bit 31.    Reserved */
	} fields;
	struct
	{
		uint8_t IE : 1; /* Bit  0.    Invalid Operation (IE) */
		uint8_t DE : 1; /* Bit  1.    Denormalized Operand (DE) */
		uint8_t ZE : 1; /* Bit  2.    Zero Divide (ZE) */
		uint8_t OE : 1; /* Bit  3.    Overflow (OE) */
		uint8_t UE : 1; /* Bit  4.    Underflow (UE) */
		uint8_t PE : 1; /* Bit  5.    Precision (PE) */
		uint8_t SF : 1; /* Bit  6.    Stack Fault (SF) */
		uint8_t ES : 1; /* Bit  7.    Exception Summary Status (ES) */
		uint8_t C0 : 1; /* Bit  8.    Condition code 0 (C0) */
		uint8_t C1 : 1; /* Bit  9.    Condition code 1 (C1) */
		uint8_t C2 : 1; /* Bit 10.    Condition code 2 (C2) */
		uint8_t TOP : 3; /* Bit 11-13. Top of Stack Pointer (TOP) */
		uint8_t C3 : 1; /* Bit 14.    Condition code 3 (C3) */
		uint8_t B : 1; /* Bit 15.    FPU Busy (B) */
	} fpu_fields;
	uint8_t l8;
	uint32_t eflags;
	uint16_t fpu_flags;
} nmd_x86_cpu_flags;

enum NMD_X86_EFLAGS
{
	NMD_X86_EFLAGS_ID = (1 << 21),
	NMD_X86_EFLAGS_VIP = (1 << 20),
	NMD_X86_EFLAGS_VIF = (1 << 19),
	NMD_X86_EFLAGS_AC = (1 << 18),
	NMD_X86_EFLAGS_VM = (1 << 17),
	NMD_X86_EFLAGS_RF = (1 << 16),
	NMD_X86_EFLAGS_NT = (1 << 14),
	NMD_X86_EFLAGS_IOPL = (1 << 12) /*| (1 << 13)*/,
	NMD_X86_EFLAGS_OF = (1 << 11),
	NMD_X86_EFLAGS_DF = (1 << 10),
	NMD_X86_EFLAGS_IF = (1 << 9),
	NMD_X86_EFLAGS_TF = (1 << 8),
	NMD_X86_EFLAGS_SF = (1 << 7),
	NMD_X86_EFLAGS_ZF = (1 << 6),
	NMD_X86_EFLAGS_AF = (1 << 4),
	NMD_X86_EFLAGS_PF = (1 << 2),
	NMD_X86_EFLAGS_CF = (1 << 0)
};

enum NMD_X86_FPU_FLAGS
{
	NMD_X86_FPU_FLAGS_C0 = (1 << 8),
	NMD_X86_FPU_FLAGS_C1 = (1 << 9),
	NMD_X86_FPU_FLAGS_C2 = (1 << 10),
	NMD_X86_FPU_FLAGS_C3 = (1 << 14)
};

typedef struct nmd_x86_instruction
{
	bool valid : 1;                                         /* If true, the instruction is valid. */
	bool has_modrm : 1;                                     /* If true, the instruction has a ModR/M byte. */
	bool has_sib : 1;                                       /* If true, the instruction has an SIB byte. */
	bool has_rex : 1;                                       /* If true, the instruction has a REX prefix */
	bool rex_w_prefix : 1;                                  /* If true, a REX.W prefix is closer to the opcode than a operand size override prefix. */
	bool repeat_prefix : 1;                                 /* If true, a 'repeat'(F3h) prefix is closer to the opcode than a 'repeat not zero'(F2h) prefix. */
	uint8_t mode;                                           /* The decoding mode. A member of 'NMD_X86_MODE'. */
	uint8_t length;                                         /* The instruction's length in bytes. */
	uint8_t opcode;                                         /* Opcode byte. */
	uint8_t opcode_size;                                    /* The opcode's size in bytes. */
	uint16_t id;                                            /* The instruction's identifier. A member of 'NMD_X86_INSTRUCTION'. */
	uint16_t prefixes;                                      /* A mask of prefixes. See 'NMD_X86_PREFIXES'. */
	uint8_t num_prefixes;                                   /* Number of prefixes. */
	uint8_t num_operands;                                   /* The number of operands. */
	uint8_t group;                                          /* The instruction's group(e.g. jmp, prvileged...). A member of 'NMD_GROUP'. */
	uint8_t buffer[NMD_X86_MAXIMUM_INSTRUCTION_LENGTH];     /* A buffer containing the full instruction. */
	nmd_x86_operand operands[NMD_X86_MAXIMUM_NUM_OPERANDS]; /* Operands. */
	nmd_x86_modrm modrm;                                    /* The Mod/RM byte. Check 'flags.fields.has_modrm'. */
	nmd_x86_sib sib;                                        /* The SIB byte. Check 'flags.fields.has_sib'. */
	uint8_t imm_mask;                                       /* A mask of one or more members of 'NMD_X86_IMM'. */
	uint8_t disp_mask;                                      /* A mask of one or more members of 'NMD_X86_DISP'. */
	uint64_t immediate;                                     /* Immediate. Check 'imm_mask'. */
	uint32_t displacement;                                  /* Displacement. Check 'disp_mask'. */
	uint8_t opcode_map;                                     /* The instruction's opcode map. A member of 'NMD_X86_OPCODE_MAP'. */
	uint8_t encoding;                                       /* The instruction's encoding. A member of 'NMD_X86_INSTRUCTION_ENCODING'. */
	nmd_x86_vex vex;                                        /* VEX prefix. */
	nmd_x86_cpu_flags modified_flags;                       /* Cpu flags modified by the instruction. */
	nmd_x86_cpu_flags tested_flags;                         /* Cpu flags tested by the instruction. */
	nmd_x86_cpu_flags set_flags;                            /* Cpu flags set by the instruction. */
	nmd_x86_cpu_flags cleared_flags;                        /* Cpu flags cleared by the instruction. */
	nmd_x86_cpu_flags undefined_flags;                      /* Cpu flags whose state is undefined. */
	uint8_t rex;                                            /* REX prefix. */
	uint8_t segment_override;                               /* The segment override prefix closest to the opcode. A member of 'NMD_X86_PREFIXES'. */
	uint16_t simd_prefix;                                   /* One of these prefixes that is the closest to the opcode: NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE, NMD_X86_PREFIXES_LOCK, NMD_X86_PREFIXES_REPEAT_NOT_ZERO, NMD_X86_PREFIXES_REPEAT, or NMD_X86_PREFIXES_NONE. The prefixes are specified as members of the 'NMD_X86_PREFIXES' enum. */
} nmd_x86_instruction;

typedef union nmd_x86_register
{
	int8_t  h8;
	int8_t  l8;
	int16_t l16;
	int32_t l32;
	int64_t l64;
} nmd_x86_register;

typedef union nmd_x86_register_512
{
	uint64_t xmm0[2];
	uint64_t ymm0[4];
	uint64_t zmm0[8];
} nmd_x86_register_512;

/*
Assembles one or more instructions from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using the '\n'(new line) character.
Parameters:
 - string          [in]         A pointer to a string that represents one or more instructions in assembly language.
 - buffer          [out]        A pointer to a buffer that receives the encoded instructions.
 - buffer_size     [in]         The size of the buffer in bytes.
 - runtime_address [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
 - mode            [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
 - count           [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed, and on output the number of instructions parsed. This parameter may be null.
*/
NMD_ASSEMBLY_API size_t nmd_x86_assemble(const char* string, void* buffer, size_t buffer_size, uint64_t runtime_address, NMD_X86_MODE mode, size_t* count);

/*
Decodes an instruction. Returns true if the instruction is valid, false otherwise.
Parameters:
 - buffer      [in]  A pointer to a buffer containing a encoded instruction.
 - buffer_size [in]  The buffer's size in bytes.
 - instruction [out] A pointer to a variable of type 'nmd_x86_instruction' that receives information about the instruction.
 - mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
 - flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
*/
NMD_ASSEMBLY_API bool nmd_x86_decode(const void* buffer, size_t buffer_size, nmd_x86_instruction* instruction, NMD_X86_MODE mode, uint32_t flags);

/*
Formats an instruction. This function may cause a crash if you modify 'instruction' manually.
Parameters:
 - instruction     [in]  A pointer to a variable of type 'nmd_x86_instruction' describing the instruction to be formatted.
 - buffer          [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
 - runtime_address [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
 - flags           [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
*/
NMD_ASSEMBLY_API void nmd_x86_format(const nmd_x86_instruction* instruction, char* buffer, uint64_t runtime_address, uint32_t flags);

/*
Returns the instruction's length if it's valid, zero otherwise.
Parameters:
 - buffer      [in] A pointer to a buffer containing a encoded instruction.
 - buffer_size [in] The buffer's size in bytes.
 - mode        [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
*/
NMD_ASSEMBLY_API size_t nmd_x86_ldisasm(const void* buffer, size_t buffer_size, NMD_X86_MODE mode);

#endif /* NMD_ASSEMBLY_H */


#ifdef NMD_ASSEMBLY_IMPLEMENTATION

/* Four high-order bits of an opcode to index a row of the opcode table */
#define _NMD_R(b) ((b) >> 4)

/* Four low-order bits to index a column of the table */
#define _NMD_C(b) ((b) & 0xF)

#define _NMD_NUM_ELEMENTS(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifndef _NMD_IS_UPPERCASE
#define _NMD_IS_UPPERCASE(c) ((c) >= 'A' && (c) <= 'Z')
#define _NMD_IS_LOWERCASE(c) ((c) >= 'a' && (c) <= 'z')
#define _NMD_TOLOWER(c) (_NMD_IS_UPPERCASE(c) ? (c) + 0x20 : (c))
#define _NMD_IS_DECIMAL_NUMBER(c) ((c) >= '0' && (c) <= '9')
#define _NMD_MIN(a, b) ((a)<(b)?(a):(b))
#define _NMD_MAX(a, b) ((a)>(b)?(a):(b))
#endif /* _NMD_IS_UPPERCASE */  

#define _NMD_SET_REG_OPERAND(operand, _is_implicit, _action, _reg) {operand.type = NMD_X86_OPERAND_TYPE_REGISTER; operand.is_implicit = _is_implicit; operand.action = _action; operand.fields.reg = _reg;}
#define _NMD_SET_IMM_OPERAND(operand, _is_implicit, _action, _imm) {operand.type = NMD_X86_OPERAND_TYPE_IMMEDIATE; operand.is_implicit = _is_implicit; operand.action = _action; operand.fields.imm = _imm;}
#define _NMD_SET_MEM_OPERAND(operand, _is_implicit, _action, _segment, _base, _index, _scale, _disp) {operand.type = NMD_X86_OPERAND_TYPE_MEMORY; operand.is_implicit = _is_implicit; operand.action = _action; operand.fields.mem.segment = _segment; operand.fields.mem.base = _base; operand.fields.mem.index = _index; operand.fields.mem.scale = _scale; operand.fields.mem.disp = _disp;}
#define _NMD_GET_GPR(reg) (reg + (instruction->mode>>2)*8) /* reg(16),reg(32),reg(64). e.g. ax,eax,rax */
#define _NMD_GET_IP() (NMD_X86_REG_IP + (instruction->mode>>2)) /* ip,eip,rip */					
#define _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, _16, _32) ((mode) == NMD_X86_MODE_16 ? ((opszprfx) ? (_32) : (_16)) : ((opszprfx) ? (_16) : (_32))) /* Get something based on mode and operand size prefix. Used for instructions where the the 64-bit mode variant does not exist or is the same as the one for 32-bit mode */
#define _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, rex_w_prefix, _16, _32, _64) ((mode) == NMD_X86_MODE_16 ? ((opszprfx) ? (_32) : (_16)) : ((opszprfx) ? (_16) : ((rex_w_prefix) ? (_64) : (_32)))) /* Get something based on mode and operand size prefix. The 64-bit version is accessed with the REX.W prefix */
#define _NMD_GET_BY_MODE_OPSZPRFX_D64(mode, opszprfx, _16, _32, _64) ((mode) == NMD_X86_MODE_16 ? ((opszprfx) ? (_32) : (_16)) : ((opszprfx) ? (_16) : ((mode) == NMD_X86_MODE_64 ? (_64) : (_32)))) /* Get something based on mode and operand size prefix. The 64-bit version is accessed by default when mode is NMD_X86_MODE_64 and there's no operand size override prefix. */
#define _NMD_GET_BY_MODE_OPSZPRFX_F64(mode, opszprfx, _16, _32, _64) ((mode) == NMD_X86_MODE_64 ? (_64) : ((mode) == NMD_X86_MODE_16 ? ((opszprfx) ? (_32) : (_16)) : ((opszprfx) ? (_16) : (_32)))) /* Get something based on mode and operand size prefix. The 64-bit version is accessed when mode is NMD_X86_MODE_64 independent of an operand size override prefix. */

/* Make sure we can read a byte, read a byte, increment the buffer and decrement the buffer's size */
#define _NMD_READ_BYTE(buffer_, buffer_size_, var_) { if ((buffer_size_) < sizeof(uint8_t)) { return false; } var_ = *((uint8_t*)(buffer_)); buffer_ = ((uint8_t*)(buffer_)) + sizeof(uint8_t); (buffer_size_) -= sizeof(uint8_t); }

NMD_ASSEMBLY_API const char* const _nmd_reg8[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
NMD_ASSEMBLY_API const char* const _nmd_reg8_x64[] = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil" };
NMD_ASSEMBLY_API const char* const _nmd_reg16[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
NMD_ASSEMBLY_API const char* const _nmd_reg32[] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
NMD_ASSEMBLY_API const char* const _nmd_reg64[] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi" };
NMD_ASSEMBLY_API const char* const _nmd_regrxb[] = { "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
NMD_ASSEMBLY_API const char* const _nmd_regrxw[] = { "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
NMD_ASSEMBLY_API const char* const _nmd_regrxd[] = { "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
NMD_ASSEMBLY_API const char* const _nmd_regrx[] = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
NMD_ASSEMBLY_API const char* const _nmd_segment_reg[] = { "es", "cs", "ss", "ds", "fs", "gs" };

NMD_ASSEMBLY_API const char* const _nmd_condition_suffixes[] = { "o", "no", "b", "nb", "z", "nz", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g" };

NMD_ASSEMBLY_API const char* const _nmd_op1_opcode_map_mnemonics[] = { "add", "adc", "and", "xor", "or", "sbb", "sub", "cmp" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp1[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp2[] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp3[] = { "test", "test", "not", "neg", "mul", "imul", "div", "idiv" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp5[] = { "inc", "dec", "call", "call far", "jmp", "jmp far", "push" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp6[] = { "sldt", "str", "lldt", "ltr", "verr", "verw" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7[] = { "sgdt", "sidt", "lgdt", "lidt", "smsw", 0, "lmsw", "invlpg" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7_reg0[] = { "enclv", "vmcall", "vmlaunch", "vmresume", "vmxoff", "pconfig" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7_reg1[] = { "monitor", "mwait", "clac", "stac", 0, 0, 0, "encls" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7_reg2[] = { "xgetbv", "xsetbv", 0, 0, "vmfunc", "xend", "xtest", "enclu" };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7_reg3[] = { "vmrun ", "vmmcall", "vmload ", "vmsave", "stgi", "clgi", "skinit eax", "invlpga " };
NMD_ASSEMBLY_API const char* const _nmd_opcode_extensions_grp7_reg7[] = { "swapgs", "rdtscp", "monitorx", "mwaitx", "clzero ", "rdpru" };

NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesD8[] = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesD9[] = { "ld", 0, "st", "stp", "ldenv", "ldcw", "nstenv", "nstcw" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesDA_DE[] = { "iadd", "imul", "icom", "icomp", "isub", "isubr", "idiv", "idivr" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesDB[] = { "ild", "isttp", "ist", "istp", 0, "ld", 0, "stp" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesDC[] = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesDD[] = { "ld", "isttp", "st", "stp", "rstor", 0, "nsave", "nstsw" };
NMD_ASSEMBLY_API const char* const _nmd_escape_opcodesDF[] = { "ild", "isttp", "ist", "istp", "bld", "ild", "bstp", "istp" };
NMD_ASSEMBLY_API const char* const* _nmd_escape_opcodes[] = { _nmd_escape_opcodesD8, _nmd_escape_opcodesD9, _nmd_escape_opcodesDA_DE, _nmd_escape_opcodesDB, _nmd_escape_opcodesDC, _nmd_escape_opcodesDD, _nmd_escape_opcodesDA_DE, _nmd_escape_opcodesDF };

NMD_ASSEMBLY_API const uint8_t _nmd_op1_modrm[] = { 0xFF, 0x63, 0x69, 0x6B, 0xC0, 0xC1, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE };
NMD_ASSEMBLY_API const uint8_t _nmd_op1_imm8[] = { 0x6A, 0x6B, 0x80, 0x82, 0x83, 0xA8, 0xC0, 0xC1, 0xC6, 0xCD, 0xD4, 0xD5, 0xEB };
NMD_ASSEMBLY_API const uint8_t _nmd_op1_imm32[] = { 0xE8, 0xE9, 0x68, 0x81, 0x69, 0xA9, 0xC7 };
NMD_ASSEMBLY_API const uint8_t _nmd_invalid_op2[] = { 0x04, 0x0a, 0x0c, 0x7a, 0x7b, 0x36, 0x39 };
NMD_ASSEMBLY_API const uint8_t _nmd_two_opcodes[] = { 0xb0, 0xb1, 0xb3, 0xbb, 0xc0, 0xc1 };
NMD_ASSEMBLY_API const uint8_t _nmd_valid_3DNow_opcodes[] = { 0x0c, 0x0d, 0x1c, 0x1d, 0x8a, 0x8e, 0x90, 0x94, 0x96, 0x97, 0x9a, 0x9e, 0xa0, 0xa4, 0xa6, 0xa7, 0xaa, 0xae, 0xb0, 0xb4, 0xb6, 0xb7, 0xbb, 0xbf };

NMD_ASSEMBLY_API bool _nmd_find_byte(const uint8_t* arr, const size_t N, const uint8_t x)
{
	size_t i = 0;
	for (; i < N; i++)
	{
		if (arr[i] == x)
			return true;
	};

	return false;
}

/* Returns a pointer to the first occurrence of 'c' in 's', or a null pointer if 'c' is not present. */
NMD_ASSEMBLY_API const char* _nmd_strchr(const char* s, char c)
{
	for (; *s; s++)
	{
		if (*s == c)
			return s;
	}

	return 0;
}

/* Returns a pointer to the last occurrence of 'c' in 's', or a null pointer if 'c' is not present. */
NMD_ASSEMBLY_API const char* _nmd_reverse_strchr(const char* s, char c)
{
	const char* end = s;
	while (*end)
		end++;

	for (; end > s; end--)
	{
		if (*end == c)
			return end;
	}

	return 0;
}

/* Returns a pointer to the first occurrence of 's2' in 's', or a null pointer if 's2' is not present. */
NMD_ASSEMBLY_API const char* _nmd_strstr(const char* s, const char* s2)
{
	size_t i = 0;
	for (; *s; s++)
	{
		if (s2[i] == '\0')
			return s - i;

		if (*s != s2[i])
			i = 0;

		if (*s == s2[i])
			i++;
	}

	return 0;
}

/* Returns a pointer to the first occurrence of 's2' in 's', or a null pointer if 's2' is not present. If 's3_opt' is not null it receives the address of the next byte in 's'. */
NMD_ASSEMBLY_API const char* _nmd_strstr_ex(const char* s, const char* s2, const char** s3_opt)
{
	size_t i = 0;
	for (; *s; s++)
	{
		if (s2[i] == '\0')
		{
			if (s3_opt)
				*s3_opt = s;
			return s - i;
		}

		if (*s != s2[i])
			i = 0;

		if (*s == s2[i])
			i++;
	}

	if (s2[i] == '\0')
	{
		if (s3_opt)
			*s3_opt = s;
		return s - i;
	}

	return 0;
}


/* Inserts 'c' at 's'. */
NMD_ASSEMBLY_API void _nmd_insert_char(const char* s, char c)
{
	char* end = (char*)s;
	while (*end)
		end++;

	*(end + 1) = '\0';

	for (; end > s; end--)
		*end = *(end - 1);

	*end = c;
}

/* Returns true if there is only a number between 's1' and 's2', false otherwise. */
NMD_ASSEMBLY_API bool _nmd_is_number(const char* s1, const char* s2)
{
	const char* s = s1;
	for (; s < s2; s++)
	{
		if (!(*s >= '0' && *s <= '9') && !(*s >= 'a' && *s <= 'f') && !(*s >= 'A' && *s <= 'F'))
		{
			if ((s == s1 + 1 && *s1 == '0' && (*s == 'x' || *s == 'X')) || (s == s2 - 1 && (*s == 'h' || *s == 'H')))
				continue;

			return false;
		}
	}

	return true;
}

/* Returns a pointer to the first occurrence of a number between 's1' and 's2', zero otherwise. */
NMD_ASSEMBLY_API const char* _nmd_find_number(const char* s1, const char* s2)
{
	const char* s = s1;
	for (; s < s2; s++)
	{
		if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))
			return s;
	}

	return 0;
}

/* Returns true if s1 matches s2 exactly. */
NMD_ASSEMBLY_API bool _nmd_strcmp(const char* s1, const char* s2)
{
	for (; *s1 && *s2; s1++, s2++)
	{
		if (*s1 != *s2)
			return false;
	}

	return !*s1 && !*s2;
}

NMD_ASSEMBLY_API size_t _nmd_get_bit_index(uint32_t mask)
{
	size_t i = 0;
	while (!(mask & (1 << i)))
		i++;

	return i;
}

NMD_ASSEMBLY_API size_t _nmd_assembly_get_num_digits_hex(uint64_t n)
{
	if (n == 0)
		return 1;

	size_t num_digits = 0;
	for (; n > 0; n /= 16)
		num_digits++;

	return num_digits;
}

NMD_ASSEMBLY_API size_t _nmd_assembly_get_num_digits(uint64_t n)
{
	if (n == 0)
		return 1;

	size_t num_digits = 0;
	for (; n > 0; n /= 10)
		num_digits++;

	return num_digits;
}


typedef struct _nmd_assemble_info
{
	char* s; /* string */
	uint8_t* b; /* buffer */
	NMD_X86_MODE mode;
	uint64_t runtime_address;
} _nmd_assemble_info;

enum _NMD_NUMBER_BASE
{
	_NMD_NUMBER_BASE_NONE = 0,
	_NMD_NUMBER_BASE_DECIMAL = 10,
	_NMD_NUMBER_BASE_HEXADECIMAL = 16,
	_NMD_NUMBER_BASE_BINARY = 2
};

NMD_ASSEMBLY_API size_t _nmd_assemble_reg(_nmd_assemble_info* ai, uint8_t base_byte)
{
	uint8_t i = 0;
	if (ai->mode == NMD_X86_MODE_64)
	{
		for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg64); i++)
		{
			if (_nmd_strcmp(ai->s, _nmd_reg64[i]))
			{
				ai->b[0] = base_byte + i;
				return 1;
			}
		}

		for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrx); i++)
		{
			if (_nmd_strcmp(ai->s, _nmd_regrx[i]))
			{
				ai->b[0] = 0x41;
				ai->b[1] = base_byte + i;
				return 2;
			}
		}
	}
	else if (ai->mode == NMD_X86_MODE_32)
	{
		for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg32); i++)
		{
			if (_nmd_strcmp(ai->s, _nmd_reg32[i]))
			{
				ai->b[0] = base_byte + i;
				return 1;
			}
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg16); i++)
	{
		if (_nmd_strcmp(ai->s, _nmd_reg16[i]))
		{
			ai->b[0] = 0x66;
			ai->b[1] = base_byte + i;
			return 2;
		}
	}

	return 0;
}

NMD_ASSEMBLY_API uint8_t _nmd_encode_segment_reg(NMD_X86_REG segment_reg)
{
	switch (segment_reg)
	{
	case NMD_X86_REG_ES: return 0x26;
	case NMD_X86_REG_CS: return 0x2e;
	case NMD_X86_REG_SS: return 0x36;
	case NMD_X86_REG_DS: return 0x3e;
	case NMD_X86_REG_FS: return 0x64;
	case NMD_X86_REG_GS: return 0x65;
	default: return 0;
	}
}

NMD_ASSEMBLY_API size_t _nmd_parse_number(const char* string, int64_t* p_num)
{
	if (*string == '\0')
		return 0;

	/* Assume decimal base. */
	uint8_t base = _NMD_NUMBER_BASE_DECIMAL;
	size_t i;
	const char* s = string;
	bool is_negative = false;
	bool force_positive = false;
	bool h_suffix = false;
	bool assume_hex = false;

	if (s[0] == '-')
	{
		is_negative = true;
		s++;
	}
	else if (s[0] == '+')
	{
		force_positive = true;
		s++;
	}

	if (s[0] == '0')
	{
		if (s[1] == 'x')
		{
			s += 2;
			base = _NMD_NUMBER_BASE_HEXADECIMAL;
		}
		else if (s[1] == 'b')
		{
			s += 2;
			base = _NMD_NUMBER_BASE_BINARY;
		}
	}

	for (i = 0; s[i]; i++)
	{
		const char c = s[i];

		if (base == _NMD_NUMBER_BASE_DECIMAL)
		{
			if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			{
				base = _NMD_NUMBER_BASE_HEXADECIMAL;
				assume_hex = true;
				continue;
			}
			else if (!(c >= '0' && c <= '9'))
				break;
		}
		else if (base == _NMD_NUMBER_BASE_HEXADECIMAL)
		{
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
				break;
		}
		else if (c != '0' && c != '1') /* _NMD_NUMBER_BASE_BINARY */
			break;
	}

	if (s[i] == 'h')
	{
		base = _NMD_NUMBER_BASE_HEXADECIMAL;
		h_suffix = true;
	}

	const size_t num_digits = i;
	if (num_digits == 0)
		return 0;

	int64_t num = 0;
	for (i = 0; i < num_digits; i++)
	{
		const char c = s[i];
		int n = c % 16;
		if (c >= 'A')
			n += 9;

		num += n;
		if (i < num_digits - 1)
		{
			/* Return false if number is greater than 2^64-1 */
			if (num_digits > 16 && i >= 15)
			{
				if ((base == _NMD_NUMBER_BASE_DECIMAL && (uint64_t)num >= (uint64_t)1844674407370955162) || /* ceiling((2^64-1) / 10) */
					(base == _NMD_NUMBER_BASE_HEXADECIMAL && (uint64_t)num >= (uint64_t)1152921504606846976) || /* *ceiling((2^64-1) / 16) */
					(base == _NMD_NUMBER_BASE_BINARY && (uint64_t)num >= (uint64_t)9223372036854775808U)) /* ceiling((2^64-1) / 2) */
				{
					return 0;
				}

			}
			num *= base;
		}
	}

	if (is_negative)
		num *= -1;

	*p_num = num;

	size_t offset = 0;

	if (is_negative || force_positive)
		offset += 1;

	if (h_suffix)
		offset += 1;
	else if ((base == _NMD_NUMBER_BASE_HEXADECIMAL && !assume_hex) || base == _NMD_NUMBER_BASE_BINARY) /* 0x / 0b*/
		offset += 2;

	return offset + num_digits;
}

NMD_ASSEMBLY_API size_t _nmd_append_prefix_by_reg_size(uint8_t* b, const char* s, size_t* num_prefixes, size_t* index)
{
	size_t i;

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg32); i++)
	{
		if (_nmd_strcmp(s, _nmd_reg32[i]))
		{
			*num_prefixes = 0;
			*index = i;
			return 4;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg8); i++)
	{
		if (_nmd_strcmp(s, _nmd_reg8[i]))
		{
			*num_prefixes = 0;
			*index = i;
			return 1;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg16); i++)
	{
		if (_nmd_strcmp(s, _nmd_reg16[i]))
		{
			b[0] = 0x66;
			*num_prefixes = 1;
			*index = i;
			return 2;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg64); i++)
	{
		if (_nmd_strcmp(s, _nmd_reg64[i]))
		{
			b[0] = 0x48;
			*num_prefixes = 1;
			*index = i;
			return 8;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrx); i++)
	{
		if (_nmd_strcmp(s, _nmd_regrx[i]))
		{
			b[0] = 0x49;
			*num_prefixes = 1;
			*index = i;
			return 8;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxd); i++)
	{
		if (_nmd_strcmp(s, _nmd_regrxd[i]))
		{
			b[0] = 0x41;
			*num_prefixes = 1;
			*index = i;
			return 4;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxw); i++)
	{
		if (_nmd_strcmp(s, _nmd_regrxw[i]))
		{
			b[0] = 0x66;
			b[1] = 0x41;
			*num_prefixes = 2;
			*index = i;
			return 2;
		}
	}

	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxb); i++)
	{
		if (_nmd_strcmp(s, _nmd_regrxb[i]))
		{
			b[0] = 0x41;
			*num_prefixes = 1;
			*index = i;
			return 1;
		}
	}

	return 0;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_reg8(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg8); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_reg8[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_AL + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_reg16(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg16); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_reg16[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_AX + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_reg32(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg32); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_reg32[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_EAX + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_reg64(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg64); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_reg64[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_RAX + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_regrxb(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxb); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_regrxb[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_R8B + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_regrxw(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxw); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_regrxw[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_R8W + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_regrxd(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrxd); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_regrxd[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_R8D + i);
	}
	return NMD_X86_REG_NONE;
}

NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_regrx(const char** string)
{
	const char* s = *string;
	size_t i;
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_regrx); i++)
	{
		if (_nmd_strstr_ex(s, _nmd_regrx[i], string) == s)
			return (NMD_X86_REG)(NMD_X86_REG_R8 + i);
	}
	return NMD_X86_REG_NONE;
}

/* Parses a register */
NMD_ASSEMBLY_API NMD_X86_REG _nmd_parse_reg(const char** string)
{
	NMD_X86_REG reg;
	if (!(reg = _nmd_parse_reg8(string)) && !(reg = _nmd_parse_reg16(string)) && !(reg = _nmd_parse_reg32(string)) && !(reg = _nmd_parse_reg64(string)) &&
		!(reg = _nmd_parse_regrxb(string)) && !(reg = _nmd_parse_regrxw(string)) && !(reg = _nmd_parse_regrxd(string)) && !(reg = _nmd_parse_regrx(string)))
	{
	}

	return reg;
}

/*
Parses a memory operand in the format: '[exp]'
string: a pointer to the string that represents the memory operand. The string is modified to point to the character after the memory operand.
operand[out]: Describes the memory operand.
size[out]: The size of the pointer. byte ptr:1, dword ptr:4, etc.. or zero if unknown.
Return value: True if success, false otherwise.
*/
NMD_ASSEMBLY_API bool _nmd_parse_memory_operand(const char** string, nmd_x86_memory_operand* operand, size_t* size)
{
	/* Check for pointer size */
	const char* s = *string;
	size_t num_bytes;
	if (_nmd_strstr(s, "byte") == s)
		num_bytes = 1;
	else if (_nmd_strstr(s, "word") == s)
		num_bytes = 2;
	else if (_nmd_strstr(s, "dword") == s)
		num_bytes = 4;
	else if (_nmd_strstr(s, "qword") == s)
		num_bytes = 8;
	else
		num_bytes = 0;
	*size = num_bytes;

	/* Check for the "ptr" keyword. It should only be present if a pointer size was specified */
	if (num_bytes > 0)
	{
		s += num_bytes >= 4 ? 5 : 4;

		/* " ptr" */
		if (s[0] == ' ' && s[1] == 'p' && s[2] == 't' && s[3] == 'r')
			s += 4;

		if (s[0] == ' ')
			s++;
		else if (s[0] != '[')
			return false;
	}

	/* Check for a segment register */
	size_t i = 0;
	operand->segment = NMD_X86_REG_NONE;
	for (; i < _NMD_NUM_ELEMENTS(_nmd_segment_reg); i++)
	{
		if (_nmd_strstr(s, _nmd_segment_reg[i]) == s)
		{
			if (s[2] != ':')
				return false;

			s += 3;
			operand->segment = (uint8_t)(NMD_X86_REG_ES + i);
			break;
		}
	}

	/* Check for the actual memory operand expression. If this check fails, this is not a memory operand */
	if (s[0] == '[')
		s++;
	else
		return false;

	/*
	Parse the memory operand expression.
	Even though formally there's no support for subtraction, if the expression has such operation between
	two numeric operands, it'll be resolved to a single number(the same applies for the other operations).
	*/
	operand->base = 0;
	operand->index = 0;
	operand->scale = 0;
	operand->disp = 0;
	bool add = false;
	bool sub = false;
	bool multiply = false;
	bool is_register = false;
	while (true)
	{
		/*
		Check for the base/index register. The previous math operation must not be subtration
		nor multiplication because these are not valid for registers(only addition is).
		*/
		bool parsed_element = false;
		if (!sub && !multiply)
		{
			for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_reg32); i++)
			{
				const char* tmp;
				if (_nmd_strstr_ex(s, _nmd_reg32[i], &tmp) == s)
				{
					s = tmp;
					if (add)
					{
						operand->index = (uint8_t)(NMD_X86_REG_EAX + i);
						operand->scale = 1;
						add = false;
					}
					else
						operand->base = (uint8_t)(NMD_X86_REG_EAX + i);
					parsed_element = true;
					is_register = true;
					break;
				}
			}
		}

		int64_t num;
		size_t num_digits;
		if (!parsed_element && (num_digits = _nmd_parse_number(s, &num)))
		{
			s += num_digits;

			if (add)
			{
				operand->disp += num;
				add = false;
			}
			else if (sub)
			{
				operand->disp -= num;
				sub = false;
			}
			else if (multiply)
			{
				if (!is_register || (num != 1 && num != 2 && num != 4 && num != 8))
					return false;

				operand->scale = (uint8_t)num;
			}
			else
				operand->disp = num;

			parsed_element = true;
		}

		if (!parsed_element)
			return false;

		if (s[0] == '+')
			add = true;
		else if (s[0] == '-')
			sub = true;
		else if (s[0] == '*')
		{
			/* There cannot be more than one '*' operator. */
			if (multiply)
				return false;

			multiply = true;
		}
		else if (s[0] == ']')
			break;
		else
			return false;

		s++;
	}

	*string = s + 1;
	return true;
}

NMD_ASSEMBLY_API size_t _nmd_assemble_mem_reg(uint8_t* buffer, nmd_x86_memory_operand* mem, uint8_t opcode, uint8_t modrm_reg)
{
	size_t offset = 0;

	/* Assemble segment register if required */
	if (mem->segment && mem->segment != ((mem->base == NMD_X86_REG_ESP || mem->index == NMD_X86_REG_ESP) ? NMD_X86_REG_SS : NMD_X86_REG_DS))
		buffer[offset++] = _nmd_encode_segment_reg((NMD_X86_REG)mem->segment);

	buffer[offset++] = opcode;

	nmd_x86_modrm modrm;
	modrm.fields.reg = modrm_reg;
	modrm.fields.mod = 0;

	if (mem->index != NMD_X86_REG_NONE && mem->base != NMD_X86_REG_NONE)
	{
		modrm.fields.rm = 0b100;
		nmd_x86_sib sib;
		sib.fields.scale = (uint8_t)_nmd_get_bit_index(mem->scale);
		sib.fields.base = mem->base - NMD_X86_REG_EAX;
		sib.fields.index = mem->index - NMD_X86_REG_EAX;

		const size_t next_offset = offset;
		if (mem->disp != 0)
		{
			if (mem->disp >= -128 && mem->disp <= 127)
			{
				modrm.fields.mod = 1;
				*(int8_t*)(buffer + offset + 2) = (int8_t)mem->disp;
				offset++;
			}
			else
			{
				modrm.fields.mod = 2;
				*(int32_t*)(buffer + offset + 2) = (int32_t)mem->disp;
				offset += 4;
			}
		}

		buffer[next_offset] = modrm.modrm;
		buffer[next_offset + 1] = sib.sib;
		offset += 2;

		return offset;
	}
	else if (mem->base != NMD_X86_REG_NONE)
	{
		modrm.fields.rm = mem->base - NMD_X86_REG_EAX;
		const size_t next_offset = offset;
		if (mem->disp != 0)
		{
			if (mem->disp >= -128 && mem->disp <= 127)
			{
				modrm.fields.mod = 1;
				*(int8_t*)(buffer + offset + 1) = (int8_t)mem->disp;
				offset++;
			}
			else
			{
				modrm.fields.mod = 2;
				*(int32_t*)(buffer + offset + 1) = (int32_t)mem->disp;
				offset += 4;
			}
		}
		buffer[next_offset] = modrm.modrm;
		offset++;
	}
	else
	{
		modrm.fields.rm = 0b101;
		buffer[offset++] = modrm.modrm;
		*(int32_t*)(buffer + offset) = (int32_t)mem->disp;
		offset += 4;
	}

	return offset;
}

NMD_ASSEMBLY_API size_t _nmd_assemble_single(_nmd_assemble_info* ai)
{
	const char* s;
	int64_t num;
	size_t num_digits;
	NMD_X86_REG reg, reg2;
	size_t i = 0;

	/* Parse prefixes */
	bool lock_prefix = false, repeat_prefix = false, repeat_zero_prefix = false, repeat_not_zero_prefix = false;
	if (_nmd_strstr(ai->s, "lock ") == ai->s)
		lock_prefix = true, ai->s += 5;
	else if (_nmd_strstr(ai->s, "rep ") == ai->s)
		repeat_prefix = true, ai->s += 4;
	else if (_nmd_strstr(ai->s, "repe ") == ai->s || _nmd_strstr(ai->s, "repz ") == ai->s)
		repeat_zero_prefix = true, ai->s += 5;
	else if (_nmd_strstr(ai->s, "repne ") == ai->s || _nmd_strstr(ai->s, "repnz ") == ai->s)
		repeat_not_zero_prefix = true, ai->s += 6;

	if (_nmd_strstr(ai->s, "xacquire ") == ai->s)
	{

	}
	else if (_nmd_strstr(ai->s, "xrelease ") == ai->s)
	{

	}

	/* Parse opcodes */
	if (ai->mode == NMD_X86_MODE_64) /* Only x86-64. */
	{
		if (_nmd_strstr(ai->s, "mov "))
		{
			const char* s = ai->s + 4;
			if ((reg = _nmd_parse_regrxb((const char**)&s)))
			{
				ai->b[0] = 0x41;
				ai->b[1] = 0xb0 + (reg - NMD_X86_REG_R8B);

				if (*s++ != ',')
					return 0;

				if ((num_digits = _nmd_parse_number(s, &num)))
				{
					ai->b[2] = (uint8_t)num;
					return 3;
				}
			}
		}
		else if (_nmd_strstr(ai->s, "push ") == ai->s || _nmd_strstr(ai->s, "pop ") == ai->s)
		{
			const bool is_push = ai->s[1] == 'u';
			s = ai->s + (is_push ? 5 : 4);
			if (((reg = _nmd_parse_reg64(&s))) && !*s)
			{
				ai->b[0] = (is_push ? 0x50 : 0x58) + reg % 8;
				return 1;
			}
			else if ((reg = _nmd_parse_regrxw(&s)) && !*s)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x41;
				ai->b[2] = (is_push ? 0x50 : 0x58) + reg % 8;
				return 3;
			}
			else if (((reg = _nmd_parse_regrx(&s))) && !*s)
			{
				ai->b[0] = 0x41;
				ai->b[1] = (is_push ? 0x50 : 0x58) + reg % 8;
				return 2;
			}
		}
		else if (_nmd_strstr(ai->s, "mov ") == ai->s)
		{
			ai->s += 4;
			if ((reg = _nmd_parse_reg8((const char**)&ai->s)))
			{
				ai->b[0] = 0xb0 + reg % 8;

				if (*ai->s++ != ',')
					return 0;

				if ((num_digits = _nmd_parse_number(ai->s, &num)))
				{
					ai->b[1] = (uint8_t)num;
					return 2;
				}
			}
		}
		else if (_nmd_strcmp(ai->s, "xchg r8,rax") || _nmd_strcmp(ai->s, "xchg rax,r8"))
		{
			ai->b[0] = 0x49;
			ai->b[1] = 0x90;
			return 2;
		}
		else if (_nmd_strcmp(ai->s, "xchg r8d,eax") || _nmd_strcmp(ai->s, "xchg eax,r8d"))
		{
			ai->b[0] = 0x41;
			ai->b[1] = 0x90;
			return 2;
		}
		else if (_nmd_strcmp(ai->s, "pushfq"))
		{
			ai->b[0] = 0x9c;
			return 1;
		}
		else if (_nmd_strcmp(ai->s, "popfq"))
		{
			ai->b[0] = 0x9d;
			return 1;
		}
		else if (_nmd_strcmp(ai->s, "iretq"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0xcf;
			return 2;
		}
		else if (_nmd_strcmp(ai->s, "cdqe"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0x98;
			return 2;
		}
		else if (_nmd_strcmp(ai->s, "cqo"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0x99;
			return 2;
		}
	}
	else /* x86-16 / x86-32 */
	{
		if (_nmd_strstr(ai->s, "inc ") || _nmd_strstr(ai->s, "dec "))
		{
			const bool is_inc = ai->s[0] == 'i';
			s = ai->s + 4;
			int offset = 0;
			if (((reg = _nmd_parse_reg32(&s))) && !*s)
			{
				if (ai->mode == NMD_X86_MODE_16)
					ai->b[offset++] = 0x66;
			}
			else if (((reg = _nmd_parse_reg16(&s))) && !*s)
			{
				if (ai->mode == NMD_X86_MODE_32)
					ai->b[offset++] = 0x66;
			}
			else
				return 0;

			ai->b[offset++] = (is_inc ? 0x40 : 0x48) + reg % 8;
			return offset;
		}
		else if (_nmd_strstr(ai->s, "push ") == ai->s || _nmd_strstr(ai->s, "pop ") == ai->s)
		{
			const bool is_push = ai->s[1] == 'u';
			s = ai->s + (is_push ? 5 : 4);
			if (((reg = _nmd_parse_reg32(&s))) && !*s)
			{
				if (ai->mode == NMD_X86_MODE_16)
				{
					ai->b[0] = 0x66;
					ai->b[1] = (is_push ? 0x50 : 0x58) + reg % 8;
					return 2;
				}
				else
				{
					ai->b[0] = (is_push ? 0x50 : 0x58) + reg % 8;
					return 1;
				}
			}
		}
		else if (_nmd_strcmp(ai->s, "pushad"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x60;
				return 2;
			}
			else
			{
				ai->b[0] = 0x60;
				return 1;
			}
		}
		else if (_nmd_strcmp(ai->s, "pusha"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x60;
				return 1;
			}
			else
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x60;
				return 2;
			}
		}
		else if (_nmd_strcmp(ai->s, "popad"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x61;
				return 2;
			}
			else
			{
				ai->b[0] = 0x61;
				return 1;
			}
		}
		else if (_nmd_strcmp(ai->s, "popa"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x61;
				return 1;
			}
			else
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x61;
				return 2;
			}
		}
		else if (_nmd_strcmp(ai->s, "pushfd"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x9c;
				return 2;
			}
			else
			{
				ai->b[0] = 0x9c;
				return 1;
			}
		}
		else if (_nmd_strcmp(ai->s, "popfd"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x9d;
				return 2;
			}
			else
			{
				ai->b[0] = 0x9d;
				return 1;
			}
		}
	}

	typedef struct _nmd_string_byte_pair { const char* s; uint8_t b; } _nmd_string_byte_pair;

	const _nmd_string_byte_pair single_byte_op1[] = {
		{ "int3",    0xcc },
		{ "nop",     0x90 },
		{ "ret",     0xc3 },
		{ "retf",    0xcb },
		{ "ret far", 0xcb },
		{ "leave",   0xc9 },
		{ "int1",    0xf1 },
		{ "push es", 0x06 },
		{ "push ss", 0x16 },
		{ "push ds", 0x1e },
		{ "push cs", 0x0e },
		{ "pop es",  0x07 },
		{ "pop ss",  0x17 },
		{ "pop ds",  0x1f },
		{ "daa",     0x27 },
		{ "aaa",     0x37 },
		{ "das",     0x2f },
		{ "aas",     0x3f },
		{ "xlat",    0xd7 },
		{ "fwait",   0x9b },
		{ "hlt",     0xf4 },
		{ "cmc",     0xf5 },
		{ "clc",     0xf8 },
		{ "sahf",    0x9e },
		{ "lahf",    0x9f },
		{ "into",    0xce },
		{ "salc",    0xd6 },
		{ "slc",     0xf8 },
		{ "stc",     0xf9 },
		{ "cli",     0xfa },
		{ "sti",     0xfb },
		{ "cld",     0xfc },
		{ "std",     0xfd },
	};
	for (i = 0; i < _NMD_NUM_ELEMENTS(single_byte_op1); i++)
	{
		if (_nmd_strcmp(ai->s, single_byte_op1[i].s))
		{
			ai->b[0] = single_byte_op1[i].b;
			return 1;
		}
	}

	const _nmd_string_byte_pair single_byte_op2[] = {
		{ "syscall",  0x05 },
		{ "clts",     0x06 },
		{ "sysret",   0x07 },
		{ "invd",     0x08 },
		{ "wbinvd",   0x09 },
		{ "ud2",      0x0b },
		{ "femms",    0x0e },
		{ "wrmsr",    0x30 },
		{ "rdtsc",    0x31 },
		{ "rdmsr",    0x32 },
		{ "rdpmc",    0x33 },
		{ "sysenter", 0x34 },
		{ "sysexit",  0x35 },
		{ "getsec",   0x37 },
		{ "emms",     0x77 },
		{ "push fs",  0xa0 },
		{ "pop fs",   0xa1 },
		{ "cpuid",    0xa2 },
		{ "push gs",  0xa8 },
		{ "pop gs",   0xa9 },
		{ "rsm",      0xaa }
	};
	for (i = 0; i < _NMD_NUM_ELEMENTS(single_byte_op2); i++)
	{
		if (_nmd_strcmp(ai->s, single_byte_op2[i].s))
		{
			ai->b[0] = 0x0f;
			ai->b[1] = single_byte_op2[i].b;
			return 2;
		}
	}

	/* Parse 'add', 'adc', 'and', 'xor', 'or', 'sbb', 'sub' and 'cmp' . Opcodes in first "4 rows"/[80, 83] */
	for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_op1_opcode_map_mnemonics); i++)
	{
		if (_nmd_strstr(ai->s, _nmd_op1_opcode_map_mnemonics[i]) == ai->s)
		{
			const uint8_t base_opcode = (i % 4) * 0x10 + (i >= 4 ? 8 : 0);
			ai->s += 4;

			nmd_x86_memory_operand memory_operand;
			size_t pointer_size;
			if (_nmd_parse_memory_operand((const char**)&ai->s, &memory_operand, &pointer_size)) /* Colum 00,01,08,09 */
			{
				if (*ai->s++ != ',' || !(reg = _nmd_parse_reg((const char**)&ai->s)))
					return 0;

				return _nmd_assemble_mem_reg(ai->b, &memory_operand, base_opcode, reg % 8);

				/*
				size_t offset = 0;
				if (memory_operand.segment && memory_operand.segment != ((memory_operand.base == NMD_X86_REG_ESP || memory_operand.index == NMD_X86_REG_ESP) ? NMD_X86_REG_SS : NMD_X86_REG_DS))
					ai->b[offset++] = _nmd_encode_segment_reg((NMD_X86_REG)memory_operand.segment);

				if (pointer_size == 1)
				{
					ai->b[offset++] = base_opcode + 0;

					if (*ai->s++ != ',')
						return 0;

					NMD_X86_REG reg = _nmd_parse_reg(&ai->s);

					nmd_x86_modrm modrm;
					modrm.fields.mod = 0;
					modrm.fields.reg = (reg - 1) % 8;
					modrm.fields.rm = (memory_operand.base - 1) % 8;

					ai->b[offset++] = modrm.modrm;

					return offset;
				}
				*/
			}
			else if (_nmd_strstr_ex(ai->s, "al,", &s) == ai->s && (num_digits = _nmd_parse_number(s, &num)) && *(s + num_digits) == '\0') /* column 04,0C */
			{
				if (num < -0x80 || num > 0xff)
					return 0;

				ai->b[0] = base_opcode + 4;
				ai->b[1] = (int8_t)num;
				return 2;
			}
			else if ((_nmd_strstr_ex(ai->s, "eax,", &s) == ai->s || _nmd_strstr_ex(ai->s, "ax,", &s) == ai->s || _nmd_strstr_ex(ai->s, "rax,", &s) == ai->s) && _nmd_parse_number(s, &num)) /* column 05,0D */
			{
				const bool is_eax = ai->s[0] == 'e';
				const bool is_ax = ai->s[1] == 'x';

				if (is_eax)
				{
					if (num < -(int64_t)0x80000000 || num > 0xffffffff)
						return 0;

					ai->b[0] = base_opcode + 5;
					*(int32_t*)(ai->b + 1) = (int32_t)num;
					return 5;
				}
				else if (!is_eax)
				{
					if (ai->mode != NMD_X86_MODE_64 && !is_ax)
						return 0;

					ai->b[0] = is_ax ? 0x66 : 0x48;
					ai->b[1] = base_opcode + 5;

					if (is_ax)
					{
						if (num < -0x8000 || num > 0xffff)
							return 0;

						*(int16_t*)(ai->b + 2) = (int16_t)num;
						return 4;
					}
					else
					{
						if (num < -(int64_t)0x80000000 || num > 0xffffffff)
							return 0;

						*(int32_t*)(ai->b + 2) = (int32_t)num;
						return 6;
					}
				}
			}
			else if ((reg = _nmd_parse_reg((const char**)&ai->s)) && *ai->s++ == ',') /* column 00-04,08-0B */
			{
				if (_nmd_parse_memory_operand((const char**)&ai->s, &memory_operand, &pointer_size)) /* column 02,03,0A,0B */
				{
					return 0;
				}
				else /* 00,01,08,09 */
				{
					if (!(reg2 = _nmd_parse_reg((const char**)&ai->s)))
						return 0;

					ai->b[0] = base_opcode;

					/* mod = 0b11, reg = reg2, rm = reg */
					ai->b[1] = 0b11000000 | ((reg2 % 8) << 3) | (reg % 8);

					return 2;
				}
			}

			return 0;
		}
	}

	if (_nmd_strstr(ai->s, "jmp ") == ai->s)
	{
		if (!(num_digits = _nmd_parse_number(ai->s + 4, &num)) && ai->s[4 + num_digits] == '\0')
			return 0;

		size_t offset = 0;
		if (ai->mode == NMD_X86_MODE_16)
			ai->b[offset++] = 0x66;
		ai->b[offset++] = 0xe9;
		const int64_t size = (int64_t)offset + 4;
		*(uint32_t*)(ai->b + offset) = (uint32_t)((ai->runtime_address == NMD_X86_INVALID_RUNTIME_ADDRESS) ? (num - size) : (ai->runtime_address + size + num));
		return size;
	}
	else if (ai->s[0] == 'j')
	{
		char* s = ai->s;
		while (true)
		{
			if (*s == ' ')
			{
				*s = '\0';
				break;
			}
			else if (*s == '\0')
				return 0;

			s++;
		}

		for (i = 0; i < _NMD_NUM_ELEMENTS(_nmd_condition_suffixes); i++)
		{
			if (_nmd_strcmp(ai->s + 1, _nmd_condition_suffixes[i]))
			{
				if (!(num_digits = _nmd_parse_number(s + 1, &num)))
					return 0;

				const int64_t delta = (ai->runtime_address == NMD_X86_INVALID_RUNTIME_ADDRESS ? num : num - ai->runtime_address);
				if (delta >= -(1 << 7) + 2 && delta <= (1 << 7) - 1 + 2)
				{
					ai->b[0] = 0x70 + (uint8_t)i;
					*(int8_t*)(ai->b + 1) = (int8_t)(delta - 2);
					return 2;
				}
				else if (delta >= -((int64_t)1 << 31) + 6 && delta <= ((int64_t)1 << 31) - 1 + 6)
				{
					size_t offset = 0;
					if (ai->mode == NMD_X86_MODE_16)
						ai->b[offset++] = 0x66;

					ai->b[offset++] = 0x0f;
					ai->b[offset++] = 0x80 + (uint8_t)i;
					*(int32_t*)(ai->b + offset) = (int32_t)(delta - (offset + 4));
					return offset + 4;
				}
			}
		}
	}
	else if (_nmd_strstr(ai->s, "inc ") == ai->s || _nmd_strstr(ai->s, "dec ") == ai->s)
	{
		const char* tmp = ai->s + 4;
		nmd_x86_memory_operand memory_operand;
		size_t size;
		if (_nmd_parse_memory_operand(&tmp, &memory_operand, &size))
		{
			size_t offset = 0;
			if (memory_operand.segment && memory_operand.segment != ((memory_operand.base == NMD_X86_REG_ESP || memory_operand.index == NMD_X86_REG_ESP) ? NMD_X86_REG_SS : NMD_X86_REG_DS))
				ai->b[offset++] = _nmd_encode_segment_reg((NMD_X86_REG)memory_operand.segment);

			ai->b[offset++] = size == 1 ? 0xfe : 0xff;

			nmd_x86_modrm modrm;
			modrm.fields.reg = ai->s[0] == 'i' ? 0 : 8;
			modrm.fields.mod = 0;

			if (memory_operand.index != NMD_X86_REG_NONE && memory_operand.base != NMD_X86_REG_NONE)
			{
				modrm.fields.rm = 0b100;
				nmd_x86_sib sib;
				sib.fields.scale = (uint8_t)_nmd_get_bit_index(memory_operand.scale);
				sib.fields.base = memory_operand.base - NMD_X86_REG_EAX;
				sib.fields.index = memory_operand.index - NMD_X86_REG_EAX;

				const size_t next_offset = offset;
				if (memory_operand.disp != 0)
				{
					if (memory_operand.disp >= -128 && memory_operand.disp <= 127)
					{
						modrm.fields.mod = 1;
						*(int8_t*)(ai->b + offset + 2) = (int8_t)memory_operand.disp;
						offset++;
					}
					else
					{
						modrm.fields.mod = 2;
						*(int32_t*)(ai->b + offset + 2) = (int32_t)memory_operand.disp;
						offset += 4;
					}
				}

				ai->b[next_offset] = modrm.modrm;
				ai->b[next_offset + 1] = sib.sib;
				offset += 2;

				return offset;
			}
			else if (memory_operand.base != NMD_X86_REG_NONE)
			{
				modrm.fields.rm = memory_operand.base - NMD_X86_REG_EAX;
				const size_t next_offset = offset;
				if (memory_operand.disp != 0)
				{
					if (memory_operand.disp >= -128 && memory_operand.disp <= 127)
					{
						modrm.fields.mod = 1;
						*(int8_t*)(ai->b + offset + 1) = (int8_t)memory_operand.disp;
						offset++;
					}
					else
					{
						modrm.fields.mod = 2;
						*(int32_t*)(ai->b + offset + 1) = (int32_t)memory_operand.disp;
						offset += 4;
					}
				}
				ai->b[next_offset] = modrm.modrm;
				offset++;
			}
			else
			{
				modrm.fields.rm = 0b101;
				ai->b[offset++] = modrm.modrm;
				*(int32_t*)(ai->b + offset) = (int32_t)memory_operand.disp;
				offset += 4;
			}

			return offset;
		}

		size_t num_prefixes, index;
		size = _nmd_append_prefix_by_reg_size(ai->b, ai->s + 4, &num_prefixes, &index);
		if (size > 0)
		{
			if (ai->mode == NMD_X86_MODE_64)
			{
				ai->b[num_prefixes + 0] = size == 1 ? 0xfe : 0xff;
				ai->b[num_prefixes + 1] = 0xc0 + (ai->s[0] == 'i' ? 0 : 8) + (uint8_t)index;
				return num_prefixes + 2;
			}
			else
			{
				if (size == 1)
				{
					ai->b[0] = 0xfe;
					ai->b[1] = 0xc0 + (ai->s[0] == 'i' ? 0 : 8) + (uint8_t)index;
					return 2;
				}
				else
				{
					ai->b[num_prefixes + 0] = (ai->s[0] == 'i' ? 0x40 : 0x48) + (uint8_t)index;
					return num_prefixes + 1;
				}
			}
		}
	}
	else if (_nmd_strstr(ai->s, "call ") == ai->s)
	{
		if ((num_digits = _nmd_parse_number(ai->s + 5, &num)))
		{
			ai->b[0] = 0xe8;
			if (ai->runtime_address == NMD_X86_INVALID_RUNTIME_ADDRESS)
				*(int32_t*)(ai->b + 1) = (int32_t)(num - 5);
			else
				*(int32_t*)(ai->b + 1) = (int32_t)(num - (ai->runtime_address + 5));
			return 5;
		}
	}
	else if (_nmd_strstr(ai->s, "push ") == ai->s || _nmd_strstr(ai->s, "pop ") == ai->s)
	{
		const bool is_push = ai->s[1] == 'u';
		ai->s += is_push ? 5 : 4;
		NMD_X86_REG reg;
		if ((reg = _nmd_parse_reg16((const char**)&ai->s)))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = (is_push ? 0x50 : 0x58) + reg % 8;
				return 1;
			}
			else
			{
				ai->b[0] = 0x66;
				ai->b[1] = (is_push ? 0x50 : 0x58) + reg % 8;
				return 2;
			}
		}
		else if (is_push && (num_digits = _nmd_parse_number(ai->s, &num)) && ai->s[num_digits] == '\0')
		{
			if (num >= -(1 << 7) && num <= (1 << 7) - 1)
			{
				ai->b[0] = 0x6a;
				*(int8_t*)(ai->b + 1) = (int8_t)num;
				return 2;
			}
			else
			{
				size_t offset = 0;
				if (ai->mode == NMD_X86_MODE_16)
					ai->b[offset++] = 0x66;
				ai->b[offset++] = 0x68;
				*(int32_t*)(ai->b + offset) = (int32_t)num;
				return offset + 4;
			}
		}
	}
	else if (_nmd_strstr(ai->s, "mov ") == ai->s)
	{
		ai->s += 4;
		const NMD_X86_REG reg = _nmd_parse_reg8((const char**)&ai->s);
		if (reg)
		{
			ai->b[0] = 0xb0 + (reg - NMD_X86_REG_AL);

			if (*ai->s++ != ',')
				return 0;

			if ((num_digits = _nmd_parse_number(ai->s, &num)))
			{
				ai->b[1] = (uint8_t)num;
				return 2;
			}
		}
	}
	else if (_nmd_strstr(ai->s, "ret ") == ai->s || _nmd_strstr(ai->s, "retf ") == ai->s)
	{
		const bool is_far = ai->s[3] == 'f';
		ai->s += is_far ? 5 : 4;

		if ((num_digits = _nmd_parse_number(ai->s, &num)) && ai->s[num_digits] == '\0')
		{
			ai->b[0] = is_far ? 0xca : 0xc2;
			*(uint16_t*)(ai->b + 1) = (uint16_t)num;
			return 3;
		}
	}
	else if (_nmd_strstr(ai->s, "emit ") == ai->s)
	{
		size_t offset = 5;
		while ((num_digits = _nmd_parse_number(ai->s + offset, &num)))
		{
			if (num < 0 || num > 0xff)
				return 0;

			ai->b[i++] = (uint8_t)num;

			offset += num_digits;
			if (ai->s[offset] == ' ')
				offset++;
		}
		return i;
	}
	else if (_nmd_strcmp(ai->s, "pushf"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x9c;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x9c;
			return 2;
		}
	}
	else if (_nmd_strcmp(ai->s, "popf"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x9d;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x9d;
			return 2;
		}
	}
	else if (_nmd_strcmp(ai->s, "pause"))
	{
		ai->b[0] = 0xf3;
		ai->b[1] = 0x90;
		return 2;
	}
	else if (_nmd_strcmp(ai->s, "iret"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0xcf;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0xcf;
			return 2;
		}
	}
	else if (_nmd_strcmp(ai->s, "iretd"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0xcf;
			return 2;
		}
		else
		{
			ai->b[0] = 0xcf;
			return 1;
		}
	}
	else if (_nmd_strcmp(ai->s, "pushf"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x9c;
		return 2;
	}
	else if (_nmd_strcmp(ai->s, "popf"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x9d;
		return 2;
	}
	else if (_nmd_strcmp(ai->s, "cwde"))
	{
		int offset = 0;
		if (ai->mode == NMD_X86_MODE_16)
			ai->b[offset++] = 0x66;
		ai->b[offset++] = 0x98;
		return offset;
	}
	else if (_nmd_strstr(ai->s, "int ") == ai->s)
	{
		ai->s += 4;
		if ((num_digits = _nmd_parse_number(ai->s, &num)) && ai->s[num_digits] == '\0')
		{
			ai->b[0] = 0xcd;
			ai->b[1] = (uint8_t)num;
			return 2;
		}
	}
	else if (_nmd_strcmp(ai->s, "cbw"))
	{
		int offset = 0;
		if (ai->mode != NMD_X86_MODE_16)
			ai->b[offset++] = 0x66;
		ai->b[offset++] = 0x98;
		return offset;
	}
	else if (_nmd_strcmp(ai->s, "cdq"))
	{
		int offset = 0;
		if (ai->mode == NMD_X86_MODE_16)
			ai->b[offset++] = 0x66;
		ai->b[offset++] = 0x99;
		return offset;
	}
	else if (_nmd_strcmp(ai->s, "cwd"))
	{
		int offset = 0;
		if (ai->mode != NMD_X86_MODE_16)
			ai->b[offset++] = 0x66;
		ai->b[offset++] = 0x99;
		return offset;
	}

	return 0;
}

/*
Assembles one or more instructions from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using the '\n'(new line) character.
Parameters:
 - string          [in]         A pointer to a string that represents one or more instructions in assembly language.
 - buffer          [out]        A pointer to a buffer that receives the encoded instructions.
 - buffer_size     [in]         The size of the buffer in bytes.
 - runtime_address [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
 - mode            [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
 - count           [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed, and on output the number of instructions parsed. This parameter may be null.
*/
NMD_ASSEMBLY_API size_t nmd_x86_assemble(const char* string, void* buffer, size_t buffer_size, uint64_t runtime_address, NMD_X86_MODE mode, size_t* count)
{
	if (!*string)
		return 0;

	const uint8_t* const buffer_end = (uint8_t*)buffer + buffer_size;
	const size_t num_max_instructions = count ? *count : (size_t)(-1);
	size_t num_instructions = 0;
	char parsed_string[256];
	uint8_t temp_buffer[15]; /* The assembling takes place on this buffer instead of the user's buffer because the assembler doesn't check the buffer size. If it assembled directly to the user's buffer it could access bad memory */
	uint8_t* b = (uint8_t*)buffer;
	size_t remaining_size;
	size_t length = 0;

	_nmd_assemble_info ai;
	ai.s = parsed_string;
	ai.mode = mode;
	ai.runtime_address = runtime_address;
	ai.b = temp_buffer;

	/* Parse the first character of the string because the loop just ahead accesses `string-1` which could be bad memory if we didn't do this */
	if (*string == ' ')
		string++;
	else
	{
		parsed_string[0] = *string++;
		length++;
	}

	while (*string && num_instructions < num_max_instructions)
	{
		remaining_size = buffer_end - b;

		/* Copy 'string' to 'parsed_string' converting it to lowercase and removing unwanted spaces. If the instruction separator character '\n' is found, stop. */
		char c = *string; /* Current character */
		char prev_c = *(string - 1); /* Previous character */
		while (c && c != '\n')
		{
			/* Ignore(skip) the current character if it's a space and the previous character is one of the following: ' ', '+', '*', '[' */
			if (c == ' ' && (prev_c == ' ' || prev_c == '+' || prev_c == '*' || prev_c == '['))
			{
				c = *++string;
				continue;
			}

			/* Append character */
			parsed_string[length++] = _NMD_TOLOWER(c);

			/* The maximum length is 255 */
			if (length >= 256)
				return 0;

			/* Set previous character */
			prev_c = c;

			/* Get the next character */
			c = *++string;
		}

		/* This check is only ever true if *string == '\n', that is the instruction separator character */
		if (*string /* == '\n' */)
			string++; /* Go forward by one character so 'string' points to the next instruction */

		/* If the last character is a space, remove it. */
		if (length > 0 && parsed_string[length - 1] == ' ')
			length--;

		/* After all of the string manipulation, place the null character */
		parsed_string[length] = '\0';

		/* Try to assemble the instruction */
		const size_t num_bytes = _nmd_assemble_single(&ai);
		if (num_bytes == 0 || num_bytes > remaining_size)
			return 0;

		/* Copy bytes from 'temp_buffer' to 'buffer' */
		size_t i = 0;
		for (; i < num_bytes; i++)
			b[i] = temp_buffer[i];
		b += num_bytes;

		num_instructions++;

		/* Reset length in case there's another instruction */
		length = 0;
	}

	if (count)
		*count = num_instructions;

	/* Return the number of bytes written to the buffer */
	return (size_t)((ptrdiff_t)b - (ptrdiff_t)buffer);
}


NMD_ASSEMBLY_API void _nmd_decode_operand_segment_reg(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	if (instruction->segment_override)
		operand->fields.reg = (uint8_t)(NMD_X86_REG_ES + _nmd_get_bit_index(instruction->segment_override));
	else
		operand->fields.reg = (uint8_t)(!(instruction->prefixes & NMD_X86_PREFIXES_REX_B) && (instruction->modrm.fields.rm == 0b100 || instruction->modrm.fields.rm == 0b101) ? NMD_X86_REG_SS : NMD_X86_REG_DS);
}

/* Decodes a memory operand. modrm is assumed to be in the range [00,BF] */
NMD_ASSEMBLY_API void _nmd_decode_modrm_upper32(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	/* Set operand type */
	operand->type = NMD_X86_OPERAND_TYPE_MEMORY;

	if (instruction->has_sib) /* R/M is 0b100 */
	{
		if (instruction->sib.fields.base == 0b101) /* Check if there is displacement */
		{
			if (instruction->modrm.fields.mod != 0b00)
				operand->fields.mem.base = (uint8_t)(instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R13 : NMD_X86_REG_RBP) : NMD_X86_REG_EBP);
		}
		else
			operand->fields.mem.base = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->sib.fields.base);

		if (instruction->sib.fields.index != 0b100)
			operand->fields.mem.index = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_X ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->sib.fields.index);

		if (instruction->prefixes & NMD_X86_PREFIXES_REX_X && instruction->sib.fields.index == 0b100)
		{
			operand->fields.mem.index = (uint8_t)NMD_X86_REG_R12;
		}

		operand->fields.mem.scale = instruction->sib.fields.scale;
	}
	else if (!(instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b101))
	{
		if (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE))
		{
			operand->fields.mem.base = NMD_X86_REG_BX;
			operand->fields.mem.index = NMD_X86_REG_SI;
		}
		else
		{
			if ((instruction->prefixes & (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B) && instruction->mode == NMD_X86_MODE_64)
				operand->fields.mem.base = (uint8_t)(NMD_X86_REG_R8D + instruction->modrm.fields.rm);
			else
				operand->fields.mem.base = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->modrm.fields.rm);
		}
	}

	_nmd_decode_operand_segment_reg(instruction, operand);

	operand->fields.mem.disp = instruction->displacement;
}

NMD_ASSEMBLY_API void _nmd_decode_memory_operand(const nmd_x86_instruction* instruction, nmd_x86_operand* operand, uint8_t mod11base_reg)
{
	if (instruction->modrm.fields.mod == 0b11)
	{
		operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
		operand->fields.reg = mod11base_reg + instruction->modrm.fields.rm;
	}
	else
		_nmd_decode_modrm_upper32(instruction, operand);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Eb(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, NMD_X86_REG_AL);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Ew(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, NMD_X86_REG_AX);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Ev(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Ey(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Qq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, NMD_X86_REG_MM0);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Wdq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	_nmd_decode_memory_operand(instruction, operand, NMD_X86_REG_XMM0);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Gb(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_AL + instruction->modrm.fields.reg;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Gd(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_EAX + instruction->modrm.fields.reg;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Gw(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.reg;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Gv(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
		operand->fields.reg = (uint8_t)((!(instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_REG_R8D : NMD_X86_REG_R8) + instruction->modrm.fields.reg);
	else
		operand->fields.reg = (uint8_t)((instruction->rex_w_prefix ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode != NMD_X86_MODE_16 ? NMD_X86_REG_AX : NMD_X86_REG_EAX)) + instruction->modrm.fields.reg);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Rv(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	if (instruction->prefixes & NMD_X86_PREFIXES_REX_R)
		operand->fields.reg = (uint8_t)((!(instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_REG_R8D : NMD_X86_REG_R8) + instruction->modrm.fields.rm);
	else
		operand->fields.reg = (uint8_t)((instruction->rex_w_prefix ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode != NMD_X86_MODE_16) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)) + instruction->modrm.fields.rm);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Gy(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = (uint8_t)((instruction->mode == NMD_X86_MODE_64 ? NMD_X86_REG_RAX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Pq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_MM0 + instruction->modrm.fields.reg;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Nq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_MM0 + instruction->modrm.fields.rm;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Vdq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_XMM0 + instruction->modrm.fields.reg;
}

NMD_ASSEMBLY_API void _nmd_decode_operand_Udq(const nmd_x86_instruction* instruction, nmd_x86_operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_XMM0 + instruction->modrm.fields.rm;
}

NMD_ASSEMBLY_API void _nmd_decode_conditional_flag(nmd_x86_instruction* instruction, const uint8_t condition)
{
	switch (condition)
	{
	case 0x0: instruction->tested_flags.fields.OF = 1; break;                                                                             /* Jump if overflow (OF=1) */
	case 0x1: instruction->tested_flags.fields.OF = 1; break;                                                                             /* Jump if not overflow (OF=0) */
	case 0x2: instruction->tested_flags.fields.CF = 1; break;                                                                             /* Jump if not above or equal (CF=1) */
	case 0x3: instruction->tested_flags.fields.CF = 1; break;                                                                             /* Jump if not below (CF=0) */
	case 0x4: instruction->tested_flags.fields.ZF = 1; break;                                                                             /* Jump if equal (ZF=1) */
	case 0x5: instruction->tested_flags.fields.ZF = 1; break;                                                                             /* Jump if not equal (ZF=0) */
	case 0x6: instruction->tested_flags.fields.CF = instruction->tested_flags.fields.ZF = 1; break;                                       /* Jump if not above (CF=1 or ZF=1) */
	case 0x7: instruction->tested_flags.fields.CF = instruction->tested_flags.fields.ZF = 1; break;                                       /* Jump if not below or equal (CF=0 and ZF=0) */
	case 0x8: instruction->tested_flags.fields.SF = 1; break;                                                                             /* Jump if sign (SF=1) */
	case 0x9: instruction->tested_flags.fields.SF = 1; break;                                                                             /* Jump if not sign (SF=0) */
	case 0xa: instruction->tested_flags.fields.PF = 1; break;                                                                             /* Jump if parity/parity even (PF=1) */
	case 0xb: instruction->tested_flags.fields.PF = 1; break;                                                                             /* Jump if parity odd (PF=0) */
	case 0xc: instruction->tested_flags.fields.SF = instruction->tested_flags.fields.OF = 1; break;                                       /* Jump if not greater or equal (SF != OF) */
	case 0xd: instruction->tested_flags.fields.SF = instruction->tested_flags.fields.OF = 1; break;                                       /* Jump if not less (SF=OF) */
	case 0xe: instruction->tested_flags.fields.ZF = instruction->tested_flags.fields.SF = instruction->tested_flags.fields.OF = 1; break; /* Jump if not greater (ZF=1 or SF != OF) */
	case 0xf: instruction->tested_flags.fields.ZF = instruction->tested_flags.fields.SF = instruction->tested_flags.fields.OF = 1; break; /* Jump if not less or equal (ZF=0 and SF=OF) */
	}
}

NMD_ASSEMBLY_API bool _nmd_decode_modrm(const uint8_t** p_buffer, size_t* p_buffer_size, nmd_x86_instruction* const instruction)
{
	instruction->has_modrm = true;
	_NMD_READ_BYTE(*p_buffer, *p_buffer_size, instruction->modrm.modrm);

	const bool address_prefix = (bool)(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE);

	/* Check for 16-Bit Addressing Form */
	if (instruction->mode == NMD_X86_MODE_16)
	{
		if (instruction->modrm.fields.mod != 0b11)
		{
			if (instruction->modrm.fields.mod == 0b00)
			{
				if (instruction->modrm.fields.rm == 0b110)
					instruction->disp_mask = NMD_X86_DISP16;
			}
			else
				instruction->disp_mask = (uint8_t)(instruction->modrm.fields.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP16);
		}
	}
	else
	{
		/* Check for 16-Bit Addressing Form */
		if (address_prefix && instruction->mode == NMD_X86_MODE_32)
		{
			/* Check for displacement */
			if ((instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b110) || instruction->modrm.fields.mod == 0b10)
				instruction->disp_mask = NMD_X86_DISP16;
			else if (instruction->modrm.fields.mod == 0b01)
				instruction->disp_mask = NMD_X86_DISP8;
		}
		else /*if (!address_prefix || (address_prefix && **b >= 0x40) || (address_prefix && instruction->mode == NMD_X86_MODE_64)) */
		{
			/* Check for SIB byte */
			if (instruction->modrm.modrm < 0xC0 && instruction->modrm.fields.rm == 0b100 && (!address_prefix || (address_prefix && instruction->mode == NMD_X86_MODE_64)))
			{
				instruction->has_sib = true;
				_NMD_READ_BYTE(*p_buffer, *p_buffer_size, instruction->sib.sib);
			}

			/* Check for displacement */
			if (instruction->modrm.fields.mod == 0b01) /* disp8 (ModR/M) */
				instruction->disp_mask = NMD_X86_DISP8;
			else if ((instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b101) || instruction->modrm.fields.mod == 0b10) /* disp16,32 (ModR/M) */
				instruction->disp_mask = (uint8_t)(address_prefix && !(instruction->mode == NMD_X86_MODE_64 && instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? NMD_X86_DISP16 : NMD_X86_DISP32);
			else if (instruction->has_sib && instruction->sib.fields.base == 0b101) /* disp8,32 (SIB) */
				instruction->disp_mask = (uint8_t)(instruction->modrm.fields.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP32);
		}
	}

	/* Make sure we can read 'instruction->disp_mask' bytes from the buffer */
	if (*p_buffer_size < instruction->disp_mask)
		return false;

	/* Copy 'instruction->disp_mask' bytes from the buffer */
	size_t i = 0;
	for (; i < (size_t)instruction->disp_mask; i++)
		((uint8_t*)(&instruction->displacement))[i] = (*p_buffer)[i];

	/* Increment the buffer and decrement the buffer's size */
	*p_buffer += instruction->disp_mask;
	*p_buffer_size -= instruction->disp_mask;

	return true;
}

/*
Decodes an instruction. Returns true if the instruction is valid, false otherwise.
Parameters:
 - buffer      [in]  A pointer to a buffer containing an encoded instruction.
 - buffer_size [in]  The size of the buffer in bytes.
 - instruction [out] A pointer to a variable of type 'nmd_x86_instruction' that receives information about the instruction.
 - mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
 - flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
*/
NMD_ASSEMBLY_API bool nmd_x86_decode(const void* const buffer, size_t buffer_size, nmd_x86_instruction* instruction, NMD_X86_MODE mode, uint32_t flags)
{
	/* Security considerations for memory safety:
	The contents of 'buffer' should be considered untrusted and decoded carefully.

	'buffer' should always point to the start of the buffer. We use the 'b'
	buffer iterator to read data from the buffer, however before accessing it
	make sure to check 'buffer_size' to see if we can safely access it. Then,
	after reading data from the buffer we increment 'b' and decrement 'buffer_size'.
	Helper macros: _NMD_READ_BYTE()
	*/

	/* Clear 'instruction' */
	size_t i = 0;
	for (; i < sizeof(nmd_x86_instruction); i++)
		((uint8_t*)(instruction))[i] = 0x00;

	/* Set mode */
	instruction->mode = (uint8_t)mode;

	/* Set buffer iterator */
	const uint8_t* b = (const uint8_t*)buffer;

	/*  Clamp 'buffer_size' to 15. We will only read up to 15 bytes(NMD_X86_MAXIMUM_INSTRUCTION_LENGTH) */
	if (buffer_size > 15)
		buffer_size = 15;

	/* Decode legacy and REX prefixes */
	for (; buffer_size > 0; b++, buffer_size--)
	{
		switch (*b)
		{
		case 0xF0: instruction->prefixes = (instruction->prefixes | (instruction->simd_prefix = NMD_X86_PREFIXES_LOCK)); continue;
		case 0xF2: instruction->prefixes = (instruction->prefixes | (instruction->simd_prefix = NMD_X86_PREFIXES_REPEAT_NOT_ZERO)), instruction->repeat_prefix = false; continue;
		case 0xF3: instruction->prefixes = (instruction->prefixes | (instruction->simd_prefix = NMD_X86_PREFIXES_REPEAT)), instruction->repeat_prefix = true; continue;
		case 0x2E: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_CS_SEGMENT_OVERRIDE)); continue;
		case 0x36: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_SS_SEGMENT_OVERRIDE)); continue;
		case 0x3E: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_DS_SEGMENT_OVERRIDE)); continue;
		case 0x26: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_ES_SEGMENT_OVERRIDE)); continue;
		case 0x64: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_FS_SEGMENT_OVERRIDE)); continue;
		case 0x65: instruction->prefixes = (instruction->prefixes | (instruction->segment_override = NMD_X86_PREFIXES_GS_SEGMENT_OVERRIDE)); continue;
		case 0x66: instruction->prefixes = (instruction->prefixes | (instruction->simd_prefix = NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)), instruction->rex_w_prefix = false; continue;
		case 0x67: instruction->prefixes = (instruction->prefixes | NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE); continue;
		default:
			if (mode == NMD_X86_MODE_64 && _NMD_R(*b) == 4) /* REX prefixes [0x40,0x4f] */
			{
				instruction->has_rex = true;
				instruction->rex = *b;
				instruction->prefixes = (instruction->prefixes & ~(NMD_X86_PREFIXES_REX_B | NMD_X86_PREFIXES_REX_X | NMD_X86_PREFIXES_REX_R | NMD_X86_PREFIXES_REX_W));

				if (*b & 0b0001) /* Bit position 0 */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_B;
				if (*b & 0b0010) /* Bit position 1 */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_X;
				if (*b & 0b0100) /* Bit position 2 */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_R;
				if (*b & 0b1000) /* Bit position 3 */
				{
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_W;
					instruction->rex_w_prefix = true;
				}

				continue;
			}
		}

		break;
	}

	/* Calculate the number of prefixes based on how much the iterator moved */
	instruction->num_prefixes = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));

	/* Assume the instruction uses legacy encoding. It is most likely the case */
	instruction->encoding = NMD_X86_ENCODING_LEGACY;

	/* Opcode byte. This variable is used because 'op' is simpler than 'instruction->opcode' */
	uint8_t op;
	_NMD_READ_BYTE(b, buffer_size, op);

	if (op == 0x0F) /* 2 or 3 byte opcode */
	{
		_NMD_READ_BYTE(b, buffer_size, op);

		if (op == 0x38 || op == 0x3A) /* 3 byte opcode */
		{
			instruction->opcode_size = 3;
			instruction->opcode_map = (uint8_t)(op == 0x38 ? NMD_X86_OPCODE_MAP_0F38 : NMD_X86_OPCODE_MAP_0F3A);

			_NMD_READ_BYTE(b, buffer_size, op);
			instruction->opcode = op;

			if (!_nmd_decode_modrm(&b, &buffer_size, instruction))
				return false;

			const nmd_x86_modrm modrm = instruction->modrm;
			if (instruction->opcode_map == NMD_X86_OPCODE_MAP_0F38)
			{
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					/* Check if the instruction is invalid. */
					if (op == 0x36)
					{
						return false;
					}
					else if (op <= 0xb || (op >= 0x1c && op <= 0x1e))
					{
						if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							return false;
					}
					else if (op >= 0xc8 && op <= 0xcd)
					{
						if (instruction->simd_prefix)
							return false;
					}
					else if (op == 0x10 || op == 0x14 || op == 0x15 || op == 0x17 || (op >= 0x20 && op <= 0x25) || op == 0x28 || op == 0x29 || op == 0x2b || _NMD_R(op) == 3 || op == 0x40 || op == 0x41 || op == 0xcf || (op >= 0xdb && op <= 0xdf))
					{
						if (instruction->simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0x2a || (op >= 0x80 && op <= 0x82))
					{
						if (modrm.fields.mod == 0b11 || instruction->simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0xf0 || op == 0xf1)
					{
						if (modrm.fields.mod == 0b11 && (instruction->simd_prefix == NMD_X86_PREFIXES_NONE || instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
							return false;
						else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
							return false;
					}
					else if (op == 0xf5 || op == 0xf8)
					{
						if (instruction->simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || modrm.fields.mod == 0b11)
							return false;
					}
					else if (op == 0xf6)
					{
						if (instruction->simd_prefix == NMD_X86_PREFIXES_NONE && modrm.fields.mod == 0b11)
							return false;
						else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							return false;
					}
					else if (op == 0xf9)
					{
						if (instruction->simd_prefix != NMD_X86_PREFIXES_NONE || modrm.fields.mod == 0b11)
							return false;
					}
					else
						return false;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					if (_NMD_R(op) == 0x00)
						instruction->id = NMD_X86_INSTRUCTION_PSHUFB + op;
					else if (op >= 0x1c && op <= 0x1e)
						instruction->id = NMD_X86_INSTRUCTION_PABSB + (op - 0x1c);
					else if (_NMD_R(op) == 2)
						instruction->id = NMD_X86_INSTRUCTION_PMOVSXBW + _NMD_C(op);
					else if (_NMD_R(op) == 3)
						instruction->id = NMD_X86_INSTRUCTION_PMOVZXBW + _NMD_C(op);
					else if (_NMD_R(op) == 8)
						instruction->id = NMD_X86_INSTRUCTION_INVEPT + _NMD_C(op);
					else if (_NMD_R(op) == 0xc)
						instruction->id = NMD_X86_INSTRUCTION_SHA1NEXTE + (_NMD_C(op) - 8);
					else if (_NMD_R(op) == 0xd)
						instruction->id = NMD_X86_INSTRUCTION_AESIMC + (_NMD_C(op) - 0xb);
					else
					{
						switch (op)
						{
						case 0x10: instruction->id = NMD_X86_INSTRUCTION_PBLENDVB; break;
						case 0x14: instruction->id = NMD_X86_INSTRUCTION_BLENDVPS; break;
						case 0x15: instruction->id = NMD_X86_INSTRUCTION_BLENDVPD; break;
						case 0x17: instruction->id = NMD_X86_INSTRUCTION_PTEST; break;
						case 0x40: instruction->id = NMD_X86_INSTRUCTION_PMULLD; break;
						case 0x41: instruction->id = NMD_X86_INSTRUCTION_PHMINPOSUW; break;
						case 0xf0: case 0xf1: instruction->id = (uint16_t)((instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simd_prefix == 0x00) ? NMD_X86_INSTRUCTION_MOVBE : NMD_X86_INSTRUCTION_CRC32); break;
						case 0xf6: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_ADCX : NMD_X86_INSTRUCTION_ADOX); break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
				if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
				{
					if (op == 0x80 || op == 0x81) /* invept,invvpid */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
					}
					else if (op == 0xf6)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) /* adcx */
							instruction->modified_flags.eflags = instruction->tested_flags.eflags = NMD_X86_EFLAGS_CF;
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT) /* adox */
							instruction->modified_flags.eflags = instruction->tested_flags.eflags = NMD_X86_EFLAGS_OF;
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					instruction->num_operands = 2;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;

					if (_NMD_R(op) == 0 || (op >= 0x1c && op <= 0x1e))
					{
						_nmd_decode_operand_Pq(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Qq(instruction, &instruction->operands[1]);
					}
					else if (_NMD_R(op) == 8)
					{
						_nmd_decode_operand_Gy(instruction, &instruction->operands[0]);
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[1]);
					}
					else if (_NMD_R(op) >= 1 && _NMD_R(op) <= 0xe)
					{
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
					}
					else if (op == 0xf6)
					{
						_nmd_decode_operand_Gy(instruction, &instruction->operands[!instruction->simd_prefix ? 1 : 0]);
						_nmd_decode_operand_Ey(instruction, &instruction->operands[!instruction->simd_prefix ? 0 : 1]);
					}
					else if (op == 0xf0 || op == 0xf1)
					{
						if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || (instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) == (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
						{
							_nmd_decode_operand_Gd(instruction, &instruction->operands[0]);
							if (op == 0xf0)
								_nmd_decode_operand_Eb(instruction, &instruction->operands[1]);
							else if (instruction->prefixes == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
								_nmd_decode_operand_Ey(instruction, &instruction->operands[1]);
							else
								_nmd_decode_operand_Ew(instruction, &instruction->operands[1]);
						}
						else
						{
							if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
								_nmd_decode_operand_Gw(instruction, &instruction->operands[op == 0xf0 ? 0 : 1]);
							else
								_nmd_decode_operand_Gy(instruction, &instruction->operands[op == 0xf0 ? 0 : 1]);

							_nmd_decode_memory_operand(instruction, &instruction->operands[op == 0xf0 ? 1 : 0], (uint8_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : (instruction->rex_w_prefix ? NMD_X86_REG_RAX : NMD_X86_REG_EAX)));
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
			else /* 0x3a */
			{
				instruction->imm_mask = NMD_X86_IMM8;
				_NMD_READ_BYTE(b, buffer_size, instruction->immediate);

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					/* Check if the instruction is invalid. */
					if ((op >= 0x8 && op <= 0xe) || (op >= 0x14 && op <= 0x17) || (op >= 0x20 && op <= 0x22) || (op >= 0x40 && op <= 0x42) || op == 0x44 || (op >= 0x60 && op <= 0x63) || op == 0xdf || op == 0xce || op == 0xcf)
					{
						if (instruction->simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0x0f || op == 0xcc)
					{
						if (instruction->simd_prefix)
							return false;
					}
					else
						return false;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					if (_NMD_R(op) == 0)
						instruction->id = NMD_X86_INSTRUCTION_ROUNDPS + (op - 8);
					else if (_NMD_R(op) == 4)
						instruction->id = NMD_X86_INSTRUCTION_DPPS + _NMD_C(op);
					else if (_NMD_R(op) == 6)
						instruction->id = NMD_X86_INSTRUCTION_PCMPESTRM + _NMD_C(op);
					else
					{
						switch (op)
						{
						case 0x14: instruction->id = NMD_X86_INSTRUCTION_PEXTRB; break;
						case 0x15: instruction->id = NMD_X86_INSTRUCTION_PEXTRW; break;
						case 0x16: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_PEXTRQ : NMD_X86_INSTRUCTION_PEXTRD); break;
						case 0x17: instruction->id = NMD_X86_INSTRUCTION_EXTRACTPS; break;
						case 0x20: instruction->id = NMD_X86_INSTRUCTION_PINSRB; break;
						case 0x21: instruction->id = NMD_X86_INSTRUCTION_INSERTPS; break;
						case 0x22: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_PINSRQ : NMD_X86_INSTRUCTION_PINSRD); break;
						case 0xcc: instruction->id = NMD_X86_INSTRUCTION_SHA1RNDS4; break;
						case 0xdf: instruction->id = NMD_X86_INSTRUCTION_AESKEYGENASSIST; break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					instruction->num_operands = 3;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = instruction->operands[2].action = NMD_X86_OPERAND_ACTION_READ;
					instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;

					if (op == 0x0f && !instruction->simd_prefix)
					{
						_nmd_decode_operand_Pq(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Qq(instruction, &instruction->operands[1]);
					}
					else if (_NMD_R(op) == 1)
					{
						_nmd_decode_memory_operand(instruction, &instruction->operands[0], NMD_X86_REG_EAX);
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[1]);
					}
					else if (_NMD_R(op) == 2)
					{
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
						_nmd_decode_memory_operand(instruction, &instruction->operands[1], (uint8_t)(_NMD_C(op) == 1 ? NMD_X86_REG_XMM0 : NMD_X86_REG_EAX));
					}
					else if (op == 0xcc || op == 0xdf || _NMD_R(op) == 4 || _NMD_R(op) == 6 || _NMD_R(op) == 0)
					{
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
		}
		else if (op == 0x0f) /* 3DNow! opcode map*/
		{
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_3DNOW
			if (flags & NMD_X86_DECODER_FLAGS_3DNOW)
			{
				if (!_nmd_decode_modrm(&b, &buffer_size, instruction))
					return false;

				instruction->encoding = NMD_X86_ENCODING_3DNOW;
				instruction->opcode = 0x0f;
				instruction->imm_mask = NMD_X86_IMM8; /* The real opcode is encoded as the immediate byte. */
				_NMD_READ_BYTE(b, buffer_size, instruction->immediate);

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (!_nmd_find_byte(_nmd_valid_3DNow_opcodes, sizeof(_nmd_valid_3DNow_opcodes), (uint8_t)instruction->immediate))
					return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

			}
			else
				return false;
#else /* NMD_ASSEMBLY_DISABLE_DECODER_3DNOW */
			return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_3DNOW */
		}
		else /* 2 byte opcode. */
		{
			instruction->opcode_size = 2;
			instruction->opcode = op;
			instruction->opcode_map = NMD_X86_OPCODE_MAP_0F;

			/* Check for ModR/M, SIB and displacement */
			if (op >= 0x20 && op <= 0x23 && buffer_size == 2)
			{
				instruction->has_modrm = true;
				_NMD_READ_BYTE(b, buffer_size, instruction->modrm.modrm);
			}
			else if (op < 4 || (_NMD_R(op) != 3 && _NMD_R(op) > 0 && _NMD_R(op) < 7) || (op >= 0xD0 && op != 0xFF) || (_NMD_R(op) == 7 && _NMD_C(op) != 7) || _NMD_R(op) == 9 || _NMD_R(op) == 0xB || (_NMD_R(op) == 0xC && _NMD_C(op) < 8) || (_NMD_R(op) == 0xA && (op % 8) >= 3) || op == 0x0ff || op == 0x00 || op == 0x0d)
			{
				if (!_nmd_decode_modrm(&b, &buffer_size, instruction))
					return false;
			}

			const nmd_x86_modrm modrm = instruction->modrm;
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
			if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
			{
				/* Check if the instruction is invalid. */
				if (_nmd_find_byte(_nmd_invalid_op2, sizeof(_nmd_invalid_op2), op))
					return false;
				else if (op == 0xc7)
				{
					if ((!instruction->simd_prefix && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= 0b101 : modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010)) || (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (modrm.fields.mod == 0b11 || modrm.fields.reg != 0b001)) || ((instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT) && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? 0b110 : 0b101) : (modrm.fields.reg != 0b001 && modrm.fields.reg != 0b110))))
						return false;
				}
				else if (op == 0x00)
				{
					if (modrm.fields.reg >= 0b110)
						return false;
				}
				else if (op == 0x01)
				{
					if ((modrm.fields.mod == 0b11 ? ((instruction->simd_prefix & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO | NMD_X86_PREFIXES_REPEAT) && ((modrm.modrm >= 0xc0 && modrm.modrm <= 0xc5) || (modrm.modrm >= 0xc8 && modrm.modrm <= 0xcb) || (modrm.modrm >= 0xcf && modrm.modrm <= 0xd1) || (modrm.modrm >= 0xd4 && modrm.modrm <= 0xd7) || modrm.modrm == 0xee || modrm.modrm == 0xef || modrm.modrm == 0xfa || modrm.modrm == 0xfb)) || (modrm.fields.reg == 0b000 && modrm.fields.rm >= 0b110) || (modrm.fields.reg == 0b001 && modrm.fields.rm >= 0b100 && modrm.fields.rm <= 0b110) || (modrm.fields.reg == 0b010 && (modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011)) || (modrm.fields.reg == 0b101 && modrm.fields.rm < 0b110 && (!(instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT) || (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT && (modrm.fields.rm != 0b000 && modrm.fields.rm != 0b010)))) || (modrm.fields.reg == 0b111 && (modrm.fields.rm > 0b101 || (mode != NMD_X86_MODE_64 && modrm.fields.rm == 0b000)))) : (!(instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT) && modrm.fields.reg == 0b101)))
						return false;
				}
				else if (op == 0x1A || op == 0x1B)
				{
					if (modrm.fields.mod == 0b11)
						return false;
				}
				else if (op == 0x20 || op == 0x22)
				{
					if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b101)
						return false;
				}
				else if (op >= 0x24 && op <= 0x27)
					return false;
				else if (op >= 0x3b && op <= 0x3f)
					return false;
				else if (_NMD_R(op) == 5)
				{
					if ((op == 0x50 && modrm.fields.mod != 0b11) || (instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x52 || op == 0x53)) || (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT && (op == 0x50 || (op >= 0x54 && op <= 0x57))) || (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (op == 0x50 || (op >= 0x52 && op <= 0x57) || op == 0x5b)))
						return false;
				}
				else if (_NMD_R(op) == 6)
				{
					if ((!(instruction->simd_prefix & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op == 0x6c || op == 0x6d)) || (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT && op != 0x6f) || instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op == 0x78 || op == 0x79)
				{
					if ((((instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && op == 0x78) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b000)) || ((instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && modrm.fields.mod != 0b11)) || (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT))
						return false;
				}
				else if (op == 0x7c || op == 0x7d)
				{
					if (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT || !(instruction->simd_prefix & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
						return false;
				}
				else if (op == 0x7e || op == 0x7f)
				{
					if (instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op >= 0x71 && op <= 0x73)
				{
					if (instruction->simd_prefix & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || modrm.modrm <= 0xcf || (modrm.modrm >= 0xe8 && modrm.modrm <= 0xef))
						return false;
				}
				else if (op == 0x73)
				{
					if (modrm.modrm >= 0xe0 && modrm.modrm <= 0xe8)
						return false;
				}
				else if (op == 0xa6)
				{
					if (modrm.modrm != 0xc0 && modrm.modrm != 0xc8 && modrm.modrm != 0xd0)
						return false;
				}
				else if (op == 0xa7)
				{
					if (!(modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b101 && modrm.fields.rm == 0b000))
						return false;
				}
				else if (op == 0xae)
				{
					if (((!instruction->simd_prefix && modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b100) || (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b110)) || (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (modrm.fields.reg < 0b110 || (modrm.fields.mod == 0b11 && modrm.fields.reg == 0b111))) || (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.reg != 0b100 && modrm.fields.reg != 0b110) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b101))))
						return false;
				}
				else if (op == 0xb8)
				{
					if (!(instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT))
						return false;
				}
				else if (op == 0xba)
				{
					if (modrm.fields.reg <= 0b011)
						return false;
				}
				else if (op == 0xd0)
				{
					if (!instruction->simd_prefix || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
						return false;
				}
				else if (op == 0xe0)
				{
					if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op == 0xf0)
				{
					if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? modrm.fields.mod == 0b11 : true)
						return false;
				}
				else if (instruction->simd_prefix & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
				{
					if ((op >= 0x13 && op <= 0x17 && !(op == 0x16 && instruction->simd_prefix & NMD_X86_PREFIXES_REPEAT)) || op == 0x28 || op == 0x29 || op == 0x2e || op == 0x2f || (op <= 0x76 && op >= 0x74))
						return false;
				}
				else if (op == 0x71 || op == 0x72 || (op == 0x73 && !(instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)))
				{
					if ((modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf) || modrm.modrm >= 0xf8)
						return false;
				}
				else if (op >= 0xc3 && op <= 0xc6)
				{
					if ((op == 0xc5 && modrm.fields.mod != 0b11) || (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || (op == 0xc3 && instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
						return false;
				}
				else if (_NMD_R(op) >= 0xd && _NMD_C(op) != 0 && op != 0xff && ((_NMD_C(op) == 6 && _NMD_R(op) != 0xf) ? (!instruction->simd_prefix || (_NMD_R(op) == 0xD && (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) ? modrm.fields.mod != 0b11 : false)) : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || ((_NMD_C(op) == 7 && _NMD_R(op) != 0xe) ? modrm.fields.mod != 0b11 : false))))
					return false;
				else if (modrm.fields.mod == 0b11)
				{
					if (op == 0xb2 || op == 0xb4 || op == 0xb5 || op == 0xc3 || op == 0xe7 || op == 0x2b || (instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x12 || op == 0x16)) || (!(instruction->simd_prefix & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op == 0x13 || op == 0x17)))
						return false;
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

			if (_NMD_R(op) == 8) /* imm32 */
				instruction->imm_mask = _NMD_GET_BY_MODE_OPSZPRFX_F64(mode, instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE, NMD_X86_IMM16, NMD_X86_IMM32, NMD_X86_IMM32);
			else if ((_NMD_R(op) == 7 && _NMD_C(op) < 4) || op == 0xA4 || op == 0xC2 || (op > 0xC3 && op <= 0xC6) || op == 0xBA || op == 0xAC) /* imm8 */
				instruction->imm_mask = NMD_X86_IMM8;
			else if (op == 0x78 && (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) /* imm8 + imm8 = "imm16" */
				instruction->imm_mask = NMD_X86_IMM16;

			/* Make sure we can read 'instruction->imm_mask' bytes from the buffer */
			if (buffer_size < instruction->imm_mask)
				return false;

			/* Copy 'instruction->imm_mask' bytes from the buffer */
			for (i = 0; i < instruction->imm_mask; i++)
				((uint8_t*)(&instruction->immediate))[i] = b[i];

			/* Increment the buffer and decrement the buffer's size */
			b += instruction->imm_mask;
			buffer_size -= instruction->imm_mask;

			if (_NMD_R(op) == 8 && instruction->immediate & ((uint64_t)(1) << (instruction->imm_mask * 8 - 1)))
				instruction->immediate |= 0xffffffffffffffff << (instruction->imm_mask * 8);

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
			if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
			{
				if (_NMD_R(op) == 8)
					instruction->id = NMD_X86_INSTRUCTION_JO + _NMD_C(op);
				else if (op >= 0xa2 && op <= 0xa5)
					instruction->id = NMD_X86_INSTRUCTION_CPUID + (op - 0xa2);
				else if (op == 0x05)
					instruction->id = NMD_X86_INSTRUCTION_SYSCALL;
				else if (_NMD_R(op) == 4)
					instruction->id = NMD_X86_INSTRUCTION_CMOVO + _NMD_C(op);
				else if (op == 0x00)
					instruction->id = NMD_X86_INSTRUCTION_SLDT + modrm.fields.reg;
				else if (op == 0x01)
				{
					if (modrm.fields.mod == 0b11)
					{
						switch (modrm.fields.reg)
						{
						case 0b000: instruction->id = NMD_X86_INSTRUCTION_VMCALL + modrm.fields.rm; break;
						case 0b001: instruction->id = NMD_X86_INSTRUCTION_MONITOR + modrm.fields.rm; break;
						case 0b010: instruction->id = NMD_X86_INSTRUCTION_XGETBV + modrm.fields.rm; break;
						case 0b011: instruction->id = NMD_X86_INSTRUCTION_VMRUN + modrm.fields.rm; break;
						case 0b100: instruction->id = NMD_X86_INSTRUCTION_SMSW; break;
						case 0b110: instruction->id = NMD_X86_INSTRUCTION_LMSW; break;
						case 0b111: instruction->id = (uint16_t)(modrm.fields.rm == 0b000 ? NMD_X86_INSTRUCTION_SWAPGS : NMD_X86_INSTRUCTION_RDTSCP); break;
						}
					}
					else
						instruction->id = NMD_X86_INSTRUCTION_SGDT + modrm.fields.reg;
				}
				else if (op <= 0x0b)
					instruction->id = NMD_X86_INSTRUCTION_LAR + (op - 2);
				else if (op == 0x19 || (op >= 0x1c && op <= 0x1f))
				{
					if (op == 0x1e && modrm.modrm == 0xfa)
						instruction->id = NMD_X86_INSTRUCTION_ENDBR64;
					else if (op == 0x1e && modrm.modrm == 0xfb)
						instruction->id = NMD_X86_INSTRUCTION_ENDBR32;
					else
						instruction->id = NMD_X86_INSTRUCTION_NOP;
				}
				else if (op >= 0x10 && op <= 0x17)
				{
					switch (instruction->simd_prefix)
					{
					case NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + _NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + _NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT_NOT_ZERO: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + _NMD_C(op); break;
					default: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + _NMD_C(op); break;
					}
				}
				else if (op >= 0x20 && op <= 0x23)
					instruction->id = NMD_X86_INSTRUCTION_MOV;
				else if (_NMD_R(op) == 3)
					instruction->id = NMD_X86_INSTRUCTION_WRMSR + _NMD_C(op);
				else if (_NMD_R(op) == 5)
				{
					switch (instruction->simd_prefix)
					{
					case NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE: instruction->id = NMD_X86_INSTRUCTION_MOVMSKPD + _NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT: instruction->id = NMD_X86_INSTRUCTION_BNDMOV + _NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT_NOT_ZERO: instruction->id = NMD_X86_INSTRUCTION_BNDCL + _NMD_C(op); break;
					default: instruction->id = NMD_X86_INSTRUCTION_MOVMSKPS + _NMD_C(op); break;
					}
				}
				else if (op >= 0x60 && op <= 0x6d)
					instruction->id = NMD_X86_INSTRUCTION_PUNPCKLBW + _NMD_C(op);
				else if (op >= 0x74 && op <= 0x76)
					instruction->id = NMD_X86_INSTRUCTION_PCMPEQB + (op - 0x74);
				else if (op >= 0xb2 && op <= 0xb5)
					instruction->id = NMD_X86_INSTRUCTION_LSS + (op - 0xb2);
				else if (op >= 0xc3 && op <= 0xc5)
					instruction->id = NMD_X86_INSTRUCTION_MOVNTI + (op - 0xc3);
				else if (op == 0xc7)
				{
					if (modrm.fields.reg == 0b001)
						instruction->id = (uint16_t)(instruction->rex_w_prefix ? NMD_X86_INSTRUCTION_CMPXCHG16B : NMD_X86_INSTRUCTION_CMPXCHG8B);
					else if (modrm.fields.reg == 0b111)
						instruction->id = (uint16_t)(modrm.fields.mod == 0b11 ? (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_RDPID : NMD_X86_INSTRUCTION_RDSEED) : NMD_X86_INSTRUCTION_VMPTRST);
					else
						instruction->id = (uint16_t)(modrm.fields.mod == 0b11 ? NMD_X86_INSTRUCTION_RDRAND : (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_VMCLEAR : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_VMXON : NMD_X86_INSTRUCTION_VMPTRLD)));
				}
				else if (op >= 0xc8 && op <= 0xcf)
					instruction->id = NMD_X86_INSTRUCTION_BSWAP;
				else if (op == 0xa3)
					instruction->id = (uint16_t)((modrm.fields.mod == 0b11 ? NMD_X86_INSTRUCTION_RDFSBASE : NMD_X86_INSTRUCTION_FXSAVE) + modrm.fields.reg);
				else if (op >= 0xd1 && op <= 0xfe)
				{
					if (op == 0xd6)
						instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVQ : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVQ2DQ : NMD_X86_INSTRUCTION_MOVDQ2Q));
					else if (op == 0xe6)
						instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTTPD2DQ : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTDQ2PD : NMD_X86_INSTRUCTION_CVTPD2DQ));
					else if (op == 0xe7)
						instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVNTDQ : NMD_X86_INSTRUCTION_MOVNTQ);
					else if (op == 0xf7)
						instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MASKMOVDQU : NMD_X86_INSTRUCTION_MASKMOVQ);
					else
						instruction->id = NMD_X86_INSTRUCTION_PSRLW + (op - 0xd1);
				}
				else
				{
					switch (op)
					{
					case 0xa0: case 0xa8: instruction->id = NMD_X86_INSTRUCTION_PUSH; break;
					case 0xa1: case 0xa9: instruction->id = NMD_X86_INSTRUCTION_POP; break;
					case 0xaf: instruction->id = NMD_X86_INSTRUCTION_IMUL; break;
					case 0xb0: case 0xb1: instruction->id = NMD_X86_INSTRUCTION_CMPXCHG; break;
					case 0x10: case 0x11: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVUPD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSS : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_MOVSD : NMD_X86_INSTRUCTION_MOVUPD))); break;
					case 0x12: case 0x13: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVLPD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSLDUP : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_MOVDDUP : NMD_X86_INSTRUCTION_MOVLPS))); break;
					case 0x14: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UNPCKLPD : NMD_X86_INSTRUCTION_UNPCKLPS); break;
					case 0x15: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UNPCKHPD : NMD_X86_INSTRUCTION_UNPCKHPS); break;
					case 0x16: case 0x17: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVHPD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSHDUP : NMD_X86_INSTRUCTION_MOVHPS)); break;
					case 0x18: instruction->id = (uint16_t)(modrm.fields.reg >= 0b100 ? NMD_X86_INSTRUCTION_NOP : (modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_PREFETCHNTA : (modrm.fields.reg == 0b001 ? NMD_X86_INSTRUCTION_PREFETCHT0 : (modrm.fields.reg == 0b010 ? NMD_X86_INSTRUCTION_PREFETCHT1 : NMD_X86_INSTRUCTION_PREFETCHT2)))); break;
					case 0x1a: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_BNDMOV : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BNDCL : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_BNDCU : NMD_X86_INSTRUCTION_BNDLDX))); break;
					case 0x1b: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_BNDMOV : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BNDMK : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_BNDCN : NMD_X86_INSTRUCTION_BNDSTX))); break;
					case 0x28: case 0x29: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVAPD : NMD_X86_INSTRUCTION_MOVAPS); break;
					case 0x2a: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTPI2PD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTSI2SS : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTSI2SD : NMD_X86_INSTRUCTION_CVTPI2PS))); break;
					case 0x2b: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVNTPD : NMD_X86_INSTRUCTION_MOVNTPS); break;
					case 0x2c: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTTPD2PI : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTTSS2SI : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTTSS2SI : NMD_X86_INSTRUCTION_CVTTPS2PI))); break;
					case 0x2d: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTPD2PI : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTSS2SI : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTSS2SI : NMD_X86_INSTRUCTION_CVTPS2PI))); break;
					case 0x2e: case 0x2f: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UCOMISD : NMD_X86_INSTRUCTION_UCOMISS); break;
					case 0x6e: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && !instruction->rex_w_prefix && (instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_INSTRUCTION_MOVQ : NMD_X86_INSTRUCTION_MOVD); break;
					case 0x6f: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVDQA : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVDQU : NMD_X86_INSTRUCTION_MOVQ)); break;
					case 0x70: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_PSHUFD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_PSHUFHW : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_PSHUFLW : NMD_X86_INSTRUCTION_PSHUFW))); break;
					case 0x71: instruction->id = (uint16_t)(modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_PSRLQ : (modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_PSRAW : NMD_X86_INSTRUCTION_PSLLW)); break;
					case 0x72: instruction->id = (uint16_t)(modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_PSRLD : (modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_PSRAD : NMD_X86_INSTRUCTION_PSLLD)); break;
					case 0x73: instruction->id = (uint16_t)(modrm.fields.reg == 0b010 ? NMD_X86_INSTRUCTION_PSRLQ : (modrm.fields.reg == 0b011 ? NMD_X86_INSTRUCTION_PSRLDQ : (modrm.fields.reg == 0b110 ? NMD_X86_INSTRUCTION_PSLLQ : NMD_X86_INSTRUCTION_PSLLDQ))); break;
					case 0x78: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_EXTRQ : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_INSERTQ : NMD_X86_INSTRUCTION_VMREAD)); break;
					case 0x79: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_EXTRQ : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_INSERTQ : NMD_X86_INSTRUCTION_VMWRITE)); break;
					case 0x7c: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_HADDPD : NMD_X86_INSTRUCTION_HADDPS); break;
					case 0x7d: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_HSUBPD : NMD_X86_INSTRUCTION_HSUBPS); break;
					case 0x7e: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || (instruction->rex_w_prefix && instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) ? NMD_X86_INSTRUCTION_MOVQ : NMD_X86_INSTRUCTION_MOVD); break;
					case 0x7f: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVDQA : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVDQU : NMD_X86_INSTRUCTION_MOVQ)); break;
					case 0x77: instruction->id = NMD_X86_INSTRUCTION_EMMS; break;
					case 0x0e: instruction->id = NMD_X86_INSTRUCTION_FEMMS; break;
					case 0xa3: instruction->id = NMD_X86_INSTRUCTION_BT; break;
					case 0xa4: case 0xa5: instruction->id = NMD_X86_INSTRUCTION_SHLD; break;
					case 0xaa: instruction->id = NMD_X86_INSTRUCTION_RSM; break;
					case 0xab: instruction->id = NMD_X86_INSTRUCTION_BTS; break;
					case 0xac: case 0xad: instruction->id = NMD_X86_INSTRUCTION_SHRD; break;
					case 0xb6: case 0xb7: instruction->id = NMD_X86_INSTRUCTION_MOVZX; break;
					case 0xb8: instruction->id = NMD_X86_INSTRUCTION_POPCNT; break;
					case 0xb9: instruction->id = NMD_X86_INSTRUCTION_UD1; break;
					case 0xba: instruction->id = (uint16_t)(modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_BT : (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_BTS : (modrm.fields.reg == 0b110 ? NMD_X86_INSTRUCTION_BTR : NMD_X86_INSTRUCTION_BTC))); break;
					case 0xbb: instruction->id = NMD_X86_INSTRUCTION_BTC; break;
					case 0xbc: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BSF : NMD_X86_INSTRUCTION_TZCNT); break;
					case 0xbd: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BSR : NMD_X86_INSTRUCTION_LZCNT); break;
					case 0xbe: case 0xbf: instruction->id = NMD_X86_INSTRUCTION_MOVSX; break;
					case 0xc0: case 0xc1: instruction->id = NMD_X86_INSTRUCTION_XADD; break;
					case 0xc2: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CMPPD : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CMPSS : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CMPSD : NMD_X86_INSTRUCTION_CMPPS))); break;
					case 0xd0: instruction->id = (uint16_t)(instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_ADDSUBPD : NMD_X86_INSTRUCTION_ADDSUBPS); break;
					case 0xff: instruction->id = NMD_X86_INSTRUCTION_UD0; break;
					}
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
			if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
			{
				if (_NMD_R(op) == 4 || _NMD_R(op) == 8 || _NMD_R(op) == 9) /* Conditional Move (CMOVcc),Conditional jump(Jcc),Byte set on condition(SETcc) */
					_nmd_decode_conditional_flag(instruction, _NMD_C(op));
				else if (op == 0x05 || op == 0x07) /* syscall,sysret */
				{
					instruction->cleared_flags.eflags = op == 0x05 ? NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_RF : NMD_X86_EFLAGS_RF;
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
				}
				else if (op == 0x34) /* sysenter */
					instruction->cleared_flags.eflags = NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM;
				else if (op == 0xaa) /* rsm */
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
				else if (op == 0xaf) /* mul */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF;
					instruction->undefined_flags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF;
				}
				else if (op == 0xb0 || op == 0xb1) /* cmpxchg */
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				else if (op == 0xc0 || op == 0xc1) /* xadd */
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				else if (op == 0x00 && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101)) /* verr,verw*/
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_OF;
				else if (op == 0x01 && modrm.fields.mod == 0b11)
				{
					if (modrm.fields.reg == 0b000)
					{
						if (modrm.fields.rm == 0b001 || modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011) /* vmcall,vmlaunch,vmresume */
						{
							instruction->tested_flags.eflags = NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_VM;
							instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
						}
					}
				}
				else if (op == 0x34)
					instruction->cleared_flags.eflags = NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_IF;
				else if (op == 0x78 || op == 0x79) /* vmread,vmwrite */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
					instruction->cleared_flags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0x02 || op == 0x03) /* lar,lsl */
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_ZF;
				else if (op == 0xa3 || op == 0xab || op == 0xb3 || op == 0xba || op == 0xbb) /* bt,bts,btc */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF;
					instruction->undefined_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
				}
				else if (op == 0xa4 || op == 0xa5 || op == 0xac || op == 0xad || op == 0xbc) /* shld,shrd */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF;
					instruction->undefined_flags.eflags = NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0xaa) /* rsm */
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
				else if ((op == 0xbc || op == 0xbd) && instruction->prefixes & NMD_X86_PREFIXES_REPEAT) /* tzcnt */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
					instruction->undefined_flags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0xbc || op == 0xbd) /* bsf */
				{
					instruction->modified_flags.eflags = NMD_X86_EFLAGS_ZF;
					instruction->undefined_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_GROUP
			/* Parse the instruction's group. */
			if (flags & NMD_X86_DECODER_FLAGS_GROUP)
			{
				if (_NMD_R(op) == 8)
					instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
				else if ((op == 0x01 && modrm.fields.rm == 0b111 && (modrm.fields.mod == 0b00 || modrm.modrm == 0xf8)) || op == 0x06 || op == 0x08 || op == 0x09 || op == 0x30 || op == 0x32 || op == 0x33 || op == 0x35 || op == 0x37)
					instruction->group = NMD_GROUP_PRIVILEGE;
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_GROUP */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
			if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
			{
				if (op == 0x2 || op == 0x3 || (op >= 0x10 && op <= 0x17) || _NMD_R(op) == 2 || (_NMD_R(op) >= 4 && _NMD_R(op) <= 7 && op != 0x77) || op == 0xa3 || op == 0xab || op == 0xaf || (_NMD_R(op) >= 0xc && op != 0xc7 && op != 0xff))
					instruction->num_operands = 2;
				else if (_NMD_R(op) == 9 || op == 0xc7)
					instruction->num_operands = 1;
				else if (op == 0xa4 || op == 0xa5 || op == 0xc2 || (op >= 0xc4 && op <= 0xc6))
					instruction->num_operands = 3;

				if (op == 0x05 || op == 0x07) /* syscall,sysret */
				{
					instruction->num_operands = 5;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, op == 0x05 ? NMD_X86_OPERAND_ACTION_WRITE : NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_RCX);
					_NMD_SET_REG_OPERAND(instruction->operands[2], true, op == 0x05 ? NMD_X86_OPERAND_ACTION_WRITE : NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_R11);
					_NMD_SET_REG_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_CS);
					_NMD_SET_REG_OPERAND(instruction->operands[4], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS);
				}
				else if (op == 0x34 || op == 0x35) /* sysenter,sysexit */
				{
					instruction->num_operands = op == 0x34 ? 2 : 4;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
					if (op == 0x35)
					{
						_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, _NMD_GET_GPR(NMD_X86_REG_CX));
						_NMD_SET_REG_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_READ, _NMD_GET_GPR(NMD_X86_REG_DX));
					}
				}
				else if (_NMD_R(op) == 8) /* jCC rel32 */
				{
					instruction->num_operands = 2;
					_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_IP());
				}
				else if (op == 0x31) /* rdtsc */
				{
					instruction->num_operands = 2;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EAX);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EDX);
				}
				else if (op == 0xa2) /* cpuid */
				{
					instruction->num_operands = 4;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, NMD_X86_REG_EAX);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EBX);
					_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE | NMD_X86_OPERAND_ACTION_CONDREAD, NMD_X86_REG_ECX);
					_NMD_SET_REG_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EDX);
				}
				else if (op == 0xa0 || op == 0xa8) /* push fs,push gs */
				{
					instruction->num_operands = 3;
					_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, op == 0xa0 ? NMD_X86_REG_FS : NMD_X86_REG_GS);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
					_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
				}
				else if (op == 0x30 || op == 0x32 || op == 0x33) /* wrmsr,rdmsr,rdpmc */
				{
					instruction->num_operands = 3;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, op == 0x30 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EAX);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, op == 0x30 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_EDX);
					_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_ECX);
				}
				else if (op == 0xa1 || op == 0xa9) /* pop fs,pop gs */
				{
					instruction->num_operands = 3;
					_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_WRITE, op == 0xa1 ? NMD_X86_REG_FS : NMD_X86_REG_GS);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
					_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
				}
				else if (op == 0x37) /* getsec */
				{
					instruction->num_operands = 2;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDWRITE, NMD_X86_REG_EAX);
					_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_EBX);
				}
				else if (op == 0xaa) /* rsm */
				{
					instruction->num_operands = 1;
					_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
				}
				else if (op == 0x00)
				{
					if (instruction->modrm.fields.reg >= 0b010)
						_nmd_decode_operand_Ew(instruction, &instruction->operands[0]);
					else
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);

					instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
				}
				else if (op == 0x01)
				{
					if (instruction->modrm.fields.mod != 0b11)
					{
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
						instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
					}
					else if (instruction->modrm.fields.reg == 0b100)
						_nmd_decode_operand_Rv(instruction, &instruction->operands[0]);
					else if (instruction->modrm.fields.reg == 0b110)
					{
						_nmd_decode_operand_Ew(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
					}

					if (instruction->modrm.fields.reg == 0b100)
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
				}
				else if (op == 0x02 || op == 0x03)
				{
					_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Ew(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x0d)
				{
					_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 0x8)
				{
					instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
				}
				else if (_NMD_R(op) == 9)
				{
					_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
				}
				else if (op == 0x17)
				{
					_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Vdq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op >= 0x20 && op <= 0x23)
				{
					instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
					instruction->operands[op < 0x22 ? 0 : 1].fields.reg = NMD_X86_REG_EAX + instruction->modrm.fields.rm;
					instruction->operands[op < 0x22 ? 1 : 0].fields.reg = (uint8_t)((op % 2 == 0 ? NMD_X86_REG_CR0 : NMD_X86_REG_DR0) + instruction->modrm.fields.reg);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x29 || op == 0x2b || (op == 0x7f && instruction->simd_prefix))
				{
					_nmd_decode_operand_Wdq(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Vdq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x2a || op == 0x2c || op == 0x2d)
				{
					if (op == 0x2a)
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
					else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						_nmd_decode_operand_Gy(instruction, &instruction->operands[0]);
					else if (op == 0x2d && instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						_nmd_decode_operand_Qq(instruction, &instruction->operands[0]);
					else
						_nmd_decode_operand_Pq(instruction, &instruction->operands[0]);

					if (op == 0x2a)
					{
						if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT || instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							_nmd_decode_operand_Ey(instruction, &instruction->operands[1]);
						else
							_nmd_decode_operand_Qq(instruction, &instruction->operands[1]);
					}
					else
						_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x50)
				{
					_nmd_decode_operand_Gy(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Udq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 5 || (op >= 0x10 && op <= 0x16) || op == 0x28 || op == 0x2e || op == 0x2f || (op == 0x7e && instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT))
				{
					_nmd_decode_operand_Vdq(instruction, &instruction->operands[op == 0x11 || op == 0x13 ? 1 : 0]);
					_nmd_decode_operand_Wdq(instruction, &instruction->operands[op == 0x11 || op == 0x13 ? 0 : 1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x7e)
				{
					_nmd_decode_operand_Ey(instruction, &instruction->operands[0]);
					instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
					instruction->operands[1].fields.reg = (uint8_t)((instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_XMM0 : NMD_X86_REG_MM0) + instruction->modrm.fields.reg);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 6 || op == 0x70 || (op >= 0x74 && op <= 0x76) || (op >= 0x7c && op <= 0x7f))
				{
					if (!instruction->simd_prefix)
					{
						_nmd_decode_operand_Pq(instruction, &instruction->operands[op == 0x7f ? 1 : 0]);

						if (op == 0x6e)
							_nmd_decode_operand_Ey(instruction, &instruction->operands[1]);
						else
							_nmd_decode_operand_Qq(instruction, &instruction->operands[op == 0x7f ? 0 : 1]);
					}
					else
					{
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);

						if (op == 0x6e)
							_nmd_decode_operand_Ey(instruction, &instruction->operands[1]);
						else
							_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
					}

					if (op == 0x70)
						instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;

					instruction->operands[0].action = (uint8_t)(((op >= 0x60 && op <= 0x6d) || (op >= 0x74 && op <= 0x76)) ? NMD_X86_OPERAND_ACTION_READWRITE : NMD_X86_OPERAND_ACTION_WRITE);
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op >= 0x71 && op <= 0x73)
				{
					if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						_nmd_decode_operand_Udq(instruction, &instruction->operands[0]);
					else
						_nmd_decode_operand_Qq(instruction, &instruction->operands[0]);
					instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0x78 || op == 0x79)
				{
					if (instruction->simd_prefix)
					{
						if (op == 0x78)
						{
							i = 0;
							if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
								_nmd_decode_operand_Vdq(instruction, &instruction->operands[i++]);
							_nmd_decode_operand_Udq(instruction, &instruction->operands[i + 0]);
							instruction->operands[i + 1].type = instruction->operands[i + 2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							/* FIXME: We should not access the buffer from here
							instruction->operands[i + 1].fields.imm = b[1];
							instruction->operands[i + 2].fields.imm = b[2];
							*/
						}
						else
						{
							_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
							_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
						}
					}
					else
					{
						_nmd_decode_operand_Ey(instruction, &instruction->operands[op == 0x78 ? 0 : 1]);
						_nmd_decode_operand_Gy(instruction, &instruction->operands[op == 0x78 ? 1 : 0]);
					}
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 0xa && (op % 8) < 2)
				{
					instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
					instruction->operands[0].fields.reg = (uint8_t)(op > 0xa8 ? NMD_X86_REG_GS : NMD_X86_REG_FS);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if ((_NMD_R(op) == 0xa && ((op % 8) >= 3 && (op % 8) <= 5)) || op == 0xb3 || op == 0xbb)
				{
					_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Gv(instruction, &instruction->operands[1]);

					if (_NMD_R(op) == 0xa)
					{
						if ((op % 8) == 4)
							instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						else if ((op % 8) == 5)
						{
							instruction->operands[2].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[2].fields.reg = NMD_X86_REG_CL;
						}
					}

					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0xaf || op == 0xb8)
				{
					_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Ev(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0xba)
				{
					_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
					instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg <= 0b101 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READWRITE);
					instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 0xb && (op % 8) >= 6)
				{
					_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
					if ((op % 8) == 6)
						_nmd_decode_operand_Eb(instruction, &instruction->operands[1]);
					else
						_nmd_decode_operand_Ew(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (_NMD_R(op) == 0x4 || (_NMD_R(op) == 0xb && ((op % 8) == 0x4 || (op % 8) == 0x5)))
				{
					_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Ev(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if ((_NMD_R(op) == 0xb || _NMD_R(op) == 0xc) && _NMD_C(op) < 2)
				{
					if (_NMD_C(op) == 0)
					{
						_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Gb(instruction, &instruction->operands[1]);
					}
					else
					{
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Gv(instruction, &instruction->operands[1]);
					}

					if (_NMD_R(op) == 0xb)
					{
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDWRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else
						instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READWRITE;
				}
				else if (op == 0xb2)
				{
					_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
					_nmd_decode_modrm_upper32(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0xc3)
				{
					_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Gy(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
				else if (op == 0xc2 || op == 0xc6)
				{
					_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
				}
				else if (op == 0xc4)
				{
					if (instruction->prefixes == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
					else
						_nmd_decode_operand_Pq(instruction, &instruction->operands[0]);
					_nmd_decode_operand_Ey(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
				}
				else if (op == 0xc5)
				{
					_nmd_decode_operand_Gd(instruction, &instruction->operands[0]);
					if (instruction->prefixes == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						_nmd_decode_operand_Udq(instruction, &instruction->operands[1]);
					else
						_nmd_decode_operand_Nq(instruction, &instruction->operands[1]);
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
				}
				else if (op == 0xc7)
				{
					if (instruction->modrm.fields.mod == 0b11)
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
					else
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
					instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg == 0b001 ? (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDWRITE) : (instruction->modrm.fields.mod == 0b11 || !instruction->simd_prefix ? NMD_X86_OPERAND_ACTION_WRITE : NMD_X86_OPERAND_ACTION_READ));
				}
				else if (op >= 0xc8 && op <= 0xcf)
				{
					instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
					instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & (NMD_X86_PREFIXES_REX_W | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_REX_W | NMD_X86_PREFIXES_REX_B) ? NMD_X86_REG_R8 : (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8D : NMD_X86_REG_EAX)) + (op % 8));
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
				}
				else if (_NMD_R(op) >= 0xd)
				{
					if (op == 0xff)
					{
						_nmd_decode_operand_Gd(instruction, &instruction->operands[0]);
						_nmd_decode_memory_operand(instruction, &instruction->operands[1], NMD_X86_REG_EAX);
					}
					else if (op == 0xd6 && instruction->simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					{
						if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
						{
							_nmd_decode_operand_Vdq(instruction, &instruction->operands[0]);
							_nmd_decode_operand_Qq(instruction, &instruction->operands[1]);
						}
						else
						{
							_nmd_decode_operand_Pq(instruction, &instruction->operands[0]);
							_nmd_decode_operand_Wdq(instruction, &instruction->operands[1]);
						}
					}
					else
					{
						const size_t first_operand_index = op == 0xe7 || op == 0xd6 ? 1 : 0;
						const size_t second_operand_index = op == 0xe7 || op == 0xd6 ? 0 : 1;

						if (!instruction->simd_prefix)
						{
							if (op == 0xd7)
								_nmd_decode_operand_Gd(instruction, &instruction->operands[0]);
							else
								_nmd_decode_operand_Pq(instruction, &instruction->operands[first_operand_index]);
							_nmd_decode_operand_Qq(instruction, &instruction->operands[second_operand_index]);
						}
						else
						{
							if (op == 0xd7)
								_nmd_decode_operand_Gd(instruction, &instruction->operands[0]);
							else
								_nmd_decode_operand_Vdq(instruction, &instruction->operands[first_operand_index]);
							_nmd_decode_operand_Wdq(instruction, &instruction->operands[second_operand_index]);
						}
					}
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
		}
	}
	else /* 1 byte opcode */
	{
		instruction->opcode_size = 1;
		instruction->opcode = op;
		instruction->opcode_map = NMD_X86_OPCODE_MAP_DEFAULT;

		/* Check for ModR/M, SIB and displacement. */
		if (_NMD_R(op) == 8 || _nmd_find_byte(_nmd_op1_modrm, sizeof(_nmd_op1_modrm), op) || (_NMD_R(op) < 4 && (_NMD_C(op) < 4 || (_NMD_C(op) >= 8 && _NMD_C(op) < 0xC))) || (_NMD_R(op) == 0xD && _NMD_C(op) >= 8) /* FIXME: We should not access the buffer directly from here || (remaining_size > 1 && ((nmd_x86_modrm*)(b + 1))->fields.mod != 0b11 && (op == 0xc4 || op == 0xc5 || op == 0x62)) */)
		{
			if (!_nmd_decode_modrm(&b, &buffer_size, instruction))
				return false;
		}

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_EVEX
		/* Check if instruction is EVEX. */
		if (flags & NMD_X86_DECODER_FLAGS_EVEX && op == 0x62 && !instruction->has_modrm)
		{
			instruction->encoding = NMD_X86_ENCODING_EVEX;
			return false;
		}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_EVEX */
#if !defined(NMD_ASSEMBLY_DISABLE_DECODER_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_DECODER_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VEX
			/* Check if instruction is VEX. */
			if (flags & NMD_X86_DECODER_FLAGS_VEX && (op == 0xc4 || op == 0xc5) && !instruction->has_modrm)
			{
				instruction->encoding = NMD_X86_ENCODING_VEX;

				instruction->vex.vex[0] = op;

				uint8_t byte1;
				_NMD_READ_BYTE(b, buffer_size, byte1);

				instruction->vex.R = byte1 & 0b10000000;
				if (instruction->vex.vex[0] == 0xc4)
				{
					instruction->vex.X = (byte1 & 0b01000000) == 0b01000000;
					instruction->vex.B = (byte1 & 0b00100000) == 0b00100000;
					instruction->vex.m_mmmm = (uint8_t)(byte1 & 0b00011111);

					uint8_t byte2;
					_NMD_READ_BYTE(b, buffer_size, byte2);

					instruction->vex.W = (byte2 & 0b10000000) == 0b10000000;
					instruction->vex.vvvv = (uint8_t)((byte2 & 0b01111000) >> 3);
					instruction->vex.L = (byte2 & 0b00000100) == 0b00000100;
					instruction->vex.pp = (uint8_t)(byte2 & 0b00000011);

					_NMD_READ_BYTE(b, buffer_size, op);
					instruction->opcode = op;

					if (op == 0x0c || op == 0x0d || op == 0x40 || op == 0x41 || op == 0x17 || op == 0x21 || op == 0x42)
					{
						instruction->imm_mask = NMD_X86_IMM8;
						_NMD_READ_BYTE(b, buffer_size, instruction->immediate);
					}

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
					/* Check if the instruction is invalid. */
					if (op == 0x0c && instruction->vex.m_mmmm != 3)
						return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */


#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
					/*if(op == 0x0c)
						instruction->id = NMD_X86_INSTR
						*/
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */
				}
				else /* 0xc5 */
				{
					instruction->vex.vvvv = (uint8_t)(byte1 & 0b01111000);
					instruction->vex.L = byte1 & 0b00000100;
					instruction->vex.pp = (uint8_t)(byte1 & 0b00000011);

					_NMD_READ_BYTE(b, buffer_size, op);
					instruction->opcode = op;
				}

				if (!_nmd_decode_modrm(&b, &buffer_size, instruction))
					return false;
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VEX */
#if !(defined(NMD_ASSEMBLY_DISABLE_DECODER_EVEX) && defined(NMD_ASSEMBLY_DISABLE_DECODER_VEX))
			else
#endif
			{
				const nmd_x86_modrm modrm = instruction->modrm;
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				/* Check if the instruction is invalid. */
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					if (op == 0xC6 || op == 0xC7)
					{
						if ((modrm.fields.reg != 0b000 && modrm.fields.reg != 0b111) || (modrm.fields.reg == 0b111 && (modrm.fields.mod != 0b11 || modrm.fields.rm != 0b000)))
							return false;
					}
					else if (op == 0x8f)
					{
						if (modrm.fields.reg != 0b000)
							return false;
					}
					else if (op == 0xfe)
					{
						if (modrm.fields.reg >= 0b010)
							return false;
					}
					else if (op == 0xff)
					{
						if (modrm.fields.reg == 0b111 || (modrm.fields.mod == 0b11 && (modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101)))
							return false;
					}
					else if (op == 0x8c)
					{
						if (modrm.fields.reg >= 0b110)
							return false;
					}
					else if (op == 0x8e)
					{
						if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b110)
							return false;
					}
					else if (op == 0x62)
					{
						if (mode == NMD_X86_MODE_64)
							return false;
					}
					else if (op == 0x8d)
					{
						if (modrm.fields.mod == 0b11)
							return false;
					}
					else if (op == 0xc4 || op == 0xc5)
					{
						if (mode == NMD_X86_MODE_64 && instruction->has_modrm && modrm.fields.mod != 0b11)
							return false;
					}
					else if (op >= 0xd8 && op <= 0xdf)
					{
						switch (op)
						{
						case 0xd9:
							if ((modrm.fields.reg == 0b001 && modrm.fields.mod != 0b11) || (modrm.modrm > 0xd0 && modrm.modrm < 0xd8) || modrm.modrm == 0xe2 || modrm.modrm == 0xe3 || modrm.modrm == 0xe6 || modrm.modrm == 0xe7 || modrm.modrm == 0xef)
								return false;
							break;
						case 0xda:
							if (modrm.modrm >= 0xe0 && modrm.modrm != 0xe9)
								return false;
							break;
						case 0xdb:
							if (((modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110) && modrm.fields.mod != 0b11) || (modrm.modrm >= 0xe5 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
								return false;
							break;
						case 0xdd:
							if ((modrm.fields.reg == 0b101 && modrm.fields.mod != 0b11) || _NMD_R(modrm.modrm) == 0xf)
								return false;
							break;
						case 0xde:
							if (modrm.modrm == 0xd8 || (modrm.modrm >= 0xda && modrm.modrm <= 0xdf))
								return false;
							break;
						case 0xdf:
							if ((modrm.modrm >= 0xe1 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
								return false;
							break;
						}
					}
					else if (mode == NMD_X86_MODE_64)
					{
						if (op == 0x6 || op == 0x7 || op == 0xe || op == 0x16 || op == 0x17 || op == 0x1e || op == 0x1f || op == 0x27 || op == 0x2f || op == 0x37 || op == 0x3f || (op >= 0x60 && op <= 0x62) || op == 0x82 || op == 0xce || (op >= 0xd4 && op <= 0xd6))
							return false;
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

				/* Check for immediate */
				if (_nmd_find_byte(_nmd_op1_imm32, sizeof(_nmd_op1_imm32), op) || (_NMD_R(op) < 4 && (_NMD_C(op) == 5 || _NMD_C(op) == 0xD)) || (_NMD_R(op) == 0xB && _NMD_C(op) >= 8) || (op == 0xF7 && modrm.fields.reg == 0b000)) /* imm32,16 */
				{
					if (_NMD_R(op) == 0xB && _NMD_C(op) >= 8)
						instruction->imm_mask = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_IMM64 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_IMM16 : NMD_X86_IMM32));
					else
					{
						if ((mode == NMD_X86_MODE_16 && instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) || (mode != NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)))
							instruction->imm_mask = NMD_X86_IMM32;
						else
							instruction->imm_mask = NMD_X86_IMM16;
					}
				}
				else if (_NMD_R(op) == 7 || (_NMD_R(op) == 0xE && _NMD_C(op) < 8) || (_NMD_R(op) == 0xB && _NMD_C(op) < 8) || (_NMD_R(op) < 4 && (_NMD_C(op) == 4 || _NMD_C(op) == 0xC)) || (op == 0xF6 && modrm.fields.reg <= 0b001) || _nmd_find_byte(_nmd_op1_imm8, sizeof(_nmd_op1_imm8), op)) /* imm8 */
					instruction->imm_mask = NMD_X86_IMM8;
				else if (_NMD_R(op) == 0xA && _NMD_C(op) < 4)
					instruction->imm_mask = (uint8_t)(mode == NMD_X86_MODE_64 ? (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? NMD_X86_IMM32 : NMD_X86_IMM64) : (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? NMD_X86_IMM16 : NMD_X86_IMM32));
				else if (op == 0xEA || op == 0x9A) /* imm32,48 */
				{
					if (mode == NMD_X86_MODE_64)
						return false;
					instruction->imm_mask = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_IMM32 : NMD_X86_IMM48);
				}
				else if (op == 0xC2 || op == 0xCA) /* imm16 */
					instruction->imm_mask = NMD_X86_IMM16;
				else if (op == 0xC8) /* imm16 + imm8 */
					instruction->imm_mask = NMD_X86_IMM16 | NMD_X86_IMM8;

				/* Make sure we can read 'instruction->imm_mask' bytes from the buffer */
				if (buffer_size < instruction->imm_mask)
					return false;

				/* Copy 'instruction->imm_mask' bytes from the buffer */
				for (i = 0; i < instruction->imm_mask; i++)
					((uint8_t*)(&instruction->immediate))[i] = b[i];

				/* Increment the buffer and decrement the buffer's size */
				b += instruction->imm_mask;
				buffer_size -= instruction->imm_mask;

				/* Sign extend immediate for specific instructions */
				if (op == 0xe9 || op == 0xeb || op == 0xe8 || _NMD_R(op) == 7)
				{
					if (instruction->immediate & ((uint64_t)(1) << (instruction->imm_mask * 8 - 1)))
						instruction->immediate |= 0xffffffffffffffff << (instruction->imm_mask * 8);
				}
				else if (op == 0x68 && mode == NMD_X86_MODE_64 && instruction->immediate & ((uint64_t)1 << 31))
					instruction->immediate |= 0xffffffff00000000;

				/* These are optional features */
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					const bool opszprfx = instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE;
					if ((op >= 0x88 && op <= 0x8c) || (op >= 0xa0 && op <= 0xa3) || _NMD_R(op) == 0xb || op == 0x8e)
						instruction->id = NMD_X86_INSTRUCTION_MOV;
					else if (_NMD_R(op) == 5)
						instruction->id = (uint16_t)((_NMD_C(op) < 8) ? NMD_X86_INSTRUCTION_PUSH : NMD_X86_INSTRUCTION_POP);
					else if (_NMD_R(op) < 4 && (op % 8 < 6))
						instruction->id = (NMD_X86_INSTRUCTION_ADD + (_NMD_R(op) << 1) + (_NMD_C(op) >= 8 ? 1 : 0));
					else if (op >= 0x80 && op <= 0x84)
						instruction->id = NMD_X86_INSTRUCTION_ADD + modrm.fields.reg;
					else if (op == 0xe8)
						instruction->id = NMD_X86_INSTRUCTION_CALL;
					else if (op == 0xcc)
						instruction->id = NMD_X86_INSTRUCTION_INT3;
					else if (op == 0x8d)
						instruction->id = NMD_X86_INSTRUCTION_LEA;
					else if (_NMD_R(op) == 4)
						instruction->id = (uint16_t)((_NMD_C(op) < 8) ? NMD_X86_INSTRUCTION_INC : NMD_X86_INSTRUCTION_DEC);
					else if (_NMD_R(op) == 7)
						instruction->id = NMD_X86_INSTRUCTION_JO + _NMD_C(op);
					else if (op == 0xff)
						instruction->id = NMD_X86_INSTRUCTION_INC + modrm.fields.reg;
					else if (op == 0xeb || op == 0xe9)
						instruction->id = NMD_X86_INSTRUCTION_JMP;
					else if (op == 0x90)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							instruction->id = NMD_X86_INSTRUCTION_PAUSE;
						else if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							instruction->id = NMD_X86_INSTRUCTION_XCHG;
						else
							instruction->id = NMD_X86_INSTRUCTION_NOP;
					}
					else if (op == 0xc3 || op == 0xc2)
						instruction->id = NMD_X86_INSTRUCTION_RET;
					else if ((op >= 0x91 && op <= 0x97) || op == 0x86 || op == 0x87)
						instruction->id = NMD_X86_INSTRUCTION_XCHG;
					else if (op == 0xc0 || op == 0xc1 || (op >= 0xd0 && op <= 0xd3))
						instruction->id = NMD_X86_INSTRUCTION_ROL + modrm.fields.reg;
					else if (_NMD_R(op) == 0x0f && (op % 8 < 6))
						instruction->id = NMD_X86_INSTRUCTION_INT1 + (op - 0xf1);
					else if (op >= 0xd4 && op <= 0xd7)
						instruction->id = NMD_X86_INSTRUCTION_AAM + (op - 0xd4);
					else if (op >= 0xe0 && op <= 0xe3)
						instruction->id = NMD_X86_INSTRUCTION_LOOPNE + (op - 0xe0);
					else /* case 0x: instruction->id = NMD_X86_INSTRUCTION_; break; */
					{
						switch (op)
						{
						case 0x8f: instruction->id = NMD_X86_INSTRUCTION_POP; break;
						case 0xfe: instruction->id = (uint16_t)(modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_INC : NMD_X86_INSTRUCTION_DEC); break;
						case 0x84: case 0x85: case 0xa8: case 0xa9: instruction->id = NMD_X86_INSTRUCTION_TEST; break;
						case 0xf6: case 0xf7: instruction->id = NMD_X86_INSTRUCTION_TEST + modrm.fields.reg; break;
						case 0x69: case 0x6b: instruction->id = NMD_X86_INSTRUCTION_IMUL; break;
						case 0x9a: instruction->id = NMD_X86_INSTRUCTION_CALL; break;
						case 0x62: instruction->id = NMD_X86_INSTRUCTION_BOUND; break;
						case 0x63: instruction->id = (uint16_t)(mode == NMD_X86_MODE_64 ? NMD_X86_INSTRUCTION_MOVSXD : NMD_X86_INSTRUCTION_ARPL); break;
						case 0x68: case 0x6a: case 0x06: case 0x16: case 0x1e: case 0x0e: instruction->id = NMD_X86_INSTRUCTION_PUSH; break;
						case 0x6c: instruction->id = NMD_X86_INSTRUCTION_INSB; break;
						case 0x6d: instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_INSW : NMD_X86_INSTRUCTION_INSD); break;
						case 0x6e: instruction->id = NMD_X86_INSTRUCTION_OUTSB; break;
						case 0x6f: instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_OUTSW : NMD_X86_INSTRUCTION_OUTSD); break;
						case 0xc4: instruction->id = NMD_X86_INSTRUCTION_LES; break;
						case 0xc5: instruction->id = NMD_X86_INSTRUCTION_LDS; break;
						case 0xc6: case 0xc7: instruction->id = (uint16_t)(modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_MOV : (instruction->opcode == 0xc6 ? NMD_X86_INSTRUCTION_XABORT : NMD_X86_INSTRUCTION_XBEGIN)); break;
						case 0xc8: instruction->id = NMD_X86_INSTRUCTION_ENTER; break;
						case 0xc9: instruction->id = NMD_X86_INSTRUCTION_LEAVE; break;
						case 0xca: case 0xcb: instruction->id = NMD_X86_INSTRUCTION_RETF; break;
						case 0xcd: instruction->id = NMD_X86_INSTRUCTION_INT; break;
						case 0xce: instruction->id = NMD_X86_INSTRUCTION_INTO; break;
						case 0xcf:
							if (instruction->rex_w_prefix)
								instruction->id = NMD_X86_INSTRUCTION_IRETQ;
							else if (mode == NMD_X86_MODE_16)
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_IRETD : NMD_X86_INSTRUCTION_IRET);
							else
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_IRET : NMD_X86_INSTRUCTION_IRETD);
							break;
						case 0xe4: case 0xe5: case 0xec: case 0xed: instruction->id = NMD_X86_INSTRUCTION_IN; break;
						case 0xe6: case 0xe7: case 0xee: case 0xef: instruction->id = NMD_X86_INSTRUCTION_OUT; break;
						case 0xea: instruction->id = NMD_X86_INSTRUCTION_LJMP; break;

						case 0x9c:
							if (opszprfx)
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_PUSHFD : NMD_X86_INSTRUCTION_PUSHF);
							else
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_PUSHF : (mode == NMD_X86_MODE_32 ? NMD_X86_INSTRUCTION_PUSHFD : NMD_X86_INSTRUCTION_PUSHFQ));
							break;
						case 0x9d:
							if (opszprfx)
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_POPFD : NMD_X86_INSTRUCTION_POPF);
							else
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_POPF : (mode == NMD_X86_MODE_32 ? NMD_X86_INSTRUCTION_POPFD : NMD_X86_INSTRUCTION_POPFQ));
							break;
						case 0x60: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_INSTRUCTION_PUSHA, NMD_X86_INSTRUCTION_PUSHAD); break;
						case 0x61: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_INSTRUCTION_POPA, NMD_X86_INSTRUCTION_POPAD); break;
						case 0x07: case 0x17: case 0x1f: instruction->id = NMD_X86_INSTRUCTION_POP; break;
						case 0x27: instruction->id = NMD_X86_INSTRUCTION_DAA; break;
						case 0x37: instruction->id = NMD_X86_INSTRUCTION_AAA; break;
						case 0x2f: instruction->id = NMD_X86_INSTRUCTION_DAS; break;
						case 0x3f: instruction->id = NMD_X86_INSTRUCTION_AAS; break;
						case 0x9b: instruction->id = NMD_X86_INSTRUCTION_FWAIT; break;
						case 0x9e: instruction->id = NMD_X86_INSTRUCTION_SAHF; break;
						case 0x9f: instruction->id = NMD_X86_INSTRUCTION_LAHF; break;
						case 0xA4: instruction->id = NMD_X86_INSTRUCTION_MOVSB; break;
						case 0xA5: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_INSTRUCTION_MOVSW, NMD_X86_INSTRUCTION_MOVSD, NMD_X86_INSTRUCTION_MOVSQ); break; /*(uint16_t)(instruction->rex_w_prefix ? NMD_X86_INSTRUCTION_MOVSQ : (opszprfx ? NMD_X86_INSTRUCTION_MOVSW : NMD_X86_INSTRUCTION_MOVSD)); break;*/
						case 0xA6: instruction->id = NMD_X86_INSTRUCTION_CMPSB; break;
						case 0xA7: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_INSTRUCTION_CMPSW, NMD_X86_INSTRUCTION_CMPSD, NMD_X86_INSTRUCTION_CMPSQ); break;
						case 0xAA: instruction->id = NMD_X86_INSTRUCTION_STOSB; break;
						case 0xAB: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_INSTRUCTION_STOSW, NMD_X86_INSTRUCTION_STOSD, NMD_X86_INSTRUCTION_STOSQ); break;
						case 0xAC: instruction->id = NMD_X86_INSTRUCTION_LODSB; break;
						case 0xAD: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_INSTRUCTION_LODSW, NMD_X86_INSTRUCTION_LODSD, NMD_X86_INSTRUCTION_LODSQ); break;
						case 0xAE: instruction->id = NMD_X86_INSTRUCTION_SCASB; break;
						case 0xAF: instruction->id = _NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_INSTRUCTION_SCASW, NMD_X86_INSTRUCTION_SCASD, NMD_X86_INSTRUCTION_SCASQ); break;
						case 0x98:
							if (instruction->prefixes & NMD_X86_PREFIXES_REX_W)
								instruction->id = (uint16_t)NMD_X86_INSTRUCTION_CDQE;
							else if (instruction->mode == NMD_X86_MODE_16)
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_CWDE : NMD_X86_INSTRUCTION_CBW);
							else
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_CBW : NMD_X86_INSTRUCTION_CWDE);
							break;
						case 0x99:
							if (instruction->prefixes & NMD_X86_PREFIXES_REX_W)
								instruction->id = (uint16_t)NMD_X86_INSTRUCTION_CQO;
							else if (instruction->mode == NMD_X86_MODE_16)
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_CDQ : NMD_X86_INSTRUCTION_CWD);
							else
								instruction->id = (uint16_t)(opszprfx ? NMD_X86_INSTRUCTION_CWD : NMD_X86_INSTRUCTION_CDQ);
							break;

							/* Floating-point opcodes. */
#define _NMD_F_OP_GET_OFFSET() ((_NMD_R(modrm.modrm) - 0xc) << 1) + (_NMD_C(op) >= 8 ? 1 : 0)
						case 0xd8: instruction->id = (NMD_X86_INSTRUCTION_FADD + (modrm.fields.mod == 0b11 ? _NMD_F_OP_GET_OFFSET() : modrm.fields.reg)); break;
						case 0xd9:
							if (modrm.fields.mod == 0b11)
							{
								if (modrm.modrm <= 0xcf)
									instruction->id = (uint16_t)(modrm.modrm <= 0xc7 ? NMD_X86_INSTRUCTION_FLD : NMD_X86_INSTRUCTION_FXCH);
								else if (modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf)
									instruction->id = NMD_X86_INSTRUCTION_FSTPNCE;
								else if (modrm.modrm == 0xd0)
									instruction->id = NMD_X86_INSTRUCTION_FNOP;
								else
									instruction->id = NMD_X86_INSTRUCTION_FCHS + (modrm.modrm - 0xe0);
							}
							else
								instruction->id = NMD_X86_INSTRUCTION_FLD + modrm.fields.reg;
							break;
						case 0xda:
							if (modrm.fields.mod == 0b11)
								instruction->id = ((modrm.modrm == 0xe9) ? NMD_X86_INSTRUCTION_FUCOMPP : NMD_X86_INSTRUCTION_FCMOVB + _NMD_F_OP_GET_OFFSET());
							else
								instruction->id = NMD_X86_INSTRUCTION_FIADD + modrm.fields.reg;
							break;
						case 0xdb:
							if (modrm.fields.mod == 0b11)
								instruction->id = (modrm.modrm == 0xe2 ? NMD_X86_INSTRUCTION_FNCLEX : (modrm.modrm == 0xe2 ? NMD_X86_INSTRUCTION_FNINIT : NMD_X86_INSTRUCTION_FCMOVNB + _NMD_F_OP_GET_OFFSET()));
							else
								instruction->id = (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_FLD : (modrm.fields.reg == 0b111 ? NMD_X86_INSTRUCTION_FSTP : NMD_X86_INSTRUCTION_FILD + modrm.fields.reg));
							break;
						case 0xdc:
							if (modrm.fields.mod == 0b11)
								instruction->id = (NMD_X86_INSTRUCTION_FADD + ((_NMD_R(modrm.modrm) - 0xc) << 1) + ((_NMD_C(modrm.modrm) >= 8 && _NMD_R(modrm.modrm) <= 0xd) ? 1 : 0));
							else
								instruction->id = NMD_X86_INSTRUCTION_FADD + modrm.fields.reg;
							break;
						case 0xdd:
							if (modrm.fields.mod == 0b11)
							{
								switch ((modrm.modrm - 0xc0) >> 3)
								{
								case 0b000: instruction->id = NMD_X86_INSTRUCTION_FFREE; break;
								case 0b001: instruction->id = NMD_X86_INSTRUCTION_FXCH; break;
								case 0b010: instruction->id = NMD_X86_INSTRUCTION_FST; break;
								case 0b011: instruction->id = NMD_X86_INSTRUCTION_FSTP; break;
								case 0b100: instruction->id = NMD_X86_INSTRUCTION_FUCOM; break;
								case 0b101: instruction->id = NMD_X86_INSTRUCTION_FUCOMP; break;
								}
							}
							else
							{
								switch (modrm.fields.reg)
								{
								case 0b000: instruction->id = NMD_X86_INSTRUCTION_FLD; break;
								case 0b001: instruction->id = NMD_X86_INSTRUCTION_FISTTP; break;
								case 0b010: instruction->id = NMD_X86_INSTRUCTION_FST; break;
								case 0b011: instruction->id = NMD_X86_INSTRUCTION_FSTP; break;
								case 0b100: instruction->id = NMD_X86_INSTRUCTION_FRSTOR; break;
								case 0b110: instruction->id = NMD_X86_INSTRUCTION_FNSAVE; break;
								case 0b111: instruction->id = NMD_X86_INSTRUCTION_FNSTSW; break;
								}
							}
							break;
						case 0xde:
							if (modrm.fields.mod == 0b11)
								instruction->id = (modrm.modrm == 0xd9 ? NMD_X86_INSTRUCTION_FCOMPP : ((modrm.modrm >= 0xd0 && modrm.modrm <= 0xd7) ? NMD_X86_INSTRUCTION_FCOMP : NMD_X86_INSTRUCTION_FADDP + _NMD_F_OP_GET_OFFSET()));
							else
								instruction->id = NMD_X86_INSTRUCTION_FIADD + modrm.fields.reg;
							break;
						case 0xdf:
							if (modrm.fields.mod == 0b11)
							{
								if (modrm.fields.reg == 0b000)
									instruction->id = NMD_X86_INSTRUCTION_FFREEP;
								else if (modrm.fields.reg == 0b001)
									instruction->id = NMD_X86_INSTRUCTION_FXCH;
								else if (modrm.fields.reg <= 3)
									instruction->id = NMD_X86_INSTRUCTION_FSTP;
								else if (modrm.modrm == 0xe0)
									instruction->id = NMD_X86_INSTRUCTION_FNSTSW;
								else if (modrm.fields.reg == 0b110)
									instruction->id = NMD_X86_INSTRUCTION_FCOMIP;
								else
									instruction->id = NMD_X86_INSTRUCTION_FUCOMIP;
							}
							else
								instruction->id = (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_FILD : (modrm.fields.reg == 0b111 ? NMD_X86_INSTRUCTION_FISTP : (NMD_X86_INSTRUCTION_FILD + modrm.fields.reg)));
							break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
				if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
				{
					if (op == 0xcc || op == 0xcd) /* int3,int n */
					{
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_RF;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_VM;
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF;
					}
					else if (op == 0xce) /* into */
					{
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_RF;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_VM;
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC;
					}
					else if (_NMD_R(op) == 7) /* conditional jump */
						_nmd_decode_conditional_flag(instruction, _NMD_C(op));
					else if (_NMD_R(op) == 4 || ((op == 0xfe || op == 0xff) && modrm.fields.reg <= 0b001)) /* inc,dec */
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					else if (op <= 0x05 || (op >= 0x10 && op <= 0x15) || ((_NMD_R(op) == 1 || _NMD_R(op) == 2 || _NMD_R(op) == 3) && (_NMD_C(op) >= 0x8 && _NMD_C(op) <= 0x0d)) || ((op >= 0x80 && op <= 0x83) && (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b111)) || (op == 0xa6 || op == 0xa7) || (op == 0xae || op == 0xaf)) /* add,adc,sbb,sub,cmp, cmps,cmpsb,cmpsw,cmpsd,cmpsq, scas,scasb,scasw,scasd */
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF;
					else if (op == 0x9c) /* pushf,pushfd,pushfq */
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
					else if (op == 0x9d) /* popf,popfd,popfq */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_ID;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_VIP;
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_RF;
					}
					else if (op == 0xcf) /* iret,iretd,iretf */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_VM;
					}
					else if ((op >= 0x08 && op <= 0x0d) || ((_NMD_R(op) == 2 || _NMD_R(op) == 3) && _NMD_C(op) <= 5) || ((op >= 0x80 && op <= 0x83) && (modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110)) || (op == 0x84 || op == 0x85 || op == 0xa8 || op == 0xa9) || ((op == 0xf6 || op == 0xf7) && modrm.fields.reg == 0b000)) /* or,and,xor, test */
					{
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_CF;
						instruction->undefined_flags.eflags = NMD_X86_EFLAGS_AF;
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0x69 || op == 0x6b || ((op == 0xf6 || op == 0xf7) && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101))) /* mul,imul */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF;
						instruction->undefined_flags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0xf6 || op == 0xf7) /* Group 3 */
					{
						if (modrm.fields.reg == 0b011) /* neg */
							instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
						else if (modrm.fields.reg >= 0b110) /* div,idiv */
							instruction->undefined_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0xc0 || op == 0xc1 || (op >= 0xd0 && op <= 0xd3))
					{
						if (modrm.fields.reg <= 0b011) /* rol,ror,rcl,rcr */
						{
							instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF;
							instruction->undefined_flags.eflags = NMD_X86_EFLAGS_OF;
						}
						else /* shl,shr,sar */
						{
							instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_OF;
							instruction->undefined_flags.eflags = NMD_X86_EFLAGS_AF;
						}
					}
					else if (op == 0x27 || op == 0x2f) /* daa,das */
					{
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_AF;
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
						instruction->undefined_flags.eflags = NMD_X86_EFLAGS_OF;
					}
					else if (op == 0x37 || op == 0x3f) /* aaa,aas */
					{
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_AF;
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF;
						instruction->undefined_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0x63 && mode != NMD_X86_MODE_64) /* arpl */
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_ZF;
					else if (op == 0x9b) /* fwait,wait */
						instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
					else if (op == 0x9e) /* sahf */
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_CF;
					else if (op == 0x9f) /* lahf */
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_CF;
					else if (op == 0xd4 || op == 0xd5) /* aam,aad */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
						instruction->undefined_flags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF;
					}
					else if (op == 0xd6) /* salc */
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_CF;
					else if (op >= 0xd8 && op <= 0xdf) /* escape opcodes */
					{
						if (op == 0xd8 || op == 0xdc)
						{
							if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fadd,fmul,fsub,fsubr,fdiv,fdivr */
							{
								instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
								instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
							}
							else if (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011) /* fcom,fcomp */
							{
								instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
							}
						}
						else if (op == 0xd9)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011) /* fld,fst,fstp */
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.fields.reg == 0b100) /* fldenv */
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else if (modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fldcw,fstenv,fstcw */
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
							}
							else
							{
								if (modrm.modrm < 0xc8) /* fld */
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else /*if (modrm.modrm <= 0xcf)*/ /* fxch */
								{
									instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
						else if (op == 0xda || op == 0xde)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fiadd,fimul,fisub,fisubr,fidiv,fidivr */
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else /*if (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)*/ /* ficom,ficomp */
								{
									instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
							else
							{

								if ((op == 0xda && modrm.modrm == 0xe9) || (op == 0xde && modrm.modrm == 0xd9))
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
						else if (op == 0xdb || op == 0xdd || op == 0xdf)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b111) /* fild,fist,fistp,fld,fstp */
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.fields.reg == 0b001) /* fisttp */
								{
									instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
							else
							{
								if (modrm.fields.reg <= 0b011) /* fcmovnb,fcmovne,fcmovnbe,fcmovnu */
								{
									instruction->modified_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.modrm == 0xe0 || modrm.modrm == 0xe2) /* fstsw,fclex */
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else if (modrm.modrm == 0xe3) /* finit */
									instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else /* fucomi,fcomi */
								{
									instruction->cleared_flags.fpu_flags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefined_flags.fpu_flags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
					}
					else if (op == 0xf5) /* cmc */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_CF;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_CF;
					}
					else if (op == 0xf8) /* clc */
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_CF;
					else if (op == 0xf9) /* stc */
						instruction->set_flags.eflags = NMD_X86_EFLAGS_CF;
					else if (op == 0xfa || op == 0xfb) /* cli,sti */
					{
						instruction->modified_flags.eflags = NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_VIF;
						instruction->tested_flags.eflags = NMD_X86_EFLAGS_IOPL;
					}
					else if (op == 0xfc) /* cld */
						instruction->cleared_flags.eflags = NMD_X86_EFLAGS_DF;
					else if (op == 0xfd) /* std */
						instruction->set_flags.eflags = NMD_X86_EFLAGS_DF;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_GROUP
				/* Parse the instruction's group. */
				if (flags & NMD_X86_DECODER_FLAGS_GROUP)
				{
					if (_NMD_R(op) == 7 || op == 0xe3)
						instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0xe9 || op == 0xea || op == 0xeb || (op == 0xff && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101)))
						instruction->group = NMD_GROUP_JUMP | NMD_GROUP_UNCONDITIONAL_BRANCH | (op == 0xe9 || op == 0xeb ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
					else if (op == 0x9a || op == 0xe8 || (op == 0xff && (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)))
						instruction->group = NMD_GROUP_CALL | NMD_GROUP_UNCONDITIONAL_BRANCH | (op == 0xe8 ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
					else if (op == 0xc2 || op == 0xc3 || op == 0xca || op == 0xcb)
						instruction->group = NMD_GROUP_RET;
					else if ((op >= 0xcc && op <= 0xce) || op == 0xf1)
						instruction->group = NMD_GROUP_INT;
					else if (op == 0xf4)
						instruction->group = NMD_GROUP_PRIVILEGE;
					else if (op == 0xc7 && modrm.modrm == 0xf8)
						instruction->group = NMD_GROUP_UNCONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op >= 0xe0 && op <= 0xe2)
						instruction->group = NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0x8d && mode == NMD_X86_MODE_64)
						instruction->group = NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0xcf)
						instruction->group = NMD_GROUP_RET | NMD_GROUP_INT;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_GROUP */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					if (op >= 0xd8 && op <= 0xdf)
					{
						if (modrm.fields.mod == 0b11)
						{
							if ((op == 0xd9 && (_NMD_R(modrm.modrm) == 0xc || (op >= 0xc8 && op <= 0xcf))) ||
								(op == 0xda && _NMD_R(modrm.modrm) <= 0xd) ||
								(op == 0xdb && (_NMD_R(modrm.modrm) <= 0xd || modrm.modrm >= 0xe8)) ||
								(op == 0xde && modrm.modrm != 0xd9) ||
								(op == 0xdf && modrm.modrm != 0xe0))
								instruction->num_operands = 2;
						}
						else
							instruction->num_operands = 1;
					}
					else if ((_NMD_R(op) < 4 && op % 8 <= 5) || (_NMD_R(op) >= 8 && _NMD_R(op) <= 0xa && op != 0x8f && op != 0x90 && !(op >= 0x98 && op <= 0x9f)) || op == 0x62 || op == 0x63 || (op >= 0x6c && op <= 0x6f) || op == 0xc0 || op == 0xc1 || (op >= 0xc4 && op <= 0xc8) || (op >= 0xd0 && op <= 0xd3) || (_NMD_R(op) == 0xe && op % 8 >= 4))
						instruction->num_operands = 2;
					else if (_NMD_R(op) == 4 || op == 0x8f || op == 0x9a || op == 0xd4 || op == 0xd5 || (_NMD_R(op) == 0xe && op % 8 <= 3 && op != 0xe9))
						instruction->num_operands = 1;
					else if (op == 0x69 || op == 0x6b)
						instruction->num_operands = 3;

					const bool opszprfx = instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE;
					if (_NMD_R(op) == 0xb) /* mov reg,imm */
					{
						instruction->num_operands = 2;
						_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_WRITE, (op < 0xb8 ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8B : NMD_X86_REG_AL) : (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8D : NMD_X86_REG_EAX))) + op % 8);
						_NMD_SET_IMM_OPERAND(instruction->operands[1], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
					}
					else if (_NMD_R(op) == 5) /* push reg,pop reg */
					{
						instruction->num_operands = 3;
						_NMD_SET_REG_OPERAND(instruction->operands[0], false, _NMD_C(op) < 8 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE, (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? (opszprfx ? NMD_X86_REG_R8W : NMD_X86_REG_R8) : (opszprfx ? (instruction->mode == NMD_X86_MODE_16 ? NMD_X86_REG_EAX : NMD_X86_REG_AX) : (NMD_X86_REG_AX + (instruction->mode >> 2) * 8))) + (op % 8));
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, _NMD_C(op) < 8 ? NMD_X86_OPERAND_ACTION_WRITE : NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (_NMD_R(op) == 7) /* jCC */
					{
						instruction->num_operands = 2;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_IP());
					}
					else if (op == 0xe9) /* jmp rel16,rel32 */
					{
						instruction->num_operands = 2;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_IP());
					}
					else if (op == 0x6a || op == 0x68) /* push imm8,push imm32/imm16 */
					{
						instruction->num_operands = 3;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (_NMD_R(op) == 4) /* inc,dec*/
					{
						instruction->num_operands = 1;
						_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_AX, NMD_X86_REG_EAX) + (op % 8));
					}
					else if (op == 0xcc || op == 0xf1 || op == 0xce) /* int3,int1,into */
					{
						instruction->num_operands = 1;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
					}
					else if (op == 0xcd) /* int n */
					{
						instruction->num_operands = 2;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
					}
					else if (op == 0xe8) /* call rel32 */
					{
						instruction->num_operands = 4;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_IP());
						_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (_NMD_R(op) < 4 && _NMD_C(op) < 6) /* add,adc,and,xor,or,sbb,sub,cmp Eb,Gb / Ev,Gv / Gb,Eb / Gv,Ev / AL,lb / rAX,lz */
					{
						/*
						if (op % 8 == 0)
						{
							if (modrm.mod == 0b11)
							{

							}
							else
							{
								_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
							}
						}
						*/
						if (op % 8 == 0 || op % 8 == 2)
						{
							_nmd_decode_operand_Eb(instruction, &instruction->operands[op % 8 == 0 ? 0 : 1]);
							_nmd_decode_operand_Gb(instruction, &instruction->operands[op % 8 == 0 ? 1 : 0]);
						}
						else if (op % 8 == 1 || op % 8 == 3)
						{
							_nmd_decode_operand_Ev(instruction, &instruction->operands[op % 8 == 1 ? 0 : 1]);
							_nmd_decode_operand_Gv(instruction, &instruction->operands[op % 8 == 1 ? 1 : 0]);
						}
						else if (op % 8 == 4 || op % 8 == 5)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							if (op % 8 == 4)
								instruction->operands[0].fields.reg = NMD_X86_REG_AL;
							else
								instruction->operands[0].fields.reg = (uint8_t)(instruction->rex_w_prefix ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));

							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						}

						instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						if (!(_NMD_R(op) == 3 && _NMD_C(op) >= 8))
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					}
					else if (op == 0xc3 || op == 0xcb || op == 0xcf) /* ret,retf,iret,iretd,iretf */
					{
						instruction->num_operands = 3;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0xc2 || op == 0xca) /* ret imm16,retf imm16 */
					{
						instruction->num_operands = 4;
						_NMD_SET_IMM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, _NMD_GET_IP());
						_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0xff && modrm.fields.reg == 6) /* push mem */
					{
						instruction->num_operands = 3;
						_NMD_SET_MEM_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_DS, (instruction->prefixes& NMD_X86_PREFIXES_REX_B ? (opszprfx ? NMD_X86_REG_R8W : NMD_X86_REG_R8) : (opszprfx ? (instruction->mode == NMD_X86_MODE_16 ? NMD_X86_REG_EAX : NMD_X86_REG_AX) : (NMD_X86_REG_AX + (instruction->mode >> 2) * 8))) + modrm.fields.rm, NMD_X86_REG_NONE, 0, 0);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);

					}
					else if (op == 0x9c) /* pushf,pushfd,pushfq */
					{
						instruction->num_operands = 2;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x9d) /* popf,popfd,popfq */
					{
						instruction->num_operands = 2;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0xc9) /* leave */
					{
						instruction->num_operands = 3;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_BP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x06 || op == 0x16 || op == 0x0e || op == 0x1e) /* push es,push ss,push ds,push cs */
					{
						instruction->num_operands = 3;
						_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_READ, op == 0x06 ? NMD_X86_REG_ES : (op == 0x16 ? NMD_X86_REG_SS : (op == 0x1e ? NMD_X86_REG_DS : NMD_X86_REG_CS)));
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x07 || op == 0x17 || op == 0x1f) /* pop es,pop ss,pop ds */
					{
						instruction->num_operands = 3;
						_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_WRITE, op == 0x07 ? NMD_X86_REG_ES : (op == 0x17 ? NMD_X86_REG_SS : NMD_X86_REG_DS));
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_GPR(NMD_X86_REG_SP));
						_NMD_SET_MEM_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_GPR(NMD_X86_REG_SP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x60) /* pusha,pushad */
					{
						instruction->num_operands = 10;
						const uint32_t base_reg = _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_AX, NMD_X86_REG_EAX);
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 0);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 1);
						_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 2);
						_NMD_SET_REG_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 3);
						_NMD_SET_REG_OPERAND(instruction->operands[4], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 4);
						_NMD_SET_REG_OPERAND(instruction->operands[5], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 5);
						_NMD_SET_REG_OPERAND(instruction->operands[6], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 6);
						_NMD_SET_REG_OPERAND(instruction->operands[7], true, NMD_X86_OPERAND_ACTION_READ, base_reg + 7);
						_NMD_SET_REG_OPERAND(instruction->operands[8], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_SP, NMD_X86_REG_ESP));
						_NMD_SET_MEM_OPERAND(instruction->operands[9], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_SS, _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_SP, NMD_X86_REG_ESP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x61) /* popa,popad */
					{
						instruction->num_operands = 10;
						const uint32_t base_reg = _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_AX, NMD_X86_REG_EAX);
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 0);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 1);
						_NMD_SET_REG_OPERAND(instruction->operands[2], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 2);
						_NMD_SET_REG_OPERAND(instruction->operands[3], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 3);
						_NMD_SET_REG_OPERAND(instruction->operands[4], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 4);
						_NMD_SET_REG_OPERAND(instruction->operands[5], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 5);
						_NMD_SET_REG_OPERAND(instruction->operands[6], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 6);
						_NMD_SET_REG_OPERAND(instruction->operands[7], true, NMD_X86_OPERAND_ACTION_WRITE, base_reg + 7);
						_NMD_SET_REG_OPERAND(instruction->operands[8], true, NMD_X86_OPERAND_ACTION_READWRITE, _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_SP, NMD_X86_REG_ESP));
						_NMD_SET_MEM_OPERAND(instruction->operands[9], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_SS, _NMD_GET_BY_MODE_OPSZPRFX(mode, opszprfx, NMD_X86_REG_SP, NMD_X86_REG_ESP), NMD_X86_REG_NONE, 0, 0);
					}
					else if (op == 0x27 || op == 0x2f) /* daa,das */
					{
						instruction->num_operands = 1;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, NMD_X86_REG_AL);
					}
					else if (op == 0x37 || op == 0x3f) /* aaa,aas */
					{
						instruction->num_operands = 2;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_READWRITE, NMD_X86_REG_AL);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READWRITE, NMD_X86_REG_AH);
					}
					else if (op == 0xd7) /* xlat */
					{
						instruction->num_operands = 2;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_AL);
						_NMD_SET_MEM_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, NMD_X86_REG_DS, _NMD_GET_GPR(NMD_X86_REG_BX), NMD_X86_REG_AL, 1, 0);
					}
					else if (op == 0x9e || op == 0x9f) /* sahf,lahf */
					{
						instruction->num_operands = 1;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, op == 0x9e ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_AH);
					}
					else if (op == 0x98) /* cbw,cwde,cdqe */
					{
						instruction->num_operands = 2;
						const NMD_X86_REG reg = instruction->mode == NMD_X86_MODE_64 && instruction->rex_w_prefix ? NMD_X86_REG_RAX : (((instruction->mode == NMD_X86_MODE_16 && instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) || (instruction->mode != NMD_X86_MODE_16 && !(instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))) ? NMD_X86_REG_EAX : NMD_X86_REG_AX);
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, reg);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, reg - 8);
					}
					else if (op == 0x99) /* cwd,cdq,cqo */
					{
						instruction->num_operands = 2;
						const NMD_X86_REG reg = instruction->mode == NMD_X86_MODE_64 && instruction->rex_w_prefix ? NMD_X86_REG_RAX : (((instruction->mode == NMD_X86_MODE_16 && instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) || (instruction->mode != NMD_X86_MODE_16 && !(instruction->simd_prefix & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))) ? NMD_X86_REG_EAX : NMD_X86_REG_AX);
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, reg + 2);
						_NMD_SET_REG_OPERAND(instruction->operands[1], true, NMD_X86_OPERAND_ACTION_READ, reg);
					}
					else if (op == 0xd6) /* salc */
					{
						instruction->num_operands = 1;
						_NMD_SET_REG_OPERAND(instruction->operands[0], true, NMD_X86_OPERAND_ACTION_WRITE, NMD_X86_REG_AL);
					}
					else if (op == 0xc7 && modrm.fields.reg == 0b000) /* mov Ev,lz */
					{
						instruction->num_operands = 2;
						if (modrm.fields.mod == 0b11)
						{
							const NMD_X86_REG reg = (NMD_X86_REG)(_NMD_GET_BY_MODE_OPSZPRFX_W64(mode, opszprfx, instruction->rex_w_prefix, NMD_X86_REG_AX, NMD_X86_REG_EAX, NMD_X86_REG_RAX) + modrm.fields.rm);
							_NMD_SET_REG_OPERAND(instruction->operands[0], false, NMD_X86_OPERAND_ACTION_WRITE, reg);
						}
						else
						{
							_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);
							instruction->operands[0].is_implicit = false;
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						}
						_NMD_SET_IMM_OPERAND(instruction->operands[1], false, NMD_X86_OPERAND_ACTION_READ, instruction->immediate);
					}
					else if (op >= 0x84 && op <= 0x8b)
					{
						if (op % 2 == 0)
						{
							_nmd_decode_operand_Eb(instruction, &instruction->operands[op == 0x8a ? 1 : 0]);
							_nmd_decode_operand_Gb(instruction, &instruction->operands[op == 0x8a ? 0 : 1]);
						}
						else
						{
							_nmd_decode_operand_Ev(instruction, &instruction->operands[op == 0x8b ? 1 : 0]);
							_nmd_decode_operand_Gv(instruction, &instruction->operands[op == 0x8b ? 0 : 1]);
						}

						if (op >= 0x88)
						{
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else if (op >= 0x86)
							instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READWRITE;
					}
					else if (op >= 0x80 && op <= 0x83)
					{
						if (op % 2 == 0)
							_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
						else
							_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (_NMD_R(op) == 7 || op == 0x9a || op == 0xcd || op == 0xd4 || op == 0xd5)
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					else if (op == 0x90 && instruction->prefixes & NMD_X86_PREFIXES_REX_B)
					{
						instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_R8 : NMD_X86_REG_R8D);
						instruction->operands[1].fields.reg = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_RAX : NMD_X86_REG_EAX);
					}
					else if (_NMD_R(op) == 5)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : (mode == NMD_X86_MODE_64 ? NMD_X86_REG_RAX : NMD_X86_REG_EAX)) + (op % 8));
						instruction->operands[0].action = (uint8_t)(_NMD_C(op) < 8 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
					}
					else if (op == 0x62)
					{
						_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[1]);
						instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x63)
					{
						if (mode == NMD_X86_MODE_64)
						{
							_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
							_nmd_decode_operand_Ev(instruction, &instruction->operands[1]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else
						{
							if (instruction->modrm.fields.mod == 0b11)
							{
								instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[0].fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.rm;
							}
							else
								_nmd_decode_modrm_upper32(instruction, &instruction->operands[0]);

							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[1].fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.reg;
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
					}
					else if (op == 0x69 || op == 0x6b)
					{
						_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
						_nmd_decode_operand_Ev(instruction, &instruction->operands[1]);
						instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						instruction->operands[2].fields.imm = (int64_t)(instruction->immediate);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = instruction->operands[2].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x8c)
					{
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[1].fields.reg = NMD_X86_REG_ES + instruction->modrm.fields.reg;
					}
					else if (op == 0x8d)
					{
						_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[1]);
					}
					else if (op == 0x8e)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = NMD_X86_REG_ES + instruction->modrm.fields.reg;
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						_nmd_decode_operand_Ew(instruction, &instruction->operands[1]);
					}
					else if (op == 0x8f)
					{
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					}
					else if (op >= 0x91 && op <= 0x97)
					{
						_nmd_decode_operand_Gv(instruction, &instruction->operands[0]);
						instruction->operands[0].fields.reg = instruction->operands[0].fields.reg + _NMD_C(op);
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[1].fields.reg = (uint8_t)(instruction->rex_w_prefix ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16 ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
						instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READWRITE;
					}
					else if (op >= 0xa0 && op <= 0xa3)
					{
						instruction->operands[op < 0xa2 ? 0 : 1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[op < 0xa2 ? 0 : 1].fields.reg = (uint8_t)(op % 2 == 0 ? NMD_X86_REG_AL : (instruction->rex_w_prefix ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16) || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)));
						instruction->operands[op < 0xa2 ? 1 : 0].type = NMD_X86_OPERAND_TYPE_MEMORY;

						/* FIXME: We should not access the buffer from here
						instruction->operands[op < 0xa2 ? 1 : 0].fields.mem.disp = (mode == NMD_X86_MODE_64) ? *(uint64_t*)(b + 1) : *(uint32_t*)(b + 1);
						*/

						_nmd_decode_operand_segment_reg(instruction, &instruction->operands[op < 0xa2 ? 1 : 0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xa8 || op == 0xa9)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)(op == 0xa8 ? NMD_X86_REG_AL : (instruction->rex_w_prefix ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16) || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)));
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (op == 0xc0 || op == 0xc1 || op == 0xc6)
					{
						if (!(op >= 0xc6 && instruction->modrm.fields.reg))
						{
							if (op % 2 == 0)
								_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
							else
								_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						}
						instruction->operands[op >= 0xc6 && instruction->modrm.fields.reg ? 0 : 1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						instruction->operands[0].action = (uint8_t)(op <= 0xc1 ? NMD_X86_OPERAND_ACTION_READWRITE : NMD_X86_OPERAND_ACTION_WRITE);
					}
					else if (op == 0xc4 || op == 0xc5)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);
						_nmd_decode_modrm_upper32(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xc8)
					{
						instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						/* FIXME: We should not access the buffer from here
						instruction->operands[0].fields.imm = *(uint16_t*)(b + 1);
						instruction->operands[1].fields.imm = b[3];
						*/
					}
					else if (op >= 0xd0 && op <= 0xd3)
					{
						if (op % 2 == 0)
							_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
						else
							_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);

						if (op < 0xd2)
						{
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[1].fields.imm = 1;
						}
						else
						{
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[1].fields.reg = NMD_X86_REG_CL;
						}
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READWRITE;
					}
					else if (op >= 0xd8 && op <= 0xdf)
					{
						if (instruction->modrm.fields.mod != 0b11 ||
							op == 0xd8 ||
							(op == 0xd9 && _NMD_C(instruction->modrm.modrm) == 0xc) ||
							(op == 0xda && _NMD_C(instruction->modrm.modrm) <= 0xd) ||
							(op == 0xdb && (_NMD_C(instruction->modrm.modrm) <= 0xd || instruction->modrm.modrm >= 0xe8)) ||
							op == 0xdc ||
							op == 0xdd ||
							(op == 0xde && instruction->modrm.modrm != 0xd9) ||
							(op == 0xdf && instruction->modrm.modrm != 0xe0))
						{
							instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].is_implicit = true;
							instruction->operands[0].fields.reg = NMD_X86_REG_ST0;
							instruction->operands[1].fields.reg = NMD_X86_REG_ST0 + instruction->modrm.fields.reg;
						}
					}
					else if (_NMD_R(op) == 0xe)
					{
						if (op % 8 < 4)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[0].fields.imm = (int64_t)(instruction->immediate);
						}
						else
						{
							if (op < 0xe8)
							{
								instruction->operands[0].type = (uint8_t)(_NMD_C(op) < 6 ? NMD_X86_OPERAND_TYPE_REGISTER : NMD_X86_OPERAND_TYPE_IMMEDIATE);
								instruction->operands[1].type = (uint8_t)(_NMD_C(op) < 6 ? NMD_X86_OPERAND_TYPE_IMMEDIATE : NMD_X86_OPERAND_TYPE_REGISTER);
								instruction->operands[0].fields.imm = instruction->operands[1].fields.imm = (int64_t)(instruction->immediate);
							}
							else
							{
								instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[0].fields.reg = instruction->operands[1].fields.reg = NMD_X86_REG_DX;
							}

							if (op % 2 == 0)
								instruction->operands[op % 8 == 4 ? 0 : 1].fields.reg = NMD_X86_REG_AL;
							else
								instruction->operands[op % 8 == 5 ? 0 : 1].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);

							instruction->operands[op % 8 <= 5 ? 0 : 1].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[op % 8 <= 5 ? 1 : 0].action = NMD_X86_OPERAND_ACTION_READ;
						}
					}
					else if (op == 0xf6 || op == 0xfe)
					{
						_nmd_decode_operand_Eb(instruction, &instruction->operands[0]);
						instruction->operands[0].action = (uint8_t)(op == 0xfe && instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READWRITE);
					}
					else if (op == 0xf7 || op == 0xff)
					{
						_nmd_decode_operand_Ev(instruction, &instruction->operands[0]);
						instruction->operands[0].action = (uint8_t)(op == 0xff && instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READWRITE);
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
	}

	if (instruction->prefixes & NMD_X86_PREFIXES_LOCK)
	{
		if (!(instruction->has_modrm && instruction->modrm.fields.mod != 0b11 &&
			((instruction->opcode_size == 1 && (op == 0x86 || op == 0x87 || (_NMD_R(op) < 4 && (op % 8) < 2 && op < 0x38) || ((op >= 0x80 && op <= 0x83) && instruction->modrm.fields.reg != 0b111) || (op >= 0xfe && instruction->modrm.fields.reg < 2) || ((op == 0xf6 || op == 0xf7) && (instruction->modrm.fields.reg == 0b010 || instruction->modrm.fields.reg == 0b011)))) ||
			(instruction->opcode_size == 2 && (_nmd_find_byte(_nmd_two_opcodes, sizeof(_nmd_two_opcodes), op) || op == 0xab || (op == 0xba && instruction->modrm.fields.reg != 0b100) || (op == 0xc7 && instruction->modrm.fields.reg == 0b001))))))
			return false;
	}

	instruction->length = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));
	for (i = 0; i < instruction->length; i++)
		instruction->buffer[i] = ((const uint8_t* const)(buffer))[i];

	instruction->valid = true;

	return true;
}

NMD_ASSEMBLY_API bool _nmd_ldisasm_decode_modrm(const uint8_t** p_buffer, size_t* p_buffer_size, bool address_prefix, NMD_X86_MODE mode, nmd_x86_modrm* p_modrm)
{
	_NMD_READ_BYTE(*p_buffer, *p_buffer_size, (*p_modrm).modrm);

	bool has_sib = false;
	size_t disp_size = 0;

	if (mode == NMD_X86_MODE_16)
	{
		if (p_modrm->fields.mod != 0b11)
		{
			if (p_modrm->fields.mod == 0b00)
			{
				if (p_modrm->fields.rm == 0b110)
					disp_size = 2;
			}
			else
				disp_size = p_modrm->fields.mod == 0b01 ? 1 : 2;
		}
	}
	else
	{
		if (address_prefix && mode == NMD_X86_MODE_32)
		{
			if ((p_modrm->fields.mod == 0b00 && p_modrm->fields.rm == 0b110) || p_modrm->fields.mod == 0b10)
				disp_size = 2;
			else if (p_modrm->fields.mod == 0b01)
				disp_size = 1;
		}
		else
		{
			/* Check for SIB byte */
			uint8_t sib = 0;
			if (p_modrm->modrm < 0xC0 && p_modrm->fields.rm == 0b100 && (!address_prefix || (address_prefix && mode == NMD_X86_MODE_64)))
			{
				has_sib = true;
				_NMD_READ_BYTE(*p_buffer, *p_buffer_size, sib);
			}

			if (p_modrm->fields.mod == 0b01) /* disp8 (ModR/M) */
				disp_size = 1;
			else if ((p_modrm->fields.mod == 0b00 && p_modrm->fields.rm == 0b101) || p_modrm->fields.mod == 0b10) /* disp16,32 (ModR/M) */
				disp_size = (address_prefix && !(mode == NMD_X86_MODE_64 && address_prefix) ? 2 : 4);
			else if (has_sib && (sib & 0b111) == 0b101) /* disp8,32 (SIB) */
				disp_size = (p_modrm->fields.mod == 0b01 ? 1 : 4);
		}
	}

	/* Make sure we can read 'instruction->disp_mask' bytes from the buffer */
	if (*p_buffer_size < disp_size)
		return false;

	/* Increment the buffer and decrement the buffer's size */
	*p_buffer += disp_size;
	*p_buffer_size -= disp_size;

	return true;
}

/*
Returns the length of the instruction if it is valid, zero otherwise.
Parameters:
 - buffer      [in] A pointer to a buffer containing an encoded instruction.
 - buffer_size [in] The size of the buffer in bytes.
 - mode        [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
*/
NMD_ASSEMBLY_API size_t nmd_x86_ldisasm(const void* const buffer, size_t buffer_size, const NMD_X86_MODE mode)
{
	bool operand_prefix = false;
	bool address_prefix = false;
	bool repeat_prefix = false;
	bool repeat_not_zero_prefix = false;
	bool rexW = false;
	bool lock_prefix = false;
	uint16_t simd_prefix = NMD_X86_PREFIXES_NONE;
	uint8_t opcode_size = 0;
	bool has_modrm = false;
	nmd_x86_modrm modrm;
	modrm.modrm = 0;

	/* Security considerations for memory safety:
	The contents of 'buffer' should be considered untrusted and decoded carefully.

	'buffer' should always point to the start of the buffer. We use the 'b'
	buffer iterator to read data from the buffer, however before accessing it
	make sure to check 'buffer_size' to see if we can safely access it. Then,
	after reading data from the buffer we increment 'b' and decrement 'buffer_size'.
	Helper macros: _NMD_READ_BYTE()
	*/

	/* Set buffer iterator */
	const uint8_t* b = (const uint8_t*)buffer;

	/*  Clamp 'buffer_size' to 15. We will only read up to 15 bytes(NMD_X86_MAXIMUM_INSTRUCTION_LENGTH) */
	if (buffer_size > 15)
		buffer_size = 15;

	/* Decode legacy and REX prefixes */
	for (; buffer_size > 0; b++, buffer_size--)
	{
		switch (*b)
		{
		case 0xF0: lock_prefix = true; continue;
		case 0xF2: repeat_not_zero_prefix = true, simd_prefix = NMD_X86_PREFIXES_REPEAT_NOT_ZERO; continue;
		case 0xF3: repeat_prefix = true, simd_prefix = NMD_X86_PREFIXES_REPEAT; continue;
		case 0x2E: continue;
		case 0x36: continue;
		case 0x3E: continue;
		case 0x26: continue;
		case 0x64: continue;
		case 0x65: continue;
		case 0x66: operand_prefix = true, simd_prefix = NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE; continue;
		case 0x67: address_prefix = true; continue;
		default:
			if (mode == NMD_X86_MODE_64 && _NMD_R(*b) == 4) /* REX prefixes [0x40,0x4f] */
			{
				if (_NMD_C(*b) & 0b1000)
					rexW = true;
				continue;
			}
		}

		break;
	}

	/* Calculate the number of prefixes based on how much the iterator moved */
	const size_t num_prefixes = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));

	/* Opcode byte. This variable is used because 'op' is simpler than 'instruction->opcode' */
	uint8_t op;
	_NMD_READ_BYTE(b, buffer_size, op);

	if (op == 0x0F) /* 2 or 3 byte opcode */
	{
		_NMD_READ_BYTE(b, buffer_size, op);

		if (op == 0x38 || op == 0x3A) /* 3 byte opcode */
		{
			const bool is_opcode_map38 = op == 0x38;
			opcode_size = 3;

			_NMD_READ_BYTE(b, buffer_size, op);

			if (!_nmd_ldisasm_decode_modrm(&b, &buffer_size, address_prefix, mode, &modrm))
				return 0;
			has_modrm = true;

			if (is_opcode_map38)
			{
#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
				if (op == 0x36)
				{
					return 0;
				}
				else if (op <= 0xb || (op >= 0x1c && op <= 0x1e))
				{
					if (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return 0;
				}
				else if (op >= 0xc8 && op <= 0xcd)
				{
					if (simd_prefix)
						return 0;
				}
				else if (op == 0x10 || op == 0x14 || op == 0x15 || op == 0x17 || (op >= 0x20 && op <= 0x25) || op == 0x28 || op == 0x29 || op == 0x2b || _NMD_R(op) == 3 || op == 0x40 || op == 0x41 || op == 0xcf || (op >= 0xdb && op <= 0xdf))
				{
					if (simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0x2a || (op >= 0x80 && op <= 0x82))
				{
					if (modrm.fields.mod == 0b11 || simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0xf0 || op == 0xf1)
				{
					if (modrm.fields.mod == 0b11 && (simd_prefix == NMD_X86_PREFIXES_NONE || simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
						return 0;
					else if (simd_prefix == NMD_X86_PREFIXES_REPEAT)
						return 0;
				}
				else if (op == 0xf5 || op == 0xf8)
				{
					if (simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || modrm.fields.mod == 0b11)
						return 0;
				}
				else if (op == 0xf6)
				{
					if (simd_prefix == NMD_X86_PREFIXES_NONE && modrm.fields.mod == 0b11)
						return 0;
					else if (simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return 0;
				}
				else if (op == 0xf9)
				{
					if (simd_prefix != NMD_X86_PREFIXES_NONE || modrm.fields.mod == 0b11)
						return 0;
				}
				else
					return 0;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
			}
			else /* 0x3a */
			{
				/* "Read" the immediate byte */
				uint8_t imm;
				_NMD_READ_BYTE(b, buffer_size, imm);

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
				if ((op >= 0x8 && op <= 0xe) || (op >= 0x14 && op <= 0x17) || (op >= 0x20 && op <= 0x22) || (op >= 0x40 && op <= 0x42) || op == 0x44 || (op >= 0x60 && op <= 0x63) || op == 0xdf || op == 0xce || op == 0xcf)
				{
					if (simd_prefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0x0f || op == 0xcc)
				{
					if (simd_prefix)
						return 0;
				}
				else
					return 0;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
			}
		}
		else if (op == 0x0f) /* 3DNow! opcode map*/
		{
#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW
			if (!_nmd_ldisasm_decode_modrm(&b, &buffer_size, address_prefix, mode, &modrm))
				return false;

			uint8_t imm;
			_NMD_READ_BYTE(b, buffer_size, imm);

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
			if (!_nmd_find_byte(_nmd_valid_3DNow_opcodes, sizeof(_nmd_valid_3DNow_opcodes), imm))
				return false;
#endif /*NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
#else /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW */
			return false;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW */
		}
		else /* 2 byte opcode. */
		{
			opcode_size = 2;

			/* Check for ModR/M, SIB and displacement */
			if (op >= 0x20 && op <= 0x23)
			{
				has_modrm = true;
				_NMD_READ_BYTE(b, buffer_size, modrm.modrm);
			}
			else if (op < 4 || (_NMD_R(op) != 3 && _NMD_R(op) > 0 && _NMD_R(op) < 7) || (op >= 0xD0 && op != 0xFF) || (_NMD_R(op) == 7 && _NMD_C(op) != 7) || _NMD_R(op) == 9 || _NMD_R(op) == 0xB || (_NMD_R(op) == 0xC && _NMD_C(op) < 8) || (_NMD_R(op) == 0xA && (op % 8) >= 3) || op == 0x0ff || op == 0x00 || op == 0x0d)
			{
				if (!_nmd_ldisasm_decode_modrm(&b, &buffer_size, address_prefix, mode, &modrm))
					return 0;
				has_modrm = true;
			}

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
			if (_nmd_find_byte(_nmd_invalid_op2, sizeof(_nmd_invalid_op2), op))
				return 0;
			else if (op == 0xc7)
			{
				if ((!simd_prefix && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= 0b101 : modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010)) || (simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (modrm.fields.mod == 0b11 || modrm.fields.reg != 0b001)) || ((simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simd_prefix == NMD_X86_PREFIXES_REPEAT) && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= (simd_prefix == NMD_X86_PREFIXES_REPEAT ? 0b110 : 0b101) : (modrm.fields.reg != 0b001 && modrm.fields.reg != 0b110))))
					return 0;
			}
			else if (op == 0x00)
			{
				if (modrm.fields.reg >= 0b110)
					return 0;
			}
			else if (op == 0x01)
			{
				if ((modrm.fields.mod == 0b11 ? (((simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || simd_prefix == NMD_X86_PREFIXES_REPEAT) && ((modrm.modrm >= 0xc0 && modrm.modrm <= 0xc5) || (modrm.modrm >= 0xc8 && modrm.modrm <= 0xcb) || (modrm.modrm >= 0xcf && modrm.modrm <= 0xd1) || (modrm.modrm >= 0xd4 && modrm.modrm <= 0xd7) || modrm.modrm == 0xee || modrm.modrm == 0xef || modrm.modrm == 0xfa || modrm.modrm == 0xfb)) || (modrm.fields.reg == 0b000 && modrm.fields.rm >= 0b110) || (modrm.fields.reg == 0b001 && modrm.fields.rm >= 0b100 && modrm.fields.rm <= 0b110) || (modrm.fields.reg == 0b010 && (modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011)) || (modrm.fields.reg == 0b101 && modrm.fields.rm < 0b110 && (!repeat_prefix || (simd_prefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.rm != 0b000 && modrm.fields.rm != 0b010)))) || (modrm.fields.reg == 0b111 && (modrm.fields.rm > 0b101 || (mode != NMD_X86_MODE_64 && modrm.fields.rm == 0b000)))) : (!repeat_prefix && modrm.fields.reg == 0b101)))
					return 0;
			}
			else if (op == 0x1A || op == 0x1B)
			{
				if (modrm.fields.mod == 0b11)
					return 0;
			}
			else if (op == 0x20 || op == 0x22)
			{
				if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b101)
					return 0;
			}
			else if (op >= 0x24 && op <= 0x27)
				return 0;
			else if (op >= 0x3b && op <= 0x3f)
				return 0;
			else if (_NMD_R(op) == 5)
			{
				if ((op == 0x50 && modrm.fields.mod != 0b11) || (simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x52 || op == 0x53)) || (simd_prefix == NMD_X86_PREFIXES_REPEAT && (op == 0x50 || (op >= 0x54 && op <= 0x57))) || (repeat_not_zero_prefix && (op == 0x50 || (op >= 0x52 && op <= 0x57) || op == 0x5b)))
					return 0;
			}
			else if (_NMD_R(op) == 6)
			{
				if ((!(simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (op == 0x6c || op == 0x6d)) || (simd_prefix == NMD_X86_PREFIXES_REPEAT && op != 0x6f) || repeat_not_zero_prefix)
					return 0;
			}
			else if (op == 0x78 || op == 0x79)
			{
				if ((((simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && op == 0x78) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b000)) || ((simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && modrm.fields.mod != 0b11)) || (simd_prefix == NMD_X86_PREFIXES_REPEAT))
					return 0;
			}
			else if (op == 0x7c || op == 0x7d)
			{
				if (simd_prefix == NMD_X86_PREFIXES_REPEAT || !(simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
					return 0;
			}
			else if (op == 0x7e || op == 0x7f)
			{
				if (repeat_not_zero_prefix)
					return 0;
			}
			else if (op >= 0x71 && op <= 0x73)
			{
				if ((simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || modrm.modrm <= 0xcf || (modrm.modrm >= 0xe8 && modrm.modrm <= 0xef))
					return 0;
			}
			else if (op == 0x73)
			{
				if (modrm.modrm >= 0xe0 && modrm.modrm <= 0xe8)
					return 0;
			}
			else if (op == 0xa6)
			{
				if (modrm.modrm != 0xc0 && modrm.modrm != 0xc8 && modrm.modrm != 0xd0)
					return 0;
			}
			else if (op == 0xa7)
			{
				if (!(modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b101 && modrm.fields.rm == 0b000))
					return 0;
			}
			else if (op == 0xae)
			{
				if (((!simd_prefix && modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b100) || (simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b110)) || (simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (modrm.fields.reg < 0b110 || (modrm.fields.mod == 0b11 && modrm.fields.reg == 0b111))) || (simd_prefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.reg != 0b100 && modrm.fields.reg != 0b110) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b101))))
					return 0;
			}
			else if (op == 0xb8)
			{
				if (!repeat_prefix)
					return 0;
			}
			else if (op == 0xba)
			{
				if (modrm.fields.reg <= 0b011)
					return 0;
			}
			else if (op == 0xd0)
			{
				if (!simd_prefix || simd_prefix == NMD_X86_PREFIXES_REPEAT)
					return 0;
			}
			else if (op == 0xe0)
			{
				if (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					return 0;
			}
			else if (op == 0xf0)
			{
				if (simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? modrm.fields.mod == 0b11 : true)
					return 0;
			}
			else if (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				if ((op >= 0x13 && op <= 0x17 && !(op == 0x16 && simd_prefix == NMD_X86_PREFIXES_REPEAT)) || op == 0x28 || op == 0x29 || op == 0x2e || op == 0x2f || (op <= 0x76 && op >= 0x74))
					return 0;
			}
			else if (op == 0x71 || op == 0x72 || (op == 0x73 && !(simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)))
			{
				if ((modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf) || modrm.modrm >= 0xf8)
					return 0;
			}
			else if (op >= 0xc3 && op <= 0xc6)
			{
				if ((op == 0xc5 && modrm.fields.mod != 0b11) || (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || (op == 0xc3 && simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
					return 0;
			}
			else if (_NMD_R(op) >= 0xd && _NMD_C(op) != 0 && op != 0xff && ((_NMD_C(op) == 6 && _NMD_R(op) != 0xf) ? (!simd_prefix || (_NMD_R(op) == 0xD && (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) ? modrm.fields.mod != 0b11 : false)) : (simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || ((_NMD_C(op) == 7 && _NMD_R(op) != 0xe) ? modrm.fields.mod != 0b11 : false))))
				return 0;
			else if (has_modrm && modrm.fields.mod == 0b11)
			{
				if (op == 0xb2 || op == 0xb4 || op == 0xb5 || op == 0xc3 || op == 0xe7 || op == 0x2b || (simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x12 || op == 0x16)) || (!(simd_prefix == NMD_X86_PREFIXES_REPEAT || simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (op == 0x13 || op == 0x17)))
					return 0;
			}
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */

			uint8_t imm_mask = 0;
			if (_NMD_R(op) == 8) /* imm32 */
				imm_mask = _NMD_GET_BY_MODE_OPSZPRFX_F64(mode, operand_prefix, 2, 4, 4);
			else if ((_NMD_R(op) == 7 && _NMD_C(op) < 4) || op == 0xA4 || op == 0xC2 || (op > 0xC3 && op <= 0xC6) || op == 0xBA || op == 0xAC) /* imm8 */
				imm_mask = 1;
			else if (op == 0x78 && (repeat_not_zero_prefix || operand_prefix)) /* imm8 + imm8 = "imm16" */
				imm_mask = 2;

			/* Make sure we can "read" 'imm_mask' bytes from the buffer */
			if (buffer_size < imm_mask)
				return false;

			/* Increment the buffer and decrement the buffer's size */
			b += imm_mask;
			buffer_size -= imm_mask;
		}
	}
	else /* 1 byte opcode */
	{
		opcode_size = 1;

		/* Check for ModR/M, SIB and displacement */
		if (_NMD_R(op) == 8 || _nmd_find_byte(_nmd_op1_modrm, sizeof(_nmd_op1_modrm), op) || (_NMD_R(op) < 4 && (_NMD_C(op) < 4 || (_NMD_C(op) >= 8 && _NMD_C(op) < 0xC))) || (_NMD_R(op) == 0xD && _NMD_C(op) >= 8)/* || ((op == 0xc4 || op == 0xc5) && remaining_size > 1 && ((nmd_x86_modrm*)(b + 1))->fields.mod != 0b11)*/)
		{
			if (!_nmd_ldisasm_decode_modrm(&b, &buffer_size, address_prefix, mode, &modrm))
				return 0;
			has_modrm = true;
		}

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
		if (op == 0xC6 || op == 0xC7)
		{
			if ((modrm.fields.reg != 0b000 && modrm.fields.reg != 0b111) || (modrm.fields.reg == 0b111 && (modrm.fields.mod != 0b11 || modrm.fields.rm != 0b000)))
				return 0;
		}
		else if (op == 0x8f)
		{
			if (modrm.fields.reg != 0b000)
				return 0;
		}
		else if (op == 0xfe)
		{
			if (modrm.fields.reg >= 0b010)
				return 0;
		}
		else if (op == 0xff)
		{
			if (modrm.fields.reg == 0b111 || (modrm.fields.mod == 0b11 && (modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101)))
				return 0;
		}
		else if (op == 0x8c)
		{
			if (modrm.fields.reg >= 0b110)
				return 0;
		}
		else if (op == 0x8e)
		{
			if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b110)
				return 0;
		}
		else if (op == 0x62)
		{
			if (mode == NMD_X86_MODE_64)
				return 0;
		}
		else if (op == 0x8d)
		{
			if (modrm.fields.mod == 0b11)
				return 0;
		}
		else if (op == 0xc4 || op == 0xc5)
		{
			if (mode == NMD_X86_MODE_64 && has_modrm && modrm.fields.mod != 0b11)
				return 0;
		}
		else if (op >= 0xd8 && op <= 0xdf)
		{
			switch (op)
			{
			case 0xd9:
				if ((modrm.fields.reg == 0b001 && modrm.fields.mod != 0b11) || (modrm.modrm > 0xd0 && modrm.modrm < 0xd8) || modrm.modrm == 0xe2 || modrm.modrm == 0xe3 || modrm.modrm == 0xe6 || modrm.modrm == 0xe7 || modrm.modrm == 0xef)
					return 0;
				break;
			case 0xda:
				if (modrm.modrm >= 0xe0 && modrm.modrm != 0xe9)
					return 0;
				break;
			case 0xdb:
				if (((modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110) && modrm.fields.mod != 0b11) || (modrm.modrm >= 0xe5 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
					return 0;
				break;
			case 0xdd:
				if ((modrm.fields.reg == 0b101 && modrm.fields.mod != 0b11) || _NMD_R(modrm.modrm) == 0xf)
					return 0;
				break;
			case 0xde:
				if (modrm.modrm == 0xd8 || (modrm.modrm >= 0xda && modrm.modrm <= 0xdf))
					return 0;
				break;
			case 0xdf:
				if ((modrm.modrm >= 0xe1 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
					return 0;
				break;
			}
		}
		else if (mode == NMD_X86_MODE_64)
		{
			if (op == 0x6 || op == 0x7 || op == 0xe || op == 0x16 || op == 0x17 || op == 0x1e || op == 0x1f || op == 0x27 || op == 0x2f || op == 0x37 || op == 0x3f || (op >= 0x60 && op <= 0x62) || op == 0x82 || op == 0xce || (op >= 0xd4 && op <= 0xd6))
				return 0;
		}
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX
		/* Check if instruction is VEX */
		if ((op == 0xc4 || op == 0xc5) && !has_modrm)
		{
			const uint8_t byte0 = op;

			uint8_t byte1;
			_NMD_READ_BYTE(b, buffer_size, byte1);

			if (byte0 == 0xc4)
			{
				uint8_t byte2;
				_NMD_READ_BYTE(b, buffer_size, byte2);

				_NMD_READ_BYTE(b, buffer_size, op);

				if (op == 0x0c || op == 0x0d || op == 0x40 || op == 0x41 || op == 0x17 || op == 0x21 || op == 0x42)
				{
					uint8_t imm;
					_NMD_READ_BYTE(b, buffer_size, imm);
				}
			}
			else /* 0xc5 */
			{
				_NMD_READ_BYTE(b, buffer_size, op);
			}

			if (!_nmd_ldisasm_decode_modrm(&b, &buffer_size, address_prefix, mode, &modrm))
				return false;
			has_modrm = true;
		}
		else
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX */

		{
			/* Check for immediate */
			uint8_t imm_mask = 0;
			if (_nmd_find_byte(_nmd_op1_imm32, sizeof(_nmd_op1_imm32), op) || (_NMD_R(op) < 4 && (_NMD_C(op) == 5 || _NMD_C(op) == 0xD)) || (_NMD_R(op) == 0xB && _NMD_C(op) >= 8) || (op == 0xF7 && modrm.fields.reg == 0b000)) /* imm32,16 */
			{
				if (_NMD_R(op) == 0xB && _NMD_C(op) >= 8)
					imm_mask = rexW ? 8 : (operand_prefix || (mode == NMD_X86_MODE_16 && !operand_prefix) ? 2 : 4);
				else
				{
					if ((mode == NMD_X86_MODE_16 && operand_prefix) || (mode != NMD_X86_MODE_16 && !operand_prefix))
						imm_mask = NMD_X86_IMM32;
					else
						imm_mask = NMD_X86_IMM16;
				}
			}
			else if (_NMD_R(op) == 7 || (_NMD_R(op) == 0xE && _NMD_C(op) < 8) || (_NMD_R(op) == 0xB && _NMD_C(op) < 8) || (_NMD_R(op) < 4 && (_NMD_C(op) == 4 || _NMD_C(op) == 0xC)) || (op == 0xF6 && modrm.fields.reg <= 0b001) || _nmd_find_byte(_nmd_op1_imm8, sizeof(_nmd_op1_imm8), op)) /* imm8 */
				imm_mask = 1;
			else if (_NMD_R(op) == 0xA && _NMD_C(op) < 4)
				imm_mask = (mode == NMD_X86_MODE_64) ? (address_prefix ? 4 : 8) : (address_prefix ? 2 : 4);
			else if (op == 0xEA || op == 0x9A) /* imm32,48 */
			{
				if (mode == NMD_X86_MODE_64)
					return 0;
				imm_mask = (operand_prefix ? 4 : 6);
			}
			else if (op == 0xC2 || op == 0xCA) /* imm16 */
				imm_mask = 2;
			else if (op == 0xC8) /* imm16 + imm8 */
				imm_mask = 3;

			/* Make sure we can "read" 'imm_mask' bytes from the buffer */
			if (buffer_size < imm_mask)
				return false;

			/* Increment the buffer and decrement the buffer's size */
			b += imm_mask;
			buffer_size -= imm_mask;
		}
	}

	if (lock_prefix)
	{
		if (!(has_modrm && modrm.fields.mod != 0b11 &&
			((opcode_size == 1 && (op == 0x86 || op == 0x87 || (_NMD_R(op) < 4 && (op % 8) < 2 && op < 0x38) || ((op >= 0x80 && op <= 0x83) && modrm.fields.reg != 0b111) || (op >= 0xfe && modrm.fields.reg < 2) || ((op == 0xf6 || op == 0xf7) && (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)))) ||
			(opcode_size == 2 && (_nmd_find_byte(_nmd_two_opcodes, sizeof(_nmd_two_opcodes), op) || op == 0xab || (op == 0xba && modrm.fields.reg != 0b100) || (op == 0xc7 && modrm.fields.reg == 0b001))))))
			return 0;
	}

	return (size_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));
}

typedef struct
{
	char* buffer;
	const nmd_x86_instruction* instruction;
	uint64_t runtime_address;
	uint32_t flags;
} _nmd_string_info;

NMD_ASSEMBLY_API void _nmd_append_string(_nmd_string_info* const si, const char* source)
{
	while (*source)
		*si->buffer++ = *source++;
}

NMD_ASSEMBLY_API void _nmd_append_number(_nmd_string_info* const si, uint64_t n)
{
	size_t buffer_offset;
	if (si->flags & NMD_X86_FORMAT_FLAGS_HEX)
	{
		size_t num_digits = _NMD_GET_NUM_DIGITS_HEX(n);
		buffer_offset = num_digits;

		const bool condition = n > 9 || si->flags & NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID;
		if (si->flags & NMD_X86_FORMAT_FLAGS_0X_PREFIX && condition)
			*si->buffer++ = '0', *si->buffer++ = 'x';

		const uint8_t base_char = (uint8_t)(si->flags & NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE ? 0x57 : 0x37);
		do {
			size_t num = n % 16;
			*(si->buffer + --num_digits) = (char)((num > 9 ? base_char : '0') + num);
		} while ((n /= 16) > 0);

		if (si->flags & NMD_X86_FORMAT_FLAGS_H_SUFFIX && condition)
			*(si->buffer + buffer_offset++) = 'h';
	}
	else
	{
		size_t num_digits = _NMD_GET_NUM_DIGITS(n);
		buffer_offset = num_digits + 1;

		do {
			*(si->buffer + --num_digits) = (char)('0' + n % 10);
		} while ((n /= 10) > 0);
	}

	si->buffer += buffer_offset;
}

NMD_ASSEMBLY_API void _nmd_append_signed_number(_nmd_string_info* const si, int64_t n, bool show_positive_sign)
{
	if (n >= 0)
	{
		if (show_positive_sign)
			*si->buffer++ = '+';

		_nmd_append_number(si, (uint64_t)n);
	}
	else
	{
		*si->buffer++ = '-';
		_nmd_append_number(si, (uint64_t)(~n + 1));
	}
}

NMD_ASSEMBLY_API void _nmd_append_signed_number_memory_view(_nmd_string_info* const si)
{
	_nmd_append_number(si, (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 0xFF00 : (si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFFFFFFFF00 : 0xFFFFFF00)) | si->instruction->immediate);
	if (si->flags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX)
	{
		*si->buffer++ = '(';
		_nmd_append_signed_number(si, (int8_t)(si->instruction->immediate), false);
		*si->buffer++ = ')';
	}
	else if (si->flags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC)
	{
		*si->buffer++ = '(';
		const uint32_t previous_mask = si->flags;
		si->flags &= ~NMD_X86_FORMAT_FLAGS_HEX;
		_nmd_append_signed_number(si, (int8_t)(si->instruction->immediate), false);
		si->flags = previous_mask;
		*si->buffer++ = ')';
	}
}

NMD_ASSEMBLY_API void _nmd_append_relative_address8(_nmd_string_info* const si)
{
	if (si->runtime_address == NMD_X86_INVALID_RUNTIME_ADDRESS)
	{
		/* *si->buffer++ = '$'; */
		_nmd_append_signed_number(si, (int64_t)((int8_t)(si->instruction->immediate) + (int8_t)(si->instruction->length)), true);
	}
	else
	{
		uint64_t n;
		if (si->instruction->mode == NMD_X86_MODE_64)
			n = (uint64_t)((int64_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int8_t)(si->instruction->immediate));
		else if (si->instruction->mode == NMD_X86_MODE_16)
			n = (uint16_t)((int16_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int8_t)(si->instruction->immediate));
		else
			n = (uint32_t)((int32_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int8_t)(si->instruction->immediate));
		_nmd_append_number(si, n);
	}
}

NMD_ASSEMBLY_API void _nmd_append_relative_address16_32(_nmd_string_info* const si)
{
	if (si->runtime_address == NMD_X86_INVALID_RUNTIME_ADDRESS)
	{
		_nmd_append_signed_number(si, (int64_t)(si->instruction->immediate + si->instruction->length), true);
	}
	else
	{
		_nmd_append_number(si, (uint64_t)((int64_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int64_t)((int32_t)(si->instruction->immediate))));
	}

	/*
		_nmd_append_number(si, ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & (si->instruction->mode == NMD_X86_MODE_64 ?
			(uint64_t)((int64_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int64_t)((int32_t)(si->instruction->immediate))) :
			(uint64_t)((int64_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int64_t)((int32_t)(si->instruction->immediate)))
		));
	*/
}

NMD_ASSEMBLY_API void _nmd_append_modrm_memory_prefix(_nmd_string_info* const si, const char* addr_specifier_reg)
{
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE
	if (si->flags & NMD_X86_FORMAT_FLAGS_POINTER_SIZE)
	{
		_nmd_append_string(si, addr_specifier_reg);
		_nmd_append_string(si, " ptr ");
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE */

	if (!(si->flags & NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE && !si->instruction->segment_override))
	{
		size_t i = 0;
		if (si->instruction->segment_override)
			i = _nmd_get_bit_index(si->instruction->segment_override);

		_nmd_append_string(si, si->instruction->segment_override ? _nmd_segment_reg[i] : (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_B) && (si->instruction->modrm.fields.rm == 0b100 || si->instruction->modrm.fields.rm == 0b101) ? "ss" : "ds"));
		*si->buffer++ = ':';
	}
}

NMD_ASSEMBLY_API void _nmd_append_modrm16_upper(_nmd_string_info* const si)
{
	*si->buffer++ = '[';

	if (!(si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b110))
	{
		const char* addresses[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };
		_nmd_append_string(si, addresses[si->instruction->modrm.fields.rm]);
	}

	if (si->instruction->disp_mask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		if (si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b110)
			_nmd_append_number(si, si->instruction->displacement);
		else
		{
			const bool is_negative = si->instruction->displacement & (1U << (si->instruction->disp_mask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = is_negative ? '-' : '+';

			if (is_negative)
			{
				const uint16_t mask = (uint16_t)(si->instruction->disp_mask == 2 ? 0xFFFF : 0xFF);
				_nmd_append_number(si, (uint64_t)(~si->instruction->displacement & mask) + 1);
			}
			else
				_nmd_append_number(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}

NMD_ASSEMBLY_API void _nmd_append_modrm32_upper(_nmd_string_info* const si)
{
	*si->buffer++ = '[';

	if (si->instruction->has_sib)
	{
		if (si->instruction->sib.fields.base == 0b101)
		{
			if (si->instruction->modrm.fields.mod != 0b00)
				_nmd_append_string(si, si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? "r13" : "rbp") : "ebp");
		}
		else
			_nmd_append_string(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? _nmd_regrx : _nmd_reg64) : _nmd_reg32)[si->instruction->sib.fields.base]);

		if (si->instruction->sib.fields.index != 0b100)
		{
			if (!(si->instruction->sib.fields.base == 0b101 && si->instruction->modrm.fields.mod == 0b00))
				*si->buffer++ = '+';
			_nmd_append_string(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_X ? _nmd_regrx : _nmd_reg64) : _nmd_reg32)[si->instruction->sib.fields.index]);
			if (!(si->instruction->sib.fields.scale == 0b00 && !(si->flags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', *si->buffer++ = (char)('0' + (1 << si->instruction->sib.fields.scale));
		}

		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_X && si->instruction->sib.fields.index == 0b100)
		{
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = '+';
			_nmd_append_string(si, "r12");
			if (!(si->instruction->sib.fields.scale == 0b00 && !(si->flags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', *si->buffer++ = (char)('0' + (1 << si->instruction->sib.fields.scale));
		}
	}
	else if (!(si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b101))
	{
		if ((si->instruction->prefixes & (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B) && si->instruction->mode == NMD_X86_MODE_64)
			_nmd_append_string(si, _nmd_regrx[si->instruction->modrm.fields.rm]), *si->buffer++ = 'd';
		else
			_nmd_append_string(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? _nmd_regrx : _nmd_reg64) : _nmd_reg32)[si->instruction->modrm.fields.rm]);
	}

	/* Handle displacement. */
	if (si->instruction->disp_mask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		/* Relative address. */
		if (si->instruction->modrm.fields.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.fields.mod == 0b00 && si->runtime_address != NMD_X86_INVALID_RUNTIME_ADDRESS)
		{
			if (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)
				_nmd_append_number(si, (uint64_t)((int64_t)(si->runtime_address + (uint64_t)si->instruction->length) + (int64_t)si->instruction->displacement));
			else
				_nmd_append_number(si, (uint64_t)((int64_t)(si->runtime_address + si->instruction->length) + (int64_t)((int32_t)si->instruction->displacement)));
		}
		else if (si->instruction->modrm.fields.mod == 0b00 && ((si->instruction->sib.fields.base == 0b101 && si->instruction->sib.fields.index == 0b100) || si->instruction->modrm.fields.rm == 0b101) && *(si->buffer - 1) == '[')
			_nmd_append_number(si, si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFF00000000 | si->instruction->displacement : si->instruction->displacement);
		else
		{
			if (si->instruction->modrm.fields.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.fields.mod == 0b00)
				_nmd_append_string(si, si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? "eip" : "rip");

			const bool is_negative = si->instruction->displacement & (1 << (si->instruction->disp_mask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = is_negative ? '-' : '+';

			if (is_negative)
			{
				const uint32_t mask = (uint32_t)(si->instruction->disp_mask == 4 ? -1 : (1 << (si->instruction->disp_mask * 8)) - 1);
				_nmd_append_number(si, (uint64_t)(~si->instruction->displacement & mask) + 1);
			}
			else
				_nmd_append_number(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}

NMD_ASSEMBLY_API void _nmd_append_modrm_upper(_nmd_string_info* const si, const char* addr_specifier_reg)
{
	_nmd_append_modrm_memory_prefix(si, addr_specifier_reg);

	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)) || (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32))
		_nmd_append_modrm16_upper(si);
	else
		_nmd_append_modrm32_upper(si);
}

NMD_ASSEMBLY_API void _nmd_append_modrm_upper_without_address_specifier(_nmd_string_info* const si)
{
	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)) || (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32))
		_nmd_append_modrm16_upper(si);
	else
		_nmd_append_modrm32_upper(si);
}

NMD_ASSEMBLY_API void _nmd_append_Nq(_nmd_string_info* const si)
{
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	*si->buffer++ = (char)('0' + si->instruction->modrm.fields.rm);
}

NMD_ASSEMBLY_API void _nmd_append_Pq(_nmd_string_info* const si)
{
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	*si->buffer++ = (char)('0' + si->instruction->modrm.fields.reg);
}

NMD_ASSEMBLY_API void _nmd_append_avx_register_reg(_nmd_string_info* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	_nmd_append_Pq(si);
}

NMD_ASSEMBLY_API void _nmd_append_avx_vvvv_register(_nmd_string_info* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	if ((15 - si->instruction->vex.vvvv) > 9)
		*si->buffer++ = '1', *si->buffer++ = (char)(0x26 + (15 - si->instruction->vex.vvvv));
	else
		*si->buffer++ = (char)('0' + (15 - si->instruction->vex.vvvv));
}

NMD_ASSEMBLY_API void _nmd_append_Vdq(_nmd_string_info* const si)
{
	*si->buffer++ = 'x';
	_nmd_append_Pq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Vqq(_nmd_string_info* const si)
{
	*si->buffer++ = 'y';
	_nmd_append_Pq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Vx(_nmd_string_info* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
		_nmd_append_Vdq(si);
	else
		_nmd_append_Vqq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Udq(_nmd_string_info* const si)
{
	*si->buffer++ = 'x';
	_nmd_append_Nq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Uqq(_nmd_string_info* const si)
{
	*si->buffer++ = 'y';
	_nmd_append_Nq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Ux(_nmd_string_info* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
		_nmd_append_Udq(si);
	else
		_nmd_append_Uqq(si);
}

NMD_ASSEMBLY_API void _nmd_append_Qq(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_Nq(si);
	else
		_nmd_append_modrm_upper(si, "qword");
}

NMD_ASSEMBLY_API void _nmd_append_Ev(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B)
		{
			_nmd_append_string(si, _nmd_regrx[si->instruction->modrm.fields.rm]);
			if (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_W))
				*si->buffer++ = 'd';
		}
		else
			_nmd_append_string(si, ((si->instruction->rex_w_prefix ? _nmd_reg64 : (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? _nmd_reg16 : _nmd_reg32))[si->instruction->modrm.fields.rm]);
	}
	else
		_nmd_append_modrm_upper(si, (si->instruction->rex_w_prefix) ? "qword" : ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? "word" : "dword"));
}

NMD_ASSEMBLY_API void _nmd_append_Ey(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_string(si, (si->instruction->rex_w_prefix ? _nmd_reg64 : _nmd_reg32)[si->instruction->modrm.fields.rm]);
	else
		_nmd_append_modrm_upper(si, si->instruction->rex_w_prefix ? "qword" : "dword");
}

NMD_ASSEMBLY_API void _nmd_append_Eb(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B)
			_nmd_append_string(si, _nmd_regrx[si->instruction->modrm.fields.rm]), *si->buffer++ = 'b';
		else
			_nmd_append_string(si, (si->instruction->has_rex ? _nmd_reg8_x64 : _nmd_reg8)[si->instruction->modrm.fields.rm]);
	}
	else
		_nmd_append_modrm_upper(si, "byte");
}

NMD_ASSEMBLY_API void _nmd_append_Ew(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_string(si, _nmd_reg16[si->instruction->modrm.fields.rm]);
	else
		_nmd_append_modrm_upper(si, "word");
}

NMD_ASSEMBLY_API void _nmd_append_Ed(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_string(si, _nmd_reg32[si->instruction->modrm.fields.rm]);
	else
		_nmd_append_modrm_upper(si, "dword");
}

NMD_ASSEMBLY_API void _nmd_append_Eq(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_string(si, _nmd_reg64[si->instruction->modrm.fields.rm]);
	else
		_nmd_append_modrm_upper(si, "qword");
}

NMD_ASSEMBLY_API void _nmd_append_Rv(_nmd_string_info* const si)
{
	_nmd_append_string(si, (si->instruction->rex_w_prefix ? _nmd_reg64 : (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? _nmd_reg16 : _nmd_reg32))[si->instruction->modrm.fields.rm]);
}

NMD_ASSEMBLY_API void _nmd_append_Gv(_nmd_string_info* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_R)
	{
		_nmd_append_string(si, _nmd_regrx[si->instruction->modrm.fields.reg]);
		if (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_W))
			*si->buffer++ = 'd';
	}
	else
		_nmd_append_string(si, ((si->instruction->rex_w_prefix) ? _nmd_reg64 : ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? _nmd_reg16 : _nmd_reg32))[si->instruction->modrm.fields.reg]);
}

NMD_ASSEMBLY_API void _nmd_append_Gy(_nmd_string_info* const si)
{
	_nmd_append_string(si, (si->instruction->rex_w_prefix ? _nmd_reg64 : _nmd_reg32)[si->instruction->modrm.fields.reg]);
}

NMD_ASSEMBLY_API void _nmd_append_Gb(_nmd_string_info* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_R)
		_nmd_append_string(si, _nmd_regrx[si->instruction->modrm.fields.reg]), *si->buffer++ = 'b';
	else
		_nmd_append_string(si, (si->instruction->has_rex ? _nmd_reg8_x64 : _nmd_reg8)[si->instruction->modrm.fields.reg]);
}

NMD_ASSEMBLY_API void _nmd_append_Gw(_nmd_string_info* const si)
{
	_nmd_append_string(si, _nmd_reg16[si->instruction->modrm.fields.reg]);
}

NMD_ASSEMBLY_API void _nmd_append_W(_nmd_string_info* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		_nmd_append_string(si, "xmm"), *si->buffer++ = (char)('0' + si->instruction->modrm.fields.rm);
	else
		_nmd_append_modrm_upper(si, "xmmword");
}

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX
NMD_ASSEMBLY_API char* _nmd_format_operand_to_att(char* operand, _nmd_string_info* si)
{
	char* next_operand = (char*)_nmd_strchr(operand, ',');
	const char* operand_end = next_operand ? next_operand : si->buffer;

	/* Memory operand. */
	const char* memory_operand = _nmd_strchr(operand, '[');
	if (memory_operand && memory_operand < operand_end)
	{
		memory_operand++;
		const char* segment_reg = _nmd_strchr(operand, ':');
		if (segment_reg)
		{
			if (segment_reg == operand + 2)
				_nmd_insert_char(operand, '%'), si->buffer++, operand += 4;
			else
			{
				*operand++ = '%';
				*operand++ = *(segment_reg - 2);
				*operand++ = 's';
				*operand++ = ':';
			}
		}

		/* Handle displacement. */
		char* displacement = operand;
		do
		{
			displacement++;
			displacement = (char*)_nmd_find_number(displacement, operand_end);
		} while (displacement && ((*(displacement - 1) != '+' && *(displacement - 1) != '-' && *(displacement - 1) != '[') || !_nmd_is_number(displacement, operand_end - 2)));

		bool is_there_base_or_index = true;
		char memory_operand_buffer[96];

		if (displacement)
		{
			if (*(displacement - 1) != '[')
				displacement--;
			else
				is_there_base_or_index = false;

			char* i = (char*)memory_operand;
			char* j = memory_operand_buffer;
			for (; i < displacement; i++, j++)
				*j = *i;
			*j = '\0';

			if (*displacement == '+')
				displacement++;

			for (; *displacement != ']'; displacement++, operand++)
				*operand = *displacement;
		}

		/* Handle base, index and scale. */
		if (is_there_base_or_index)
		{
			*operand++ = '(';

			char* base_or_index = operand;
			if (displacement)
			{
				char* s = memory_operand_buffer;
				for (; *s; s++, operand++)
					*operand = *s;
			}
			else
			{
				for (; *memory_operand != ']'; operand++, memory_operand++)
					*operand = *memory_operand;
			}

			_nmd_insert_char(base_or_index, '%');
			operand++;
			*operand++ = ')';

			for (; *base_or_index != ')'; base_or_index++)
			{
				if (*base_or_index == '+' || *base_or_index == '*')
				{
					if (*base_or_index == '+')
						_nmd_insert_char(base_or_index + 1, '%'), operand++;
					*base_or_index = ',';
				}
			}

			operand = base_or_index;
			operand++;
		}

		if (next_operand)
		{
			/* Move second operand to the left until the comma. */
			operand_end = _nmd_strchr(operand, ',');
			for (; *operand_end != '\0'; operand++, operand_end++)
				*operand = *operand_end;

			*operand = '\0';

			operand_end = operand;
			while (*operand_end != ',')
				operand_end--;
		}
		else
			*operand = '\0', operand_end = operand;

		si->buffer = operand;

		return (char*)operand_end;
	}
	else /* Immediate or register operand. */
	{
		_nmd_insert_char(operand, _nmd_is_number(operand, operand_end) ? '$' : '%');
		si->buffer++;
		return (char*)operand_end + 1;
	}
}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX */

/*
Formats an instruction. This function may cause a crash if you modify 'instruction' manually.
Parameters:
 - instruction     [in]  A pointer to a variable of type 'nmd_x86_instruction' describing the instruction to be formatted.
 - buffer          [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
 - runtime_address [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
 - flags           [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
*/
NMD_ASSEMBLY_API void nmd_x86_format(const nmd_x86_instruction* instruction, char* buffer, uint64_t runtime_address, uint32_t flags)
{
	if (!instruction->valid)
	{
		buffer[0] = '\0';
		return;
	}

	_nmd_string_info si;
	si.buffer = buffer;
	si.instruction = instruction;
	si.runtime_address = runtime_address;
	si.flags = flags;

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES
	if (flags & NMD_X86_FORMAT_FLAGS_BYTES)
	{
		size_t i = 0;
		for (; i < instruction->length; i++)
		{
			uint8_t num = instruction->buffer[i] >> 4;
			*si.buffer++ = (char)((num > 9 ? 0x37 : '0') + num);
			num = instruction->buffer[i] & 0xf;
			*si.buffer++ = (char)((num > 9 ? 0x37 : '0') + num);
			*si.buffer++ = ' ';
		}

		const size_t num_padding_bytes = instruction->length < NMD_X86_FORMATTER_NUM_PADDING_BYTES ? (NMD_X86_FORMATTER_NUM_PADDING_BYTES - instruction->length) : 0;
		for (i = 0; i < num_padding_bytes * 3; i++)
			*si.buffer++ = ' ';
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES */

	const uint8_t op = instruction->opcode;

	if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (instruction->prefixes & NMD_X86_PREFIXES_LOCK || ((op == 0x86 || op == 0x87) && instruction->modrm.fields.mod != 0b11)))
		_nmd_append_string(&si, instruction->repeat_prefix ? "xrelease " : "xacquire ");
	else if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (instruction->opcode_size == 1 && (op == 0xc2 || op == 0xc3 || op == 0xe8 || op == 0xe9 || _NMD_R(op) == 7 || (op == 0xff && (instruction->modrm.fields.reg == 0b010 || instruction->modrm.fields.reg == 0b100)))))
		_nmd_append_string(&si, "bnd ");

	if (instruction->prefixes & NMD_X86_PREFIXES_LOCK)
		_nmd_append_string(&si, "lock ");

	const bool opszprfx = instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE;

	if (instruction->opcode_map == NMD_X86_OPCODE_MAP_DEFAULT)
	{
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX
		if (instruction->encoding == NMD_X86_ENCODING_EVEX)
		{

		}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX */

#if !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_VEX
			if (instruction->encoding == NMD_X86_ENCODING_VEX)
			{
				if (instruction->vex.vex[0] == 0xc4)
				{
					if (instruction->opcode == 0x0c || instruction->opcode == 0x0d || instruction->opcode == 0x4a || instruction->opcode == 0x4b)
					{
						_nmd_append_string(&si, instruction->opcode == 0x0c ? "vblendps" : (instruction->opcode == 0x0c ? "vblendpd" : (instruction->opcode == 0x4a ? "vblendvps" : "vblendvpd")));
						*si.buffer++ = ' ';

						_nmd_append_avx_register_reg(&si);
						*si.buffer++ = ',';

						_nmd_append_avx_vvvv_register(&si);
						*si.buffer++ = ',';

						_nmd_append_W(&si);
						*si.buffer++ = ',';

						if (instruction->opcode <= 0x0d)
							_nmd_append_number(&si, instruction->immediate);
						else
						{
							_nmd_append_string(&si, "xmm");
							*si.buffer++ = (char)('0' + ((instruction->immediate & 0xf0) >> 4) % 8);
						}
					}
					else if (instruction->opcode == 0x40 || instruction->opcode == 0x41)
					{
						_nmd_append_string(&si, instruction->opcode == 0x40 ? "vdpps" : "vdppd");
						*si.buffer++ = ' ';

						_nmd_append_avx_register_reg(&si);
						*si.buffer++ = ',';

						_nmd_append_avx_vvvv_register(&si);
						*si.buffer++ = ',';

						_nmd_append_W(&si);
						*si.buffer++ = ',';

						_nmd_append_number(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x17)
					{
						_nmd_append_string(&si, "vextractps ");

						_nmd_append_Ev(&si);
						*si.buffer++ = ',';

						_nmd_append_Vdq(&si);
						*si.buffer++ = ',';

						_nmd_append_number(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x21)
					{
						_nmd_append_string(&si, "vinsertps ");

						_nmd_append_Vdq(&si);
						*si.buffer++ = ',';

						_nmd_append_avx_vvvv_register(&si);
						*si.buffer++ = ',';

						_nmd_append_W(&si);
						*si.buffer++ = ',';

						_nmd_append_number(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x2a)
					{
						_nmd_append_string(&si, "vmovntdqa ");

						_nmd_append_Vdq(&si);
						*si.buffer++ = ',';

						_nmd_append_modrm_upper_without_address_specifier(&si);
					}
					else if (instruction->opcode == 0x42)
					{
						_nmd_append_string(&si, "vmpsadbw ");

						_nmd_append_Vdq(&si);
						*si.buffer++ = ',';

						_nmd_append_avx_vvvv_register(&si);
						*si.buffer++ = ',';

						if (si.instruction->modrm.fields.mod == 0b11)
							_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
						else
							_nmd_append_modrm_upper_without_address_specifier(&si);
						*si.buffer++ = ',';

						_nmd_append_number(&si, instruction->immediate);
					}
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_VEX */

#if (!defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX)) && !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW)
			else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW
				if (instruction->encoding == NMD_X86_ENCODING_3DNOW)
				{
					const char* mnemonic = 0;
					switch (instruction->opcode)
					{
					case 0x0c: mnemonic = "pi2fw"; break;
					case 0x0d: mnemonic = "pi2fd"; break;
					case 0x1c: mnemonic = "pf2iw"; break;
					case 0x1d: mnemonic = "pf2id"; break;
					case 0x8a: mnemonic = "pfnacc"; break;
					case 0x8e: mnemonic = "pfpnacc"; break;
					case 0x90: mnemonic = "pfcmpge"; break;
					case 0x94: mnemonic = "pfmin"; break;
					case 0x96: mnemonic = "pfrcp"; break;
					case 0x97: mnemonic = "pfrsqrt"; break;
					case 0x9a: mnemonic = "pfsub"; break;
					case 0x9e: mnemonic = "pfadd"; break;
					case 0xa0: mnemonic = "pfcmpgt"; break;
					case 0xa4: mnemonic = "pfmax"; break;
					case 0xa6: mnemonic = "pfrcpit1"; break;
					case 0xa7: mnemonic = "pfrsqit1"; break;
					case 0xaa: mnemonic = "pfsubr"; break;
					case 0xae: mnemonic = "pfacc"; break;
					case 0xb0: mnemonic = "pfcmpeq"; break;
					case 0xb4: mnemonic = "pfmul"; break;
					case 0xb6: mnemonic = "pfrcpit2"; break;
					case 0xb7: mnemonic = "pmulhrw"; break;
					case 0xbb: mnemonic = "pswapd"; break;
					case 0xbf: mnemonic = "pavgusb"; break;
					default: return;
					}

					_nmd_append_string(&si, mnemonic);
					*si.buffer++ = ' ';

					_nmd_append_Pq(&si);
					*si.buffer++ = ',';
					_nmd_append_Qq(&si);
				}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW */

#if !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW)
				else /*if (instruction->encoding == INSTRUCTION_ENCODING_LEGACY) */
#endif
				{
					if (op >= 0x88 && op <= 0x8c) /* mov [88,8c] */
					{
						_nmd_append_string(&si, "mov ");
						if (op == 0x8b)
						{
							_nmd_append_Gv(&si);
							*si.buffer++ = ',';
							_nmd_append_Ev(&si);
						}
						else if (op == 0x89)
						{
							_nmd_append_Ev(&si);
							*si.buffer++ = ',';
							_nmd_append_Gv(&si);
						}
						else if (op == 0x88)
						{
							_nmd_append_Eb(&si);
							*si.buffer++ = ',';
							_nmd_append_Gb(&si);
						}
						else if (op == 0x8a)
						{
							_nmd_append_Gb(&si);
							*si.buffer++ = ',';
							_nmd_append_Eb(&si);
						}
						else if (op == 0x8c)
						{
							if (si.instruction->modrm.fields.mod == 0b11)
								_nmd_append_string(&si, (si.instruction->rex_w_prefix ? _nmd_reg64 : (si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? _nmd_reg16 : _nmd_reg32))[si.instruction->modrm.fields.rm]);
							else
								_nmd_append_modrm_upper(&si, "word");

							*si.buffer++ = ',';
							_nmd_append_string(&si, _nmd_segment_reg[instruction->modrm.fields.reg]);
						}
					}
					else if (op == 0x68 || op == 0x6A) /* push */
					{
						_nmd_append_string(&si, "push ");
						if (op == 0x6a)
						{
							if (flags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
								_nmd_append_signed_number_memory_view(&si);
							else
								_nmd_append_signed_number(&si, (int8_t)instruction->immediate, false);
						}
						else
							_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xff) /* Opcode extensions Group 5 */
					{
						_nmd_append_string(&si, _nmd_opcode_extensions_grp5[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (instruction->modrm.fields.mod == 0b11)
							_nmd_append_string(&si, (si.instruction->rex_w_prefix ? _nmd_reg64 : (opszprfx ? _nmd_reg16 : _nmd_reg32))[si.instruction->modrm.fields.rm]);
						else
							_nmd_append_modrm_upper(&si, (instruction->modrm.fields.reg == 0b011 || instruction->modrm.fields.reg == 0b101) ? "fword" : (instruction->mode == NMD_X86_MODE_64 && ((instruction->modrm.fields.reg >= 0b010 && instruction->modrm.fields.reg <= 0b110) || (instruction->prefixes & NMD_X86_PREFIXES_REX_W && instruction->modrm.fields.reg <= 0b010)) ? "qword" : (opszprfx ? "word" : "dword")));
					}
					else if (_NMD_R(op) < 4 && (_NMD_C(op) < 6 || (_NMD_C(op) >= 8 && _NMD_C(op) < 0xE))) /* add,adc,and,xor,or,sbb,sub,cmp */
					{
						_nmd_append_string(&si, _nmd_op1_opcode_map_mnemonics[_NMD_R((_NMD_C(op) > 6 ? op + 0x40 : op))]);
						*si.buffer++ = ' ';

						switch (op % 8)
						{
						case 0:
							_nmd_append_Eb(&si);
							*si.buffer++ = ',';
							_nmd_append_Gb(&si);
							break;
						case 1:
							_nmd_append_Ev(&si);
							*si.buffer++ = ',';
							_nmd_append_Gv(&si);
							break;
						case 2:
							_nmd_append_Gb(&si);
							*si.buffer++ = ',';
							_nmd_append_Eb(&si);
							break;
						case 3:
							_nmd_append_Gv(&si);
							*si.buffer++ = ',';
							_nmd_append_Ev(&si);
							break;
						case 4:
							_nmd_append_string(&si, "al,");
							_nmd_append_number(&si, instruction->immediate);
							break;
						case 5:
							_nmd_append_string(&si, instruction->rex_w_prefix ? "rax" : (opszprfx ? "ax" : "eax"));
							*si.buffer++ = ',';
							_nmd_append_number(&si, instruction->immediate);
							break;
						}
					}
					else if (_NMD_R(op) == 4 || _NMD_R(op) == 5) /* inc,dec,push,pop [0x40, 0x5f] */
					{
						_nmd_append_string(&si, _NMD_C(op) < 8 ? (_NMD_R(op) == 4 ? "inc " : "push ") : (_NMD_R(op) == 4 ? "dec " : "pop "));
						_nmd_append_string(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? (opszprfx ? _nmd_regrxw : _nmd_regrx) : (opszprfx ? (instruction->mode == NMD_X86_MODE_16 ? _nmd_reg32 : _nmd_reg16) : ((instruction->mode == NMD_X86_MODE_32 ? _nmd_reg32 : (instruction->mode == NMD_X86_MODE_64 ? _nmd_reg64 : _nmd_reg16)))))[op % 8]);
					}
					else if (op >= 0x80 && op < 0x84) /* add,adc,and,xor,or,sbb,sub,cmp [80,83] */
					{
						_nmd_append_string(&si, _nmd_opcode_extensions_grp1[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op == 0x80 || op == 0x82)
							_nmd_append_Eb(&si);
						else
							_nmd_append_Ev(&si);
						*si.buffer++ = ',';
						if (op == 0x83)
						{
							if ((instruction->modrm.fields.reg == 0b001 || instruction->modrm.fields.reg == 0b100 || instruction->modrm.fields.reg == 0b110) && instruction->immediate >= 0x80)
								_nmd_append_number(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? 0xFFFFFFFFFFFFFF00 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFF00 : 0xFFFFFF00)) | instruction->immediate);
							else
								_nmd_append_signed_number(&si, (int8_t)(instruction->immediate), false);
						}
						else
							_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xe8 || op == 0xe9 || op == 0xeb) /* call,jmp */
					{
						_nmd_append_string(&si, op == 0xe8 ? "call " : "jmp ");
						if (op == 0xeb)
							_nmd_append_relative_address8(&si);
						else
							_nmd_append_relative_address16_32(&si);
					}
					else if (op >= 0xA0 && op < 0xA4) /* mov [a0, a4] */
					{
						_nmd_append_string(&si, "mov ");
						if (op == 0xa0)
						{
							_nmd_append_string(&si, "al,");
							_nmd_append_modrm_memory_prefix(&si, "byte");
							*si.buffer++ = '[';
							_nmd_append_number(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							*si.buffer++ = ']';
						}
						else if (op == 0xa1)
						{
							_nmd_append_string(&si, instruction->rex_w_prefix ? "rax," : (opszprfx ? "ax," : "eax,"));
							_nmd_append_modrm_memory_prefix(&si, instruction->rex_w_prefix ? "qword" : (opszprfx ? "word" : "dword"));
							*si.buffer++ = '[';
							_nmd_append_number(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							*si.buffer++ = ']';
						}
						else if (op == 0xa2)
						{
							_nmd_append_modrm_memory_prefix(&si, "byte");
							*si.buffer++ = '[';
							_nmd_append_number(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							_nmd_append_string(&si, "],al");
						}
						else if (op == 0xa3)
						{
							_nmd_append_modrm_memory_prefix(&si, instruction->rex_w_prefix ? "qword" : (opszprfx ? "word" : "dword"));
							*si.buffer++ = '[';
							_nmd_append_number(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							_nmd_append_string(&si, "],");
							_nmd_append_string(&si, instruction->rex_w_prefix ? "rax" : (opszprfx ? "ax" : "eax"));
						}
					}
					else if (op == 0xcc) /* int3 */
						_nmd_append_string(&si, "int3");
					else if (op == 0x8d) /* lea */
					{
						_nmd_append_string(&si, "lea ");
						_nmd_append_Gv(&si);
						*si.buffer++ = ',';
						_nmd_append_modrm_upper_without_address_specifier(&si);
					}
					else if (op == 0x8f) /* pop */
					{
						_nmd_append_string(&si, "pop ");
						if (instruction->modrm.fields.mod == 0b11)
							_nmd_append_string(&si, (opszprfx ? _nmd_reg16 : _nmd_reg32)[instruction->modrm.fields.rm]);
						else
							_nmd_append_modrm_upper(&si, instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) ? "qword" : (opszprfx ? "word" : "dword"));
					}
					else if (_NMD_R(op) == 7) /* conditional jump [70,7f]*/
					{
						*si.buffer++ = 'j';
						_nmd_append_string(&si, _nmd_condition_suffixes[_NMD_C(op)]);
						*si.buffer++ = ' ';
						_nmd_append_relative_address8(&si);
					}
					else if (op == 0xa8) /* test */
					{
						_nmd_append_string(&si, "test al,");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xa9) /* test */
					{
						_nmd_append_string(&si, instruction->rex_w_prefix ? "test rax" : (opszprfx ? "test ax" : "test eax"));
						*si.buffer++ = ',';
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0x90)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							_nmd_append_string(&si, "pause");
						else if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							_nmd_append_string(&si, instruction->prefixes & NMD_X86_PREFIXES_REX_W ? "xchg r8,rax" : "xchg r8d,eax");
						else
							_nmd_append_string(&si, "nop");
					}
					else if (op == 0xc3)
						_nmd_append_string(&si, "ret");
					else if (_NMD_R(op) == 0xb) /* mov [b0, bf] */
					{
						_nmd_append_string(&si, "mov ");
						if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							_nmd_append_string(&si, _nmd_regrx[op % 8]), *si.buffer++ = _NMD_C(op) < 8 ? 'b' : 'd';
						else
							_nmd_append_string(&si, (_NMD_C(op) < 8 ? (instruction->has_rex ? _nmd_reg8_x64 : _nmd_reg8) : (instruction->rex_w_prefix ? _nmd_reg64 : (opszprfx ? _nmd_reg16 : _nmd_reg32)))[op % 8]);
						*si.buffer++ = ',';
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xfe) /* inc,dec */
					{
						_nmd_append_string(&si, instruction->modrm.fields.reg == 0b000 ? "inc " : "dec ");
						_nmd_append_Eb(&si);
					}
					else if (op == 0xf6 || op == 0xf7) /* test,test,not,neg,mul,imul,div,idiv */
					{
						_nmd_append_string(&si, _nmd_opcode_extensions_grp3[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op == 0xf6)
							_nmd_append_Eb(&si);
						else
							_nmd_append_Ev(&si);

						if (instruction->modrm.fields.reg <= 0b001)
						{
							*si.buffer++ = ',';
							_nmd_append_number(&si, instruction->immediate);
						}
					}
					else if (op == 0x69 || op == 0x6B)
					{
						_nmd_append_string(&si, "imul ");
						_nmd_append_Gv(&si);
						*si.buffer++ = ',';
						_nmd_append_Ev(&si);
						*si.buffer++ = ',';
						if (op == 0x6b)
						{
							if (si.flags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
								_nmd_append_signed_number_memory_view(&si);
							else
								_nmd_append_signed_number(&si, (int8_t)instruction->immediate, false);
						}
						else
							_nmd_append_number(&si, instruction->immediate);
					}
					else if (op >= 0x84 && op <= 0x87)
					{
						_nmd_append_string(&si, op > 0x85 ? "xchg " : "test ");
						if (op % 2 == 0)
						{
							_nmd_append_Eb(&si);
							*si.buffer++ = ',';
							_nmd_append_Gb(&si);
						}
						else
						{
							_nmd_append_Ev(&si);
							*si.buffer++ = ',';
							_nmd_append_Gv(&si);
						}
					}
					else if (op == 0x8e)
					{
						_nmd_append_string(&si, "mov ");
						_nmd_append_string(&si, _nmd_segment_reg[instruction->modrm.fields.reg]);
						*si.buffer++ = ',';
						_nmd_append_Ew(&si);
					}
					else if (op >= 0x91 && op <= 0x97)
					{
						_nmd_append_string(&si, "xchg ");
						if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
						{
							_nmd_append_string(&si, _nmd_regrx[_NMD_C(op)]);
							if (!(instruction->prefixes & NMD_X86_PREFIXES_REX_W))
								*si.buffer++ = 'd';
						}
						else
							_nmd_append_string(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? _nmd_reg64 : (opszprfx ? _nmd_reg16 : _nmd_reg32))[_NMD_C(op)]);
						_nmd_append_string(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? ",rax" : (opszprfx ? ",ax" : ",eax")));
					}
					else if (op == 0x9A)
					{
						_nmd_append_string(&si, "call far ");
						_nmd_append_number(&si, (uint64_t)(*(uint16_t*)((char*)(&instruction->immediate) + (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 2 : 4))));
						*si.buffer++ = ':';
						_nmd_append_number(&si, (uint64_t)(opszprfx ? *((uint16_t*)(&instruction->immediate)) : *((uint32_t*)(&instruction->immediate))));
					}
					else if ((op >= 0x6c && op <= 0x6f) || (op >= 0xa4 && op <= 0xa7) || (op >= 0xaa && op <= 0xaf))
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							_nmd_append_string(&si, "rep ");
						else if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							_nmd_append_string(&si, "repne ");

						const char* str = 0;
						switch (op)
						{
						case 0x6c: case 0x6d: str = "ins"; break;
						case 0x6e: case 0x6f: str = "outs"; break;
						case 0xa4: case 0xa5: str = "movs"; break;
						case 0xa6: case 0xa7: str = "cmps"; break;
						case 0xaa: case 0xab: str = "stos"; break;
						case 0xac: case 0xad: str = "lods"; break;
						case 0xae: case 0xaf: str = "scas"; break;
						}
						_nmd_append_string(&si, str);
						*si.buffer++ = (op % 2 == 0) ? 'b' : (opszprfx ? 'w' : 'd');
					}
					else if (op == 0xC0 || op == 0xC1 || (_NMD_R(op) == 0xd && _NMD_C(op) < 4))
					{
						_nmd_append_string(&si, _nmd_opcode_extensions_grp2[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op % 2 == 0)
							_nmd_append_Eb(&si);
						else
							_nmd_append_Ev(&si);
						*si.buffer++ = ',';
						if (_NMD_R(op) == 0xc)
							_nmd_append_number(&si, instruction->immediate);
						else if (_NMD_C(op) < 2)
							_nmd_append_number(&si, 1);
						else
							_nmd_append_string(&si, "cl");
					}
					else if (op == 0xc2)
					{
						_nmd_append_string(&si, "ret ");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op >= 0xe0 && op <= 0xe3)
					{
						const char* mnemonics[] = { "loopne", "loope", "loop" };
						_nmd_append_string(&si, op == 0xe3 ? (instruction->mode == NMD_X86_MODE_64 ? "jrcxz" : "jecxz") : mnemonics[_NMD_C(op)]);
						*si.buffer++ = ' ';
						_nmd_append_relative_address8(&si);
					}
					else if (op == 0xea)
					{
						_nmd_append_string(&si, "jmp far ");
						_nmd_append_number(&si, (uint64_t)(*(uint16_t*)(((uint8_t*)(&instruction->immediate) + 4))));
						*si.buffer++ = ':';
						_nmd_append_number(&si, (uint64_t)(*(uint32_t*)(&instruction->immediate)));
					}
					else if (op == 0xca)
					{
						_nmd_append_string(&si, "retf ");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xcd)
					{
						_nmd_append_string(&si, "int ");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0x63)
					{
						if (instruction->mode == NMD_X86_MODE_64)
						{
							_nmd_append_string(&si, "movsxd ");
							_nmd_append_string(&si, (instruction->mode == NMD_X86_MODE_64 ? (instruction->prefixes & NMD_X86_PREFIXES_REX_R ? _nmd_regrx : _nmd_reg64) : (opszprfx ? _nmd_reg16 : _nmd_reg32))[instruction->modrm.fields.reg]);
							*si.buffer++ = ',';
							if (instruction->modrm.fields.mod == 0b11)
							{
								if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
									_nmd_append_string(&si, _nmd_regrx[instruction->modrm.fields.rm]), *si.buffer++ = 'd';
								else
									_nmd_append_string(&si, ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? _nmd_reg16 : _nmd_reg32)[instruction->modrm.fields.rm]);
							}
							else
								_nmd_append_modrm_upper(&si, (instruction->rex_w_prefix && !(instruction->prefixes & NMD_X86_PREFIXES_REX_W)) ? "qword" : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? "word" : "dword"));
						}
						else
						{
							_nmd_append_string(&si, "arpl ");
							_nmd_append_Ew(&si);
							*si.buffer++ = ',';
							_nmd_append_Gw(&si);
						}
					}
					else if (op == 0xc4 || op == 0xc5)
					{
						_nmd_append_string(&si, op == 0xc4 ? "les" : "lds");
						*si.buffer++ = ' ';
						_nmd_append_Gv(&si);
						*si.buffer++ = ',';
						if (si.instruction->modrm.fields.mod == 0b11)
							_nmd_append_string(&si, (si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? _nmd_reg16 : _nmd_reg32)[si.instruction->modrm.fields.rm]);
						else
							_nmd_append_modrm_upper(&si, si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "dword" : "fword");
					}
					else if (op == 0xc6 || op == 0xc7)
					{
						_nmd_append_string(&si, instruction->modrm.fields.reg == 0b000 ? "mov " : (op == 0xc6 ? "xabort " : "xbegin "));
						if (instruction->modrm.fields.reg == 0b111)
						{
							if (op == 0xc6)
								_nmd_append_number(&si, instruction->immediate);
							else
								_nmd_append_relative_address16_32(&si);
						}
						else
						{
							if (op == 0xc6)
								_nmd_append_Eb(&si);
							else
								_nmd_append_Ev(&si);
							*si.buffer++ = ',';
							_nmd_append_number(&si, instruction->immediate);
						}
					}
					else if (op == 0xc8)
					{
						_nmd_append_string(&si, "enter ");
						_nmd_append_number(&si, (uint64_t)(*(uint16_t*)(&instruction->immediate)));
						*si.buffer++ = ',';
						_nmd_append_number(&si, (uint64_t)(*((uint8_t*)(&instruction->immediate) + 2)));
					}
					else if (op == 0xd4)
					{
						_nmd_append_string(&si, "aam ");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xd5)
					{
						_nmd_append_string(&si, "aad ");
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op >= 0xd8 && op <= 0xdf)
					{
						*si.buffer++ = 'f';

						if (instruction->modrm.modrm < 0xc0)
						{
							_nmd_append_string(&si, _nmd_escape_opcodes[_NMD_C(op) - 8][instruction->modrm.fields.reg]);
							*si.buffer++ = ' ';
							switch (op)
							{
							case 0xd8: case 0xda: _nmd_append_modrm_upper(&si, "dword"); break;
							case 0xd9: _nmd_append_modrm_upper(&si, instruction->modrm.fields.reg & 0b100 ? (instruction->modrm.fields.reg & 0b001 ? "word" : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "m14" : "m28")) : "dword"); break;
							case 0xdb: _nmd_append_modrm_upper(&si, instruction->modrm.fields.reg & 0b100 ? "tbyte" : "dword"); break;
							case 0xdc: _nmd_append_modrm_upper(&si, "qword"); break;
							case 0xdd: _nmd_append_modrm_upper(&si, instruction->modrm.fields.reg & 0b100 ? ((instruction->modrm.fields.reg & 0b111) == 0b111 ? "word" : "byte") : "qword"); break;
							case 0xde: _nmd_append_modrm_upper(&si, "word"); break;
							case 0xdf: _nmd_append_modrm_upper(&si, instruction->modrm.fields.reg & 0b100 ? (instruction->modrm.fields.reg & 0b001 ? "qword" : "tbyte") : "word"); break;
							}
						}
						else
						{
							switch (op)
							{
							case 0xd8:
								_nmd_append_string(&si, _nmd_escape_opcodesD8[(_NMD_R(instruction->modrm.modrm) - 0xc) * 2 + (_NMD_C(instruction->modrm.modrm) > 7 ? 1 : 0)]);
								_nmd_append_string(&si, " st(0),st(");
								*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8), *si.buffer++ = ')';
								break;
							case 0xd9:
								if (_NMD_R(instruction->modrm.modrm) == 0xc)
								{
									_nmd_append_string(&si, _NMD_C(instruction->modrm.modrm) < 8 ? "ld" : "xch");
									_nmd_append_string(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8), *si.buffer++ = ')';
								}
								else if (instruction->modrm.modrm >= 0xd8 && instruction->modrm.modrm <= 0xdf)
								{
									_nmd_append_string(&si, "stpnce st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									_nmd_append_string(&si, "),st(0)");
								}
								else
								{
									const char* str = 0;
									switch (instruction->modrm.modrm)
									{
									case 0xd0: str = "nop"; break;
									case 0xe0: str = "chs"; break;
									case 0xe1: str = "abs"; break;
									case 0xe4: str = "tst"; break;
									case 0xe5: str = "xam"; break;
									case 0xe8: str = "ld1"; break;
									case 0xe9: str = "ldl2t"; break;
									case 0xea: str = "ldl2e"; break;
									case 0xeb: str = "ldpi"; break;
									case 0xec: str = "ldlg2"; break;
									case 0xed: str = "ldln2"; break;
									case 0xee: str = "ldz"; break;
									case 0xf0: str = "2xm1"; break;
									case 0xf1: str = "yl2x"; break;
									case 0xf2: str = "ptan"; break;
									case 0xf3: str = "patan"; break;
									case 0xf4: str = "xtract"; break;
									case 0xf5: str = "prem1"; break;
									case 0xf6: str = "decstp"; break;
									case 0xf7: str = "incstp"; break;
									case 0xf8: str = "prem"; break;
									case 0xf9: str = "yl2xp1"; break;
									case 0xfa: str = "sqrt"; break;
									case 0xfb: str = "sincos"; break;
									case 0xfc: str = "rndint"; break;
									case 0xfd: str = "scale"; break;
									case 0xfe: str = "sin"; break;
									case 0xff: str = "cos"; break;
									}
									_nmd_append_string(&si, str);
								}
								break;
							case 0xda:
								if (instruction->modrm.modrm == 0xe9)
									_nmd_append_string(&si, "ucompp");
								else
								{
									const char* mnemonics[4] = { "cmovb", "cmovbe", "cmove", "cmovu" };
									_nmd_append_string(&si, mnemonics[(_NMD_R(instruction->modrm.modrm) - 0xc) + (_NMD_C(instruction->modrm.modrm) > 7 ? 2 : 0)]);
									_nmd_append_string(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								break;
							case 0xdb:
								if (_NMD_R(instruction->modrm.modrm) == 0xe && _NMD_C(instruction->modrm.modrm) < 8)
								{
									const char* mnemonics[] = { "eni8087_nop", "disi8087_nop", "nclex", "ninit", "setpm287_nop" };
									_nmd_append_string(&si, mnemonics[_NMD_C(instruction->modrm.modrm)]);
								}
								else
								{
									if (instruction->modrm.modrm >= 0xe0)
										_nmd_append_string(&si, instruction->modrm.modrm < 0xf0 ? "ucomi" : "comi");
									else
									{
										_nmd_append_string(&si, "cmovn");
										if (instruction->modrm.modrm < 0xc8)
											*si.buffer++ = 'b';
										else if (instruction->modrm.modrm < 0xd0)
											*si.buffer++ = 'e';
										else if (instruction->modrm.modrm >= 0xd8)
											*si.buffer++ = 'u';
										else
											_nmd_append_string(&si, "be");
									}
									_nmd_append_string(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								break;
							case 0xdc:
								if (_NMD_R(instruction->modrm.modrm) == 0xc)
									_nmd_append_string(&si, _NMD_C(instruction->modrm.modrm) > 7 ? "mul" : "add");
								else
								{
									_nmd_append_string(&si, _NMD_R(instruction->modrm.modrm) == 0xd ? "com" : (_NMD_R(instruction->modrm.modrm) == 0xe ? "subr" : "div"));
									if (_NMD_R(instruction->modrm.modrm) == 0xd && _NMD_C(instruction->modrm.modrm) >= 8)
									{
										if (_NMD_R(instruction->modrm.modrm) >= 8)
											*si.buffer++ = 'p';
									}
									else
									{
										if (_NMD_R(instruction->modrm.modrm) < 8)
											*si.buffer++ = 'r';
									}
								}

								if (_NMD_R(instruction->modrm.modrm) == 0xd)
								{
									_nmd_append_string(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								else
								{
									_nmd_append_string(&si, " st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									_nmd_append_string(&si, "),st(0)");
								}
								break;
							case 0xdd:
								if (_NMD_R(instruction->modrm.modrm) == 0xc)
									_nmd_append_string(&si, _NMD_C(instruction->modrm.modrm) < 8 ? "free" : "xch");
								else
								{
									_nmd_append_string(&si, instruction->modrm.modrm < 0xe0 ? "st" : "ucom");
									if (_NMD_C(instruction->modrm.modrm) >= 8)
										*si.buffer++ = 'p';
								}

								_nmd_append_string(&si, " st(");
								*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
								*si.buffer++ = ')';

								break;
							case 0xde:
								if (instruction->modrm.modrm == 0xd9)
									_nmd_append_string(&si, "compp");
								else
								{
									if (instruction->modrm.modrm >= 0xd0 && instruction->modrm.modrm <= 0xd7)
									{
										_nmd_append_string(&si, "comp st(0),st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
									else
									{
										if (_NMD_R(instruction->modrm.modrm) == 0xc)
											_nmd_append_string(&si, _NMD_C(instruction->modrm.modrm) < 8 ? "add" : "mul");
										else
										{
											_nmd_append_string(&si, instruction->modrm.modrm < 0xf0 ? "sub" : "div");
											if (_NMD_R(instruction->modrm.modrm) < 8 || (_NMD_R(instruction->modrm.modrm) >= 0xe && _NMD_C(instruction->modrm.modrm) < 8))
												*si.buffer++ = 'r';
										}
										_nmd_append_string(&si, "p st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										_nmd_append_string(&si, "),st(0)");
									}
								}
								break;
							case 0xdf:
								if (instruction->modrm.modrm == 0xe0)
									_nmd_append_string(&si, "nstsw ax");
								else
								{
									if (instruction->modrm.modrm >= 0xe8)
									{
										if (instruction->modrm.modrm < 0xf0)
											*si.buffer++ = 'u';
										_nmd_append_string(&si, "comip");
										_nmd_append_string(&si, " st(0),st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
									else
									{
										_nmd_append_string(&si, instruction->modrm.modrm < 0xc8 ? "freep" : (instruction->modrm.modrm >= 0xd0 ? "stp" : "xch"));
										_nmd_append_string(&si, " st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
								}

								break;
							}
						}
					}
					else if (op == 0xe4 || op == 0xe5)
					{
						_nmd_append_string(&si, "in ");
						_nmd_append_string(&si, op == 0xe4 ? "al" : (opszprfx ? "ax" : "eax"));
						*si.buffer++ = ',';
						_nmd_append_number(&si, instruction->immediate);
					}
					else if (op == 0xe6 || op == 0xe7)
					{
						_nmd_append_string(&si, "out ");
						_nmd_append_number(&si, instruction->immediate);
						*si.buffer++ = ',';
						_nmd_append_string(&si, op == 0xe6 ? "al" : (opszprfx ? "ax" : "eax"));
					}
					else if (op == 0xec || op == 0xed)
					{
						_nmd_append_string(&si, "in ");
						_nmd_append_string(&si, op == 0xec ? "al" : (opszprfx ? "ax" : "eax"));
						_nmd_append_string(&si, ",dx");
					}
					else if (op == 0xee || op == 0xef)
					{
						_nmd_append_string(&si, "out dx,");
						_nmd_append_string(&si, op == 0xee ? "al" : (opszprfx ? "ax" : "eax"));
					}
					else if (op == 0x62)
					{
						_nmd_append_string(&si, "bound ");
						_nmd_append_Gv(&si);
						*si.buffer++ = ',';
						_nmd_append_modrm_upper(&si, opszprfx ? "dword" : "qword");
					}
					else /* Try to parse all opcodes not parsed by the checks above. */
					{
						const char* str = 0;
						switch (instruction->opcode)
						{
						case 0x9c:
						{
							if (opszprfx)
								str = (instruction->mode == NMD_X86_MODE_16) ? "pushfd" : "pushf";
							else
								str = (instruction->mode == NMD_X86_MODE_16) ? "pushf" : ((instruction->mode == NMD_X86_MODE_32) ? "pushfd" : "pushfq");
							break;
						}
						case 0x9d:
						{
							if (opszprfx)
								str = (instruction->mode == NMD_X86_MODE_16) ? "popfd" : "popf";
							else
								str = (instruction->mode == NMD_X86_MODE_16) ? "popf" : ((instruction->mode == NMD_X86_MODE_32) ? "popfd" : "popfq");
							break;
						}
						case 0x60: str = _NMD_GET_BY_MODE_OPSZPRFX(instruction->mode, opszprfx, "pusha", "pushad"); break;
						case 0x61: str = _NMD_GET_BY_MODE_OPSZPRFX(instruction->mode, opszprfx, "popa", "popad"); break;
						case 0xcb: str = "retf"; break;
						case 0xc9: str = "leave"; break;
						case 0xf1: str = "int1"; break;
						case 0x06: str = "push es"; break;
						case 0x16: str = "push ss"; break;
						case 0x1e: str = "push ds"; break;
						case 0x0e: str = "push cs"; break;
						case 0x07: str = "pop es"; break;
						case 0x17: str = "pop ss"; break;
						case 0x1f: str = "pop ds"; break;
						case 0x27: str = "daa"; break;
						case 0x37: str = "aaa"; break;
						case 0x2f: str = "das"; break;
						case 0x3f: str = "aas"; break;
						case 0xd7: str = "xlat"; break;
						case 0x9b: str = "fwait"; break;
						case 0xf4: str = "hlt"; break;
						case 0xf5: str = "cmc"; break;
						case 0x9e: str = "sahf"; break;
						case 0x9f: str = "lahf"; break;
						case 0xce: str = "into"; break;
						case 0xcf:
							if (instruction->rex_w_prefix)
								str = "iretq";
							else if (instruction->mode == NMD_X86_MODE_16)
								str = opszprfx ? "iretd" : "iret";
							else
								str = opszprfx ? "iret" : "iretd";
							break;
						case 0x98:
							if (instruction->prefixes & NMD_X86_PREFIXES_REX_W)
								str = "cdqe";
							else if (instruction->mode == NMD_X86_MODE_16)
								str = opszprfx ? "cwde" : "cbw";
							else
								str = opszprfx ? "cbw" : "cwde";
							break;
						case 0x99:
							if (instruction->prefixes & NMD_X86_PREFIXES_REX_W)
								str = "cqo";
							else if (instruction->mode == NMD_X86_MODE_16)
								str = opszprfx ? "cdq" : "cwd";
							else
								str = opszprfx ? "cwd" : "cdq";
							break;
						case 0xd6: str = "salc"; break;
						case 0xf8: str = "clc"; break;
						case 0xf9: str = "stc"; break;
						case 0xfa: str = "cli"; break;
						case 0xfb: str = "sti"; break;
						case 0xfc: str = "cld"; break;
						case 0xfd: str = "std"; break;
						default: return;
						}
						_nmd_append_string(&si, str);
					}
				}
	}
	else if (instruction->opcode_map == NMD_X86_OPCODE_MAP_0F)
	{
		if (_NMD_R(op) == 8)
		{
			*si.buffer++ = 'j';
			_nmd_append_string(&si, _nmd_condition_suffixes[_NMD_C(op)]);
			*si.buffer++ = ' ';
			_nmd_append_relative_address16_32(&si);
		}
		else if (op == 0x05)
			_nmd_append_string(&si, "syscall");
		else if (op == 0xa2)
			_nmd_append_string(&si, "cpuid");
		else if (_NMD_R(op) == 4)
		{
			_nmd_append_string(&si, "cmov");
			_nmd_append_string(&si, _nmd_condition_suffixes[_NMD_C(op)]);
			*si.buffer++ = ' ';
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_Ev(&si);
		}
		else if (op >= 0x10 && op <= 0x17)
		{
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66_mnemonics[] = { "movupd", "movupd", "movlpd", "movlpd", "unpcklpd", "unpckhpd", "movhpd", "movhpd" };

				_nmd_append_string(&si, prefix66_mnemonics[_NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (_NMD_C(op))
				{
				case 0:
					_nmd_append_Vx(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				case 1:
					_nmd_append_W(&si);
					*si.buffer++ = ',';
					_nmd_append_Vx(&si);
					break;
				case 2:
				case 6:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				default:
					break;
				}
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3_mnemonics[] = { "movss", "movss", "movsldup", 0, 0, 0, "movshdup" };

				_nmd_append_string(&si, prefixF3_mnemonics[_NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (_NMD_C(op))
				{
				case 0:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "dword");
					break;
				case 1:
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "dword");
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				case 2:
				case 6:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				}
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2_mnemonics[] = { "movsd", "movsd", "movddup" };

				_nmd_append_string(&si, prefixF2_mnemonics[_NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (_NMD_C(op))
				{
				case 0:
				case 2:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				case 1:
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				}
			}
			else
			{
				const char* no_prefix_mnemonics[] = { "movups", "movups", "movlps", "movlps", "unpcklps", "unpckhps", "movhps", "movhps" };

				if (op == 0x12 && instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "movhlps");
				else if (op == 0x16 && instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "movlhps");
				else
					_nmd_append_string(&si, no_prefix_mnemonics[_NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (_NMD_C(op))
				{
				case 0:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				case 1:
					_nmd_append_W(&si);
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				case 2:
				case 6:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				default:
					break;
				};

			}

			switch (_NMD_C(op))
			{
			case 3:
			case 7:
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					_nmd_append_modrm_upper(&si, "qword");
				*si.buffer++ = ',';
				_nmd_append_Vdq(&si);
				break;
			case 4:
			case 5:
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_W(&si);
				break;
			};
		}
		else if (_NMD_R(op) == 6 || (op >= 0x74 && op <= 0x76))
		{
			if (op == 0x6e)
			{
				_nmd_append_string(&si, "movd ");
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					*si.buffer++ = 'x';
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, _nmd_reg32[si.instruction->modrm.fields.rm]);
				else
					_nmd_append_modrm_upper(&si, "dword");
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				{
					const char* prefix66_mnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw", "punpcklqdq", "punpckhqdq", "movd", "movdqa" };

					_nmd_append_string(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : prefix66_mnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
				}
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
				{
					_nmd_append_string(&si, "movdqu ");
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
				}
				else
				{
					const char* no_prefix_mnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw", 0, 0, "movd", "movq" };

					_nmd_append_string(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : no_prefix_mnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					_nmd_append_Pq(&si);
					*si.buffer++ = ',';
					_nmd_append_Qq(&si);
				}
			}
		}
		else if (op == 0x00)
		{
			_nmd_append_string(&si, _nmd_opcode_extensions_grp6[instruction->modrm.fields.reg]);
			*si.buffer++ = ' ';
			if (_NMD_R(instruction->modrm.modrm) == 0xc)
				_nmd_append_Ev(&si);
			else
				_nmd_append_Ew(&si);
		}
		else if (op == 0x01)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				if (instruction->modrm.fields.reg == 0b000)
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7_reg0[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b001)
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7_reg1[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b010)
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7_reg2[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b011)
				{
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7_reg3[instruction->modrm.fields.rm]);
					if (instruction->modrm.fields.rm == 0b000 || instruction->modrm.fields.rm == 0b010 || instruction->modrm.fields.rm == 0b111)
						_nmd_append_string(&si, instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "ax" : "eax");

					if (instruction->modrm.fields.rm == 0b111)
						_nmd_append_string(&si, ",ecx");
				}
				else if (instruction->modrm.fields.reg == 0b100)
					_nmd_append_string(&si, "smsw "), _nmd_append_string(&si, (instruction->rex_w_prefix ? _nmd_reg64 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? _nmd_reg16 : _nmd_reg32))[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b101)
				{
					if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
						_nmd_append_string(&si, instruction->modrm.fields.rm == 0b000 ? "setssbsy" : "saveprevssp");
					else
						_nmd_append_string(&si, instruction->modrm.fields.rm == 0b111 ? "wrpkru" : "rdpkru");
				}
				else if (instruction->modrm.fields.reg == 0b110)
					_nmd_append_string(&si, "lmsw "), _nmd_append_string(&si, _nmd_reg16[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b111)
				{
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7_reg7[instruction->modrm.fields.rm]);
					if (instruction->modrm.fields.rm == 0b100)
						_nmd_append_string(&si, instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "ax" : "eax");
				}
			}
			else
			{
				if (instruction->modrm.fields.reg == 0b101)
				{
					_nmd_append_string(&si, "rstorssp ");
					_nmd_append_modrm_upper(&si, "qword");
				}
				else
				{
					_nmd_append_string(&si, _nmd_opcode_extensions_grp7[instruction->modrm.fields.reg]);
					*si.buffer++ = ' ';
					if (si.instruction->modrm.fields.reg == 0b110)
						_nmd_append_Ew(&si);
					else
						_nmd_append_modrm_upper(&si, si.instruction->modrm.fields.reg == 0b111 ? "byte" : si.instruction->modrm.fields.reg == 0b100 ? "word" : "fword");
				}
			}
		}
		else if (op == 0x02 || op == 0x03)
		{
			_nmd_append_string(&si, op == 0x02 ? "lar" : "lsl");
			*si.buffer++ = ' ';
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				_nmd_append_string(&si, (opszprfx ? _nmd_reg16 : _nmd_reg32)[si.instruction->modrm.fields.rm]);
			else
				_nmd_append_modrm_upper(&si, "word");
		}
		else if (op == 0x0d)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				_nmd_append_string(&si, "nop ");
				_nmd_append_string(&si, (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? _nmd_reg16 : _nmd_reg32)[instruction->modrm.fields.rm]);
				*si.buffer++ = ',';
				_nmd_append_string(&si, (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? _nmd_reg16 : _nmd_reg32)[instruction->modrm.fields.reg]);
			}
			else
			{
				_nmd_append_string(&si, "prefetch");
				if (instruction->modrm.fields.reg == 0b001)
					*si.buffer++ = 'w';
				else if (instruction->modrm.fields.reg == 0b010)
					_nmd_append_string(&si, "wt1");

				*si.buffer++ = ' ';

				_nmd_append_modrm_upper(&si, "byte");
			}
		}
		else if (op == 0x18)
		{
			if (instruction->modrm.fields.mod == 0b11 || instruction->modrm.fields.reg >= 0b100)
			{
				_nmd_append_string(&si, "nop ");
				_nmd_append_Ev(&si);
			}
			else
			{
				if (instruction->modrm.fields.reg == 0b000)
					_nmd_append_string(&si, "prefetchnta");
				else
				{
					_nmd_append_string(&si, "prefetcht");
					*si.buffer++ = (char)('0' + (instruction->modrm.fields.reg - 1));
				}
				*si.buffer++ = ' ';

				_nmd_append_Eb(&si);
			}
		}
		else if (op == 0x19)
		{
			_nmd_append_string(&si, "nop ");
			_nmd_append_Ev(&si);
			*si.buffer++ = ',';
			_nmd_append_Gv(&si);
		}
		else if (op == 0x1A)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				_nmd_append_string(&si, "nop ");
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_Gv(&si);
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					_nmd_append_string(&si, "bndmov");
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
					_nmd_append_string(&si, "bndcl");
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					_nmd_append_string(&si, "bndcu");
				else
					_nmd_append_string(&si, "bndldx");

				_nmd_append_string(&si, " bnd");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
				*si.buffer++ = ',';
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					*si.buffer++ = 'q';
				_nmd_append_Ev(&si);
			}
		}
		else if (op == 0x1B)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				_nmd_append_string(&si, "nop ");
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_Gv(&si);
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					_nmd_append_string(&si, "bndmov");
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
					_nmd_append_string(&si, "bndmk");
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					_nmd_append_string(&si, "bndcn");
				else
					_nmd_append_string(&si, "bndstx");

				*si.buffer++ = ' ';
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_string(&si, "bnd");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
			}
		}
		else if (op >= 0x1c && op <= 0x1f)
		{
			if (op == 0x1e && instruction->modrm.modrm == 0xfa)
				_nmd_append_string(&si, "endbr64");
			else if (op == 0x1e && instruction->modrm.modrm == 0xfb)
				_nmd_append_string(&si, "endbr32");
			else
			{
				_nmd_append_string(&si, "nop ");
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_Gv(&si);
			}
		}
		else if (op >= 0x20 && op <= 0x23)
		{
			_nmd_append_string(&si, "mov ");
			if (op < 0x22)
			{
				_nmd_append_string(&si, (instruction->mode == NMD_X86_MODE_64 ? _nmd_reg64 : _nmd_reg32)[instruction->modrm.fields.rm]);
				_nmd_append_string(&si, op == 0x20 ? ",cr" : ",dr");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
			}
			else
			{
				_nmd_append_string(&si, op == 0x22 ? "cr" : "dr");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
				*si.buffer++ = ',';
				_nmd_append_string(&si, (instruction->mode == NMD_X86_MODE_64 ? _nmd_reg64 : _nmd_reg32)[instruction->modrm.fields.rm]);
			}
		}
		else if (op >= 0x28 && op <= 0x2f)
		{
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66_mnemonics[] = { "movapd", "movapd", "cvtpi2pd", "movntpd", "cvttpd2pi", "cvtpd2pi", "ucomisd", "comisd" };

				_nmd_append_string(&si, prefix66_mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					_nmd_append_Vx(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				case 1:
					_nmd_append_W(&si);
					*si.buffer++ = ',';
					_nmd_append_Vx(&si);
					break;
				case 2:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_Qq(&si);
					break;
				case 4:
				case 5:
					_nmd_append_Pq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				case 6:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				case 7:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
				default:
					break;
				}
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3_mnemonics[] = { 0, 0, "cvtsi2ss", "movntss", "cvttss2si", "cvtss2si", 0, 0 };

				_nmd_append_string(&si, prefixF3_mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 3:
					_nmd_append_modrm_upper(&si, "dword");
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				case 4:
				case 5:
					_nmd_append_Gv(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						_nmd_append_Udq(&si);
					else
						_nmd_append_modrm_upper(&si, "dword");
					break;
				case 2:
				case 6:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_Ev(&si);
					break;
				}
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2_mnemonics[] = { 0, 0, "cvtsi2sd", "movntsd", "cvttsd2si", "cvtsd2si", 0, 0 };

				_nmd_append_string(&si, prefixF2_mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 2:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_Ev(&si);
					break;
				case 3:
					_nmd_append_modrm_upper(&si, "qword");
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				case 4:
				case 5:
					_nmd_append_Gv(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				}
			}
			else
			{
				const char* no_prefix_mnemonics[] = { "movaps", "movaps", "cvtpi2ps", "movntps", "cvttps2pi", "cvtps2pi", "ucomiss", "comiss" };

				_nmd_append_string(&si, no_prefix_mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_W(&si);
					break;
				case 1:
					_nmd_append_W(&si);
					*si.buffer++ = ',';
					_nmd_append_Vdq(&si);
					break;
				case 4:
				case 5:
					_nmd_append_Pq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "qword");
					break;
				case 2:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					_nmd_append_Qq(&si);
					break;
				case 6:
				case 7:
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, "dword");
					break;
				default:
					break;
				};

			}

			if (!(instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op % 8) == 3)
			{
				_nmd_append_modrm_upper(&si, "xmmword");
				*si.buffer++ = ',';
				_nmd_append_Vdq(&si);
			}
		}
		else if (_NMD_R(op) == 5)
		{
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66_mnemonics[] = { "movmskpd", "sqrtpd", 0, 0, "andpd", "andnpd", "orpd", "xorpd", "addpd", "mulpd", "cvtpd2ps",  "cvtps2dq", "subpd", "minpd", "divpd", "maxpd" };

				_nmd_append_string(&si, prefix66_mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
				else
					_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_W(&si);
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3_mnemonics[] = { 0, "sqrtss", "rsqrtss", "rcpss", 0, 0, 0, 0, "addss", "mulss", "cvtss2sd", "cvttps2dq", "subss", "minss", "divss", "maxss" };

				_nmd_append_string(&si, prefixF3_mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					_nmd_append_modrm_upper(&si, op == 0x5b ? "xmmword" : "dword");
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2_mnemonics[] = { 0, "sqrtsd", 0, 0, 0, 0, 0, 0, "addsd", "mulsd", "cvtsd2ss", 0, "subsd", "minsd", "divsd", "maxsd" };

				_nmd_append_string(&si, prefixF2_mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					_nmd_append_modrm_upper(&si, "qword");
			}
			else
			{
				const char* no_prefix_mnemonics[] = { "movmskps", "sqrtps", "rsqrtps", "rcpps", "andps", "andnps", "orps", "xorps", "addps", "mulps", "cvtps2pd",  "cvtdq2ps", "subps", "minps", "divps", "maxps" };

				_nmd_append_string(&si, no_prefix_mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
				{
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					_nmd_append_Udq(&si);
				}
				else
				{
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						_nmd_append_modrm_upper(&si, op == 0x5a ? "qword" : "xmmword");
				}
			}
		}
		else if (op == 0x70)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "pshufd" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "pshufhw" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "pshuflw" : "pshufw")));
			*si.buffer++ = ' ';
			if (!(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Qq(&si);
			}
			else
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_W(&si);
			}

			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op >= 0x71 && op <= 0x73)
		{
			if (instruction->modrm.fields.reg % 2 == 1)
				_nmd_append_string(&si, instruction->modrm.fields.reg == 0b111 ? "pslldq" : "psrldq");
			else
			{
				const char* mnemonics[] = { "psrl", "psra", "psll" };
				_nmd_append_string(&si, mnemonics[(instruction->modrm.fields.reg >> 1) - 1]);
				*si.buffer++ = op == 0x71 ? 'w' : (op == 0x72 ? 'd' : 'q');
			}

			*si.buffer++ = ' ';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				_nmd_append_Udq(&si);
			else
				_nmd_append_Nq(&si);
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0x78)
		{
			if (!instruction->simd_prefix)
			{
				_nmd_append_string(&si, "vmread ");
				_nmd_append_Ey(&si);
				*si.buffer++ = ',';
				_nmd_append_Gy(&si);
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					_nmd_append_string(&si, "extrq ");
				else
				{
					_nmd_append_string(&si, "insertq ");
					_nmd_append_Vdq(&si);
					*si.buffer++ = ',';
				}
				_nmd_append_Udq(&si);
				*si.buffer++ = ',';
				_nmd_append_number(&si, instruction->immediate & 0x00FF);
				*si.buffer++ = ',';
				_nmd_append_number(&si, (instruction->immediate & 0xFF00) >> 8);
			}
		}
		else if (op == 0x79)
		{
			if (!instruction->simd_prefix)
			{
				_nmd_append_string(&si, "vmwrite ");
				_nmd_append_Gy(&si);
				*si.buffer++ = ',';
				_nmd_append_Ey(&si);
			}
			else
			{
				_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "extrq " : "insertq ");
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_Udq(&si);
			}

		}
		else if (op == 0x7c || op == 0x7d)
		{
			_nmd_append_string(&si, op == 0x7c ? "haddp" : "hsubp");
			*si.buffer++ = instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_W(&si);
		}
		else if (op == 0x7e)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "movq " : "movd ");
			if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_Udq(&si);
				else
					_nmd_append_modrm_upper(&si, "qword");
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.rm]);
				else
					_nmd_append_modrm_upper(&si, "dword");
				*si.buffer++ = ',';
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					_nmd_append_Vdq(&si);
				else
					_nmd_append_Pq(&si);
			}
		}
		else if (op == 0x7f)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "movdqu" : (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movdqa" : "movq"));
			*si.buffer++ = ' ';
			if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
			{
				_nmd_append_W(&si);
				*si.buffer++ = ',';
				_nmd_append_Vdq(&si);
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_Nq(&si);
				else
					_nmd_append_modrm_upper(&si, "qword");
				*si.buffer++ = ',';
				_nmd_append_Pq(&si);
			}
		}
		else if (_NMD_R(op) == 9)
		{
			_nmd_append_string(&si, "set");
			_nmd_append_string(&si, _nmd_condition_suffixes[_NMD_C(op)]);
			*si.buffer++ = ' ';
			_nmd_append_Eb(&si);
		}
		else if ((_NMD_R(op) == 0xA || _NMD_R(op) == 0xB) && op % 8 == 3)
		{
			_nmd_append_string(&si, op == 0xa3 ? "bt" : (op == 0xb3 ? "btr" : (op == 0xab ? "bts" : "btc")));
			*si.buffer++ = ' ';
			_nmd_append_Ev(&si);
			*si.buffer++ = ',';
			_nmd_append_Gv(&si);
		}
		else if (_NMD_R(op) == 0xA && (op % 8 == 4 || op % 8 == 5))
		{
			_nmd_append_string(&si, op > 0xA8 ? "shrd" : "shld");
			*si.buffer++ = ' ';
			_nmd_append_Ev(&si);
			*si.buffer++ = ',';
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			if (op % 8 == 4)
				_nmd_append_number(&si, instruction->immediate);
			else
				_nmd_append_string(&si, "cl");
		}
		else if (op == 0xb4 || op == 0xb5)
		{
			_nmd_append_string(&si, op == 0xb4 ? "lfs " : "lgs ");
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_modrm_upper(&si, "fword");
		}
		else if (op == 0xbc || op == 0xbd)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? (op == 0xbc ? "tzcnt" : "lzcnt") : (op == 0xbc ? "bsf" : "bsr"));
			*si.buffer++ = ' ';
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_Ev(&si);
		}
		else if (op == 0xa6)
		{
			const char* mnemonics[] = { "montmul", "xsha1", "xsha256" };
			_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg]);
		}
		else if (op == 0xa7)
		{
			const char* mnemonics[] = { "xstorerng", "xcryptecb", "xcryptcbc", "xcryptctr", "xcryptcfb", "xcryptofb" };
			_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg]);
		}
		else if (op == 0xae)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					_nmd_append_string(&si, "pcommit");
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
				{
					_nmd_append_string(&si, "incsspd ");
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.rm]);
				}
				else
				{
					const char* mnemonics[] = { "rdfsbase", "rdgsbase", "wrfsbase", "wrgsbase", 0, "lfence", "mfence", "sfence" };
					_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg]);
				}
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				{
					_nmd_append_string(&si, instruction->modrm.fields.reg == 0b110 ? "clwb " : "clflushopt ");
					_nmd_append_modrm_upper(&si, "byte");
				}
				else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
				{
					_nmd_append_string(&si, instruction->modrm.fields.reg == 0b100 ? "ptwrite " : "clrssbsy ");
					_nmd_append_modrm_upper(&si, instruction->modrm.fields.reg == 0b100 ? "dword" : "qword");
				}
				else
				{
					const char* mnemonics[] = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr", "xsave", "xrstor", "xsaveopt", "clflush" };
					_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg]);
					*si.buffer++ = ' ';
					_nmd_append_modrm_upper(&si, "dword");
				}
			}
		}
		else if (op == 0xaf)
		{
			_nmd_append_string(&si, "imul ");
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_Ev(&si);
		}
		else if (op == 0xb0 || op == 0xb1)
		{
			_nmd_append_string(&si, "cmpxchg ");
			if (op == 0xb0)
			{
				_nmd_append_Eb(&si);
				*si.buffer++ = ',';
				_nmd_append_Gb(&si);
			}
			else
			{
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_Gv(&si);
			}
		}
		else if (op == 0xb2)
		{
			_nmd_append_string(&si, "lss ");
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_modrm_upper(&si, "fword");
		}
		else if (_NMD_R(op) == 0xb && (op % 8) >= 6)
		{
			_nmd_append_string(&si, op > 0xb8 ? "movsx " : "movzx ");
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			if ((op % 8) == 6)
				_nmd_append_Eb(&si);
			else
				_nmd_append_Ew(&si);
		}
		else if (op == 0xb8)
		{
			_nmd_append_string(&si, "popcnt ");
			_nmd_append_Gv(&si);
			*si.buffer++ = ',';
			_nmd_append_Ev(&si);
		}
		else if (op == 0xba)
		{
			const char* mnemonics[] = { "bt","bts","btr","btc" };
			_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg - 4]);
			*si.buffer++ = ' ';
			_nmd_append_Ev(&si);
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0xc0 || op == 0xc1)
		{
			_nmd_append_string(&si, "xadd ");
			if (op == 0xc0)
			{
				_nmd_append_Eb(&si);
				*si.buffer++ = ',';
				_nmd_append_Gb(&si);
			}
			else
			{
				_nmd_append_Ev(&si);
				*si.buffer++ = ',';
				_nmd_append_Gv(&si);
			}
		}
		else if (op == 0xc2)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "cmppd" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "cmpss" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "cmpsd" : "cmpps")));
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				_nmd_append_Udq(&si);
			else
				_nmd_append_modrm_upper(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "dword" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "qword" : "xmmword"));
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0xc3)
		{
			_nmd_append_string(&si, "movnti ");
			_nmd_append_modrm_upper(&si, "dword");
			*si.buffer++ = ',';
			_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
		}
		else if (op == 0xc4)
		{
			_nmd_append_string(&si, "pinsrw ");
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				_nmd_append_Vdq(&si);
			else
				_nmd_append_Pq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				_nmd_append_string(&si, _nmd_reg32[si.instruction->modrm.fields.rm]);
			else
				_nmd_append_modrm_upper(&si, "word");
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0xc5)
		{
			_nmd_append_string(&si, "pextrw ");
			_nmd_append_string(&si, _nmd_reg32[si.instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				_nmd_append_Udq(&si);
			else
				_nmd_append_Nq(&si);
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0xc6)
		{
			_nmd_append_string(&si, "shufp");
			*si.buffer++ = instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_W(&si);
			*si.buffer++ = ',';
			_nmd_append_number(&si, instruction->immediate);
		}
		else if (op == 0xC7)
		{
			if (instruction->modrm.fields.reg == 0b001)
			{
				_nmd_append_string(&si, "cmpxchg8b ");
				_nmd_append_modrm_upper(&si, "qword");
			}
			else if (instruction->modrm.fields.reg <= 0b101)
			{
				const char* mnemonics[] = { "xrstors", "xsavec", "xsaves" };
				_nmd_append_string(&si, mnemonics[instruction->modrm.fields.reg - 3]);
				*si.buffer++ = ' ';
				_nmd_append_Eb(&si);
			}
			else if (instruction->modrm.fields.reg == 0b110)
			{
				if (instruction->modrm.fields.mod == 0b11)
				{
					_nmd_append_string(&si, "rdrand ");
					_nmd_append_Rv(&si);
				}
				else
				{
					_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "vmclear" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "vmxon" : "vmptrld"));
					*si.buffer++ = ' ';
					_nmd_append_modrm_upper(&si, "qword");
				}
			}
			else /* reg == 0b111 */
			{
				if (instruction->modrm.fields.mod == 0b11)
				{
					_nmd_append_string(&si, "rdseed ");
					_nmd_append_Rv(&si);
				}
				else
				{
					_nmd_append_string(&si, "vmptrst ");
					_nmd_append_modrm_upper(&si, "qword");
				}
			}
		}
		else if (op >= 0xc8 && op <= 0xcf)
		{
			_nmd_append_string(&si, "bswap ");
			_nmd_append_string(&si, (opszprfx ? _nmd_reg16 : _nmd_reg32)[op % 8]);
		}
		else if (op == 0xd0)
		{
			_nmd_append_string(&si, "addsubp");
			*si.buffer++ = instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_W(&si);
		}
		else if (op == 0xd6)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movq" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "movq2dq" : "movdq2q"));
			*si.buffer++ = ' ';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT)
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_Nq(&si);
			}
			else if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Udq(&si);
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					_nmd_append_Udq(&si);
				else
					_nmd_append_modrm_upper(&si, "qword");
				*si.buffer++ = ',';
				_nmd_append_Vdq(&si);
			}
		}
		else if (op == 0xd7)
		{
			_nmd_append_string(&si, "pmovmskb ");
			_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				_nmd_append_Udq(&si);
			else
				_nmd_append_Nq(&si);
		}
		else if (op == 0xe6)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "cvttpd2dq" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "cvtdq2pd" : "cvtpd2dq"));
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				_nmd_append_Udq(&si);
			else
				_nmd_append_modrm_upper(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "qword" : "xmmword");
		}
		else if (op == 0xe7)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movntdq" : "movntq");
			*si.buffer++ = ' ';
			_nmd_append_modrm_upper(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "xmmword" : "qword");
			*si.buffer++ = ',';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				_nmd_append_Vdq(&si);
			else
				_nmd_append_Pq(&si);
		}
		else if (op == 0xf0)
		{
			_nmd_append_string(&si, "lddqu ");
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_modrm_upper(&si, "xmmword");
		}
		else if (op == 0xf7)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "maskmovdqu " : "maskmovq ");
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_Udq(&si);
			}
			else
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Nq(&si);
			}
		}
		else if (op >= 0xd1 && op <= 0xfe)
		{
			const char* mnemonics[] = { "srlw", "srld", "srlq", "addq", "mullw", 0, 0, "subusb", "subusw", "minub", "and", "addusb", "addusw", "maxub", "andn", "avgb", "sraw", "srad", "avgw", "mulhuw", "mulhw", 0, 0, "subsb", "subsw", "minsw", "or", "addsb", "addsw", "maxsw", "xor", 0, "sllw", "slld", "sllq", "muludq", "maddwd", "sadbw", 0, "subb", "subw", "subd", "subq", "addb", "addw", "addd" };
			*si.buffer++ = 'p';
			_nmd_append_string(&si, mnemonics[op - 0xd1]);
			*si.buffer++ = ' ';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_W(&si);
			}
			else
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Qq(&si);
			}
		}
		else if (op == 0xb9 || op == 0xff)
		{
			_nmd_append_string(&si, op == 0xb9 ? "ud1 " : "ud0 ");
			_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->modrm.fields.mod == 0b11)
				_nmd_append_string(&si, (instruction->mode == NMD_X86_MODE_64 ? _nmd_reg64 : _nmd_reg32)[instruction->modrm.fields.rm]);
			else
				_nmd_append_modrm_upper(&si, "dword");
		}
		else
		{
			const char* str = 0;
			switch (op)
			{
			case 0x31: str = "rdtsc"; break;
			case 0x07: str = "sysret"; break;
			case 0x06: str = "clts"; break;
			case 0x08: str = "invd"; break;
			case 0x09: str = "wbinvd"; break;
			case 0x0b: str = "ud2"; break;
			case 0x0e: str = "femms"; break;
			case 0x30: str = "wrmsr"; break;
			case 0x32: str = "rdmsr"; break;
			case 0x33: str = "rdpmc"; break;
			case 0x34: str = "sysenter"; break;
			case 0x35: str = "sysexit"; break;
			case 0x37: str = "getsec"; break;
			case 0x77: str = "emms"; break;
			case 0xa0: str = "push fs"; break;
			case 0xa1: str = "pop fs"; break;
			case 0xa8: str = "push gs"; break;
			case 0xa9: str = "pop gs"; break;
			case 0xaa: str = "rsm"; break;
			default: return;
			}
			_nmd_append_string(&si, str);
		}
	}
	else if (instruction->opcode_map == NMD_X86_OPCODE_MAP_0F38)
	{
		if ((_NMD_R(op) == 2 || _NMD_R(op) == 3) && _NMD_C(op) <= 5)
		{
			const char* mnemonics[] = { "pmovsxbw", "pmovsxbd", "pmovsxbq", "pmovsxwd", "pmovsxwq", "pmovsxdq" };
			_nmd_append_string(&si, mnemonics[_NMD_C(op)]);
			if (_NMD_R(op) == 3)
				*(si.buffer - 4) = 'z';
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			if (instruction->modrm.fields.mod == 0b11)
				_nmd_append_Udq(&si);
			else
				_nmd_append_modrm_upper(&si, _NMD_C(op) == 5 ? "qword" : (_NMD_C(op) % 3 == 0 ? "qword" : (_NMD_C(op) % 3 == 1 ? "dword" : "word")));
		}
		else if (op >= 0x80 && op <= 0x83)
		{
			_nmd_append_string(&si, op == 0x80 ? "invept" : (op == 0x81 ? "invvpid" : "invpcid"));
			*si.buffer++ = ' ';
			_nmd_append_Gy(&si);
			*si.buffer++ = ',';
			_nmd_append_modrm_upper(&si, "xmmword");
		}
		else if (op >= 0xc8 && op <= 0xcd)
		{
			const char* mnemonics[] = { "sha1nexte", "sha1msg1", "sha1msg2", "sha256rnds2", "sha256msg1", "sha256msg2" };
			_nmd_append_string(&si, mnemonics[op - 0xc8]);
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_W(&si);
		}
		else if (op == 0xcf)
		{
			_nmd_append_string(&si, "gf2p8mulb ");
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			_nmd_append_W(&si);
		}
		else if (op == 0xf0 || op == 0xf1)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "crc32" : "movbe");
			*si.buffer++ = ' ';
			if (op == 0xf0)
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
				{
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					_nmd_append_Eb(&si);
				}
				else
				{
					_nmd_append_Gv(&si);
					*si.buffer++ = ',';
					_nmd_append_Ev(&si);
				}
			}
			else
			{
				if (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
				{
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						_nmd_append_Ew(&si);
					else
						_nmd_append_Ey(&si);
				}
				else
				{
					_nmd_append_Ev(&si);
					*si.buffer++ = ',';
					_nmd_append_Gv(&si);
				}
			}
		}
		else if (op == 0xf6)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "adcx" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "adox" : (instruction->rex_w_prefix ? "wrssq" : "wrssd")));
			*si.buffer++ = ' ';
			if (!instruction->simd_prefix)
			{
				_nmd_append_Ey(&si);
				*si.buffer++ = ',';
				_nmd_append_Gy(&si);
			}
			else
			{
				_nmd_append_Gy(&si);
				*si.buffer++ = ',';
				_nmd_append_Ey(&si);
			}
		}
		else if (op == 0xf5)
		{
			_nmd_append_string(&si, instruction->rex_w_prefix ? "wrussq " : "wrussd ");
			_nmd_append_modrm_upper(&si, instruction->rex_w_prefix ? "qword" : "dword");
			*si.buffer++ = ',';
			_nmd_append_string(&si, (instruction->rex_w_prefix ? _nmd_reg64 : _nmd_reg32)[instruction->modrm.fields.reg]);
		}
		else if (op == 0xf8)
		{
			_nmd_append_string(&si, instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movdir64b" : (instruction->simd_prefix == NMD_X86_PREFIXES_REPEAT ? "enqcmd" : "enqcmds"));
			*si.buffer++ = ' ';
			_nmd_append_string(&si, (instruction->mode == NMD_X86_MODE_64 ? _nmd_reg64 : (instruction->mode == NMD_X86_MODE_16 ? _nmd_reg16 : _nmd_reg32))[instruction->modrm.fields.rm]);
			*si.buffer++ = ',';
			_nmd_append_modrm_upper(&si, "zmmword");
		}
		else if (op == 0xf9)
		{
			_nmd_append_string(&si, "movdiri ");
			_nmd_append_modrm_upper_without_address_specifier(&si);
			*si.buffer++ = ',';
			_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.rm]);
		}
		else
		{
			if (op == 0x40)
				_nmd_append_string(&si, "pmulld");
			else if (op == 0x41)
				_nmd_append_string(&si, "phminposuw");
			else if (op >= 0xdb && op <= 0xdf)
			{
				const char* mnemonics[] = { "aesimc", "aesenc", "aesenclast", "aesdec", "aesdeclast" };
				_nmd_append_string(&si, mnemonics[op - 0xdb]);
			}
			else if (op == 0x37)
				_nmd_append_string(&si, "pcmpgtq");
			else if (_NMD_R(op) == 2)
			{
				const char* mnemonics[] = { "pmuldq", "pcmpeqq", "movntdqa", "packusdw" };
				_nmd_append_string(&si, mnemonics[_NMD_C(op) - 8]);
			}
			else if (_NMD_R(op) == 3)
			{
				const char* mnemonics[] = { "pminsb", "pminsd", "pminuw", "pminud", "pmaxsb", "pmaxsd", "pmaxuw", "pmaxud" };
				_nmd_append_string(&si, mnemonics[_NMD_C(op) - 8]);
			}
			else if (op < 0x10)
			{
				const char* mnemonics[] = { "pshufb", "phaddw", "phaddd", "phaddsw", "pmaddubsw", "phsubw", "phsubd", "phsubsw", "psignb", "psignw", "psignd", "pmulhrsw", "permilpsv", "permilpdv", "testpsv", "testpdv" };
				_nmd_append_string(&si, mnemonics[op]);
			}
			else if (op < 0x18)
				_nmd_append_string(&si, op == 0x10 ? "pblendvb" : (op == 0x14 ? "blendvps" : (op == 0x15 ? "blendvpd" : "ptest")));
			else
			{
				_nmd_append_string(&si, "pabs");
				*si.buffer++ = op == 0x1c ? 'b' : (op == 0x1d ? 'w' : 'd');
			}
			*si.buffer++ = ' ';
			if (instruction->simd_prefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				_nmd_append_W(&si);
			}
			else
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Qq(&si);
			}
		}

	}
	else if (instruction->opcode_map == NMD_X86_OPCODE_MAP_0F3A)
	{
		if (_NMD_R(op) == 1)
		{
			const char* mnemonics[] = { "pextrb", "pextrw", "pextrd", "extractps" };
			_nmd_append_string(&si, mnemonics[op - 0x14]);
			*si.buffer++ = ' ';
			if (instruction->modrm.fields.mod == 0b11)
				_nmd_append_string(&si, (si.instruction->rex_w_prefix ? _nmd_reg64 : _nmd_reg32)[instruction->modrm.fields.rm]);
			else
			{
				if (op == 0x14)
					_nmd_append_modrm_upper(&si, "byte");
				else if (op == 0x15)
					_nmd_append_modrm_upper(&si, "word");
				else if (op == 0x16)
					_nmd_append_Ey(&si);
				else
					_nmd_append_modrm_upper(&si, "dword");
			}
			*si.buffer++ = ',';
			_nmd_append_Vdq(&si);
		}
		else if (_NMD_R(op) == 2)
		{
			_nmd_append_string(&si, op == 0x20 ? "pinsrb" : (op == 0x21 ? "insertps" : "pinsrd"));
			*si.buffer++ = ' ';
			_nmd_append_Vdq(&si);
			*si.buffer++ = ',';
			if (op == 0x20)
			{
				if (instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, _nmd_reg32[instruction->modrm.fields.rm]);
				else
					_nmd_append_modrm_upper(&si, "byte");
			}
			else if (op == 0x21)
			{
				if (instruction->modrm.fields.mod == 0b11)
					_nmd_append_Udq(&si);
				else
					_nmd_append_modrm_upper(&si, "dword");
			}
			else
				_nmd_append_Ey(&si);
		}
		else
		{
			if (op < 0x10)
			{
				const char* mnemonics[] = { "roundps", "roundpd", "roundss", "roundsd", "blendps", "blendpd", "pblendw", "palignr" };
				_nmd_append_string(&si, mnemonics[op - 8]);
			}
			else if (_NMD_R(op) == 4)
			{
				const char* mnemonics[] = { "dpps", "dppd", "mpsadbw", 0, "pclmulqdq" };
				_nmd_append_string(&si, mnemonics[_NMD_C(op)]);
			}
			else if (_NMD_R(op) == 6)
			{
				const char* mnemonics[] = { "pcmpestrm", "pcmpestri", "pcmpistrm", "pcmpistri" };
				_nmd_append_string(&si, mnemonics[_NMD_C(op)]);
			}
			else if (op > 0x80)
				_nmd_append_string(&si, op == 0xcc ? "sha1rnds4" : (op == 0xce ? "gf2p8affineqb" : (op == 0xcf ? "gf2p8affineinvqb" : "aeskeygenassist")));
			*si.buffer++ = ' ';
			if (op == 0xf && !(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
			{
				_nmd_append_Pq(&si);
				*si.buffer++ = ',';
				_nmd_append_Qq(&si);
			}
			else
			{
				_nmd_append_Vdq(&si);
				*si.buffer++ = ',';
				if (instruction->modrm.fields.mod == 0b11)
					_nmd_append_string(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
				else
					_nmd_append_modrm_upper(&si, op == 0xa ? "dword" : (op == 0xb ? "qword" : "xmmword"));
			}
		}
		*si.buffer++ = ',';
		_nmd_append_number(&si, instruction->immediate);
	}

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX
	if (flags & NMD_X86_FORMAT_FLAGS_ATT_SYNTAX)
	{
		*si.buffer = '\0';
		char* operand = (char*)_nmd_reverse_strchr(buffer, ' ');
		if (operand && *(operand - 1) != ' ') /* If the instruction has a ' '(space character) and the left character of 'operand' is not ' '(space) the instruction has operands. */
		{
			/* If there is a memory operand. */
			const char* memory_operand = _nmd_strchr(buffer, '[');
			if (memory_operand)
			{
				/* If the memory operand has pointer size. */
				char* tmp2 = (char*)memory_operand - (*(memory_operand - 1) == ':' ? 7 : 4);
				if (_nmd_strstr(tmp2, "ptr") == tmp2)
				{
					/* Find the ' '(space) that is after two ' '(spaces). */
					tmp2 -= 2;
					while (*tmp2 != ' ')
						tmp2--;
					operand = tmp2;
				}
			}

			const char* const first_operand_const = operand;
			char* first_operand = operand + 1;
			char* second_operand = 0;
			/* Convert each operand to AT&T syntax. */
			do
			{
				operand++;
				operand = _nmd_format_operand_to_att(operand, &si);
				if (*operand == ',')
					second_operand = operand;
			} while (*operand);

			/* Swap operands. */
			if (second_operand) /* At least two operands. */
			{
				/* Copy first operand to 'tmp_buffer'. */
				char tmp_buffer[64];
				char* i = tmp_buffer;
				char* j = first_operand;
				for (; j < second_operand; i++, j++)
					*i = *j;

				*i = '\0';

				/* Copy second operand to first operand. */
				for (i = second_operand + 1; *i; first_operand++, i++)
					*first_operand = *i;

				*first_operand++ = ',';

				/* 'first_operand' is now the second operand. */
				/* Copy 'tmp_buffer' to second operand. */
				for (i = tmp_buffer; *first_operand; i++, first_operand++)
					*first_operand = *i;
			}

			/* Memory operands change the mnemonic string(e.g. 'mov eax, dword ptr [ebx]' -> 'movl (%ebx), %eax'). */
			if (memory_operand && !_nmd_strstr(first_operand_const - 4, "lea"))
			{
				const char* r_char = _nmd_strchr(first_operand_const, 'r');
				const char* e_char = _nmd_strchr(first_operand_const, 'e');
				const char* call_str = _nmd_strstr(first_operand_const - 5, "call");
				const char* jmp_str = _nmd_strstr(first_operand_const - 4, "jmp");
				_nmd_insert_char(first_operand_const, (instruction->mode == NMD_X86_MODE_64 && ((r_char && *(r_char - 1) == '%') || call_str || jmp_str)) ? 'q' : (instruction->mode == NMD_X86_MODE_32 && ((e_char && *(e_char - 1) == '%') || call_str || jmp_str) ? 'l' : 'b'));
				si.buffer++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX */

	size_t string_length = si.buffer - buffer;
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE
	if (flags & NMD_X86_FORMAT_FLAGS_UPPERCASE)
	{
		size_t i = 0;
		for (; i < string_length; i++)
		{
			if (_NMD_IS_LOWERCASE(buffer[i]))
				buffer[i] -= 0x20; /* Capitalize letter. */
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE */

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES
	if (flags & NMD_X86_FORMAT_FLAGS_COMMA_SPACES)
	{
		size_t i = 0;
		for (; i < string_length; i++)
		{
			if (buffer[i] == ',')
			{
				/* Move all characters after the comma one position to the right. */
				size_t j = string_length;
				for (; j > i; j--)
					buffer[j] = buffer[j - 1];

				buffer[i + 1] = ' ';
				si.buffer++, string_length++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES */

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES
	if (flags & NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES)
	{
		size_t i = 0;
		for (; i < string_length; i++)
		{
			if (buffer[i] == '+' || (buffer[i] == '-' && buffer[i - 1] != ' ' && buffer[i - 1] != '('))
			{
				/* Move all characters after the operator two positions to the right. */
				size_t j = string_length + 1;
				for (; j > i; j--)
					buffer[j] = buffer[j - 2];

				buffer[i + 1] = buffer[i];
				buffer[i] = ' ';
				buffer[i + 2] = ' ';
				si.buffer += 2, string_length += 2;
				i++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES */

	*si.buffer = '\0';
}

#pragma warning( pop )

#endif /* NMD_ASSEMBLY_IMPLEMENTATION */