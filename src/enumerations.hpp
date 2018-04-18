#include <sqee/setup.hpp>
#include <sqee/macros.hpp>

namespace sts {

//============================================================================//

enum class FighterEnum : int8_t
{
    Null = -1,
    Sara, Tux,
    Count
};

enum class StageEnum : int8_t
{
    Null = -1,
    TestZone,
    Count
};

//============================================================================//

#define ETSC SQEE_ENUM_TO_STRING_CASE

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(FighterEnum)
ETSC(Null) ETSC(Sara) ETSC(Tux) ETSC(Count)
SQEE_ENUM_TO_STRING_BLOCK_END

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(StageEnum)
ETSC(Null) ETSC(TestZone) ETSC(Count)
SQEE_ENUM_TO_STRING_BLOCK_END

#undef ETSC

//============================================================================//

} // namespace sts
