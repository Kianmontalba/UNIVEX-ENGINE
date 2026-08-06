// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/commandline/command_line_uve.h"

#include <cstddef>

namespace UVE::CommandLine {

namespace {
constexpr std::string_view kFlagPrefix = "--";
} // namespace

CommandLineUVE::CommandLineUVE(std::vector<std::string> args) {
    ParseUVE(args);
}

CommandLineUVE::CommandLineUVE(int argc, char** argv) {
    std::vector<std::string> args;
    if (argc > 1) {
        args.reserve(static_cast<std::size_t>(argc - 1));
        for (int index = 1; index < argc; ++index) {
            args.emplace_back(argv[index]);
        }
    }
    ParseUVE(args);
}

void CommandLineUVE::ParseUVE(const std::vector<std::string>& args) {
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& token = args[index];
        if (token.compare(0, kFlagPrefix.size(), kFlagPrefix) != 0) {
            continue; // bare token not preceded by a recognized flag - ignored
        }

        const std::string normalizedName = token.substr(kFlagPrefix.size());
        if (normalizedName.empty()) {
            continue; // bare "--" with no name - not a valid flag
        }

        const bool hasNextToken = (index + 1) < args.size();
        const bool nextTokenIsFlag =
            hasNextToken && args[index + 1].compare(0, kFlagPrefix.size(), kFlagPrefix) == 0;

        if (hasNextToken && !nextTokenIsFlag) {
            m_flags[normalizedName] = args[index + 1];
            ++index; // consume the value token
        } else {
            m_flags[normalizedName] = std::nullopt;
        }
    }
}

bool CommandLineUVE::HasFlagUVE(std::string_view name) const noexcept {
    return m_flags.find(std::string(name)) != m_flags.end();
}

std::string CommandLineUVE::GetValueUVE(std::string_view name, std::string_view defaultValue) const {
    const auto flagIt = m_flags.find(std::string(name));
    if (flagIt == m_flags.end() || !flagIt->second.has_value()) {
        return std::string(defaultValue);
    }
    return *flagIt->second;
}

} // namespace UVE::CommandLine
