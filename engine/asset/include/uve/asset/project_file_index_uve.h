// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <filesystem>
#include <memory>

#include "uve/asset/i_project_file_index_uve.h"

namespace UVE::Asset {

/// Engine-standard cached, read-only project-content index. The configured
/// root is normalized once at construction; each successful RefreshUVE() builds
/// a complete replacement snapshot before publishing it to readers.
class ProjectFileIndexUVE final : public IProjectFileIndexUVE {
public:
    explicit ProjectFileIndexUVE(std::filesystem::path contentRoot);
    ~ProjectFileIndexUVE() override;

    ProjectFileIndexUVE(const ProjectFileIndexUVE&) = delete;
    ProjectFileIndexUVE& operator=(const ProjectFileIndexUVE&) = delete;
    ProjectFileIndexUVE(ProjectFileIndexUVE&&) = delete;
    ProjectFileIndexUVE& operator=(ProjectFileIndexUVE&&) = delete;

    bool RefreshUVE(const IAssetDatabaseUVE& assetDatabase) override;
    [[nodiscard]] ProjectFileSnapshotUVE GetSnapshotUVE() const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
