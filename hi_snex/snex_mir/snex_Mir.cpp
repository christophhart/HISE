#define DEFINE_ID(x) static const Identifier x(#x);
#define REGISTER_INLINER(x) currentState->inlinerManager.registerInliner(#x, InlinerFunctions::x);
#define REGISTER_TYPE(X) currentState->instructionManager.registerInstruction(InstructionIds::X, InstructionParsers::X);

#include "src/mir.h"
#include "src/mir-gen.h"

#include "snex_MirHelpers.h"
#include "snex_MirState.h"
#include "snex_MirBuilder.h"

#include "snex_MirState.cpp"
#include "snex_MirHelpers.cpp"
#include "snex_MirInliners.cpp"
#include "snex_MirInstructions.cpp"

#include "snex_MirBuilder.cpp"

#include "snex_MirObject.cpp"

#undef REGISTER_TYPE
#undef REGISTER_INLINER
#undef DEFINE_ID

