#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace UVE::Asset {

enum class DataTableColumnTypeUVE : std::uint8_t {
    Boolean = 0,
    Integer,
    Number,
    String,
};

using DataTableValueUVE = std::variant<bool, std::int64_t, double, std::string>;

struct DataTableColumnUVE final {
    std::string name;
    DataTableColumnTypeUVE type = DataTableColumnTypeUVE::String;
    [[nodiscard]] bool operator==(const DataTableColumnUVE&) const = default;
};

struct DataTableRowUVE final {
    std::string identifier;
    std::vector<DataTableValueUVE> values;
    [[nodiscard]] bool operator==(const DataTableRowUVE&) const = default;
};

enum class DataTableDiagnosticCodeUVE : std::uint8_t {
    InvalidDocument = 0,
    InvalidHeader,
    HeaderMismatch,
    InvalidRow,
    DuplicateRow,
    InvalidValue,
    ValueOutOfBounds,
    LimitExceeded,
};

struct DataTableDiagnosticUVE final {
    DataTableDiagnosticCodeUVE code = DataTableDiagnosticCodeUVE::InvalidDocument;
    std::size_t line = 0U;
    std::size_t column = 0U;
    std::string message;
    [[nodiscard]] bool operator==(const DataTableDiagnosticUVE&) const = default;
};

struct DataTableSnapshotUVE final {
    std::uint64_t generation = 0U;
    std::string name;
    std::vector<DataTableColumnUVE> columns;
    std::vector<DataTableRowUVE> rows;
    std::vector<DataTableDiagnosticUVE> diagnostics;
    bool diagnosticsTruncated = false;
    [[nodiscard]] bool operator==(const DataTableSnapshotUVE&) const = default;
};

class DataTableCsvImporterUVE;
class DataTableTsvImporterUVE;
class DataTableJsonImporterUVE;

class DataTableUVE final {
public:
    static constexpr std::size_t kMaximumColumnsUVE = 64U;
    static constexpr std::size_t kMaximumRowsUVE = 4096U;
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 64U;
    static constexpr std::size_t kMaximumStringBytesUVE = 1024U;
    static constexpr std::size_t kMaximumDocumentBytesUVE = 4U * 1024U * 1024U;
    static constexpr std::size_t kMaximumDiagnosticsUVE = 128U;

    explicit DataTableUVE(std::string name = {});
    DataTableUVE(const DataTableUVE&) = default;
    DataTableUVE& operator=(const DataTableUVE&) = default;
    DataTableUVE(DataTableUVE&&) noexcept = default;
    DataTableUVE& operator=(DataTableUVE&&) noexcept = default;
    ~DataTableUVE() = default;

    [[nodiscard]] bool DefineColumnUVE(std::string name, DataTableColumnTypeUVE type);
    [[nodiscard]] bool AddRowUVE(std::string identifier, std::vector<DataTableValueUVE> values);
    [[nodiscard]] bool ClearRowsUVE() noexcept;
    [[nodiscard]] bool ImportCsvUVE(std::string_view document);
    [[nodiscard]] bool ImportTsvUVE(std::string_view document);
    [[nodiscard]] bool ImportJsonUVE(std::string_view document);
    [[nodiscard]] DataTableSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] const DataTableRowUVE* FindRowUVE(std::string_view identifier) const noexcept;

private:
    friend class DataTableCsvImporterUVE;
    friend class DataTableTsvImporterUVE;
    friend class DataTableJsonImporterUVE;

    [[nodiscard]] static bool IsBoundedIdentifierUVE(std::string_view value) noexcept;
    [[nodiscard]] static bool IsValueCompatibleUVE(const DataTableValueUVE& value,
                                                    DataTableColumnTypeUVE type) noexcept;
    [[nodiscard]] bool ValidateRowUVE(std::string_view identifier,
                                      const std::vector<DataTableValueUVE>& values) const noexcept;
    void SetDiagnosticsUVE(std::vector<DataTableDiagnosticUVE> diagnostics, bool truncated) noexcept;
    void IncrementGenerationUVE() noexcept;

    std::string m_name;
    std::vector<DataTableColumnUVE> m_columns;
    std::vector<DataTableRowUVE> m_rows;
    std::vector<DataTableDiagnosticUVE> m_diagnostics;
    bool m_diagnosticsTruncated = false;
    std::uint64_t m_generation = 1U;
};

class DataTableCsvImporterUVE final {
public:
    [[nodiscard]] static bool ImportUVE(std::string_view document, DataTableUVE& table);
    [[nodiscard]] static bool ImportDelimitedUVE(std::string_view document, char delimiter, DataTableUVE& table);
};

class DataTableTsvImporterUVE final {
public:
    [[nodiscard]] static bool ImportUVE(std::string_view document, DataTableUVE& table);
};

class DataTableJsonImporterUVE final {
public:
    [[nodiscard]] static bool ImportUVE(std::string_view document, DataTableUVE& table);
};

} // namespace UVE::Asset
