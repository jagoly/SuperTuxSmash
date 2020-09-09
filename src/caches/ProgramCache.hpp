#pragma once

#include "setup.hpp"

#include <sqee/gl/Program.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

namespace sq { class PreProcessor; }

namespace sts {

using ProgramHandle = sq::Handle<sq::Program>;

class ProgramCache final : public sq::ResourceCache<sq::ProgramKey, sq::Program>
{
public: //====================================================//

    ProgramCache(const sq::PreProcessor& processor);

    ~ProgramCache() override;

private: //===================================================//

    std::unique_ptr<sq::Program> create(const sq::ProgramKey& key) override;

    const sq::PreProcessor& mProcessor;
};

} // namespace sts
