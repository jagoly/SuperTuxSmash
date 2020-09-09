#include "caches/ProgramCache.hpp"

#include <sqee/app/PreProcessor.hpp>

using namespace sts;

ProgramCache::ProgramCache(const sq::PreProcessor& processor) : mProcessor(processor) {}

ProgramCache::~ProgramCache() = default;

std::unique_ptr<sq::Program> ProgramCache::create(const sq::ProgramKey& key)
{
    auto result = std::make_unique<sq::Program>();
    mProcessor.load_vertex(*result, key.vertexPath, key.vertexDefines);
    mProcessor.load_fragment(*result, key.fragmentPath, key.fragmentDefines);
    result->link_program_stages();
    return result;
}
