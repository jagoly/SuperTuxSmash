#include "Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

RenderFighter::RenderFighter(Renderer& renderer, const Fighter& fighter)
    : mRenderer(renderer), mFighter(fighter) {}
