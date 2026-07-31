//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/version_uve.h"

namespace UVE::Core {

std::string VersionUVE::ToStringUVE() const {
    return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch) +
           "+build." + std::to_string(build);
}

} // namespace UVE::Core
