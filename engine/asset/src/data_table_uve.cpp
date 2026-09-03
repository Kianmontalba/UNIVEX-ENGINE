#include "uve/asset/data_table_uve.h"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Asset {
namespace {

struct CsvFieldUVE final {
    std::string value;
    std::size_t line = 1U;
    std::size_t column = 1U;
};

using CsvRecordUVE = std::vector<CsvFieldUVE>;

void AddDiagnosticUVE(std::vector<DataTableDiagnosticUVE>& diagnostics,
                      bool& truncated,
                      const DataTableDiagnosticCodeUVE code,
                      const std::size_t line,
                      const std::size_t column,
                      std::string message) {
    if (diagnostics.size() >= DataTableUVE::kMaximumDiagnosticsUVE) {
        truncated = true;
        return;
    }
    diagnostics.push_back(DataTableDiagnosticUVE{code, line, column, std::move(message)});
}

[[nodiscard]] bool IsNewlineUVE(const std::string_view document, const std::size_t index) noexcept {
    return document[index] == '\n' || document[index] == '\r';
}

[[nodiscard]] std::size_t NewlineWidthUVE(const std::string_view document, const std::size_t index) noexcept {
    return document[index] == '\r' && index + 1U < document.size() && document[index + 1U] == '\n' ? 2U : 1U;
}

[[nodiscard]] bool ParseDelimitedRecordsUVE(std::string_view document,
                                      const char delimiter,
                                      std::vector<CsvRecordUVE>& records,
                                      std::vector<DataTableDiagnosticUVE>& diagnostics,
                                      bool& diagnosticsTruncated) {
    if (document.empty()) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidDocument, 1U, 1U,
                         "The CSV document is empty.");
        return false;
    }
    if (document.size() > DataTableUVE::kMaximumDocumentBytesUVE) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::LimitExceeded, 1U, 1U,
                         "The CSV document exceeds the supported byte limit.");
        return false;
    }

    CsvRecordUVE record;
    std::string field;
    std::size_t fieldLine = 1U;
    std::size_t fieldColumn = 1U;
    std::size_t line = 1U;
    std::size_t column = 1U;
    bool quoted = false;
    bool afterQuote = false;
    bool anyField = false;

    const auto pushField = [&]() {
        record.push_back(CsvFieldUVE{std::move(field), fieldLine, fieldColumn});
        field.clear();
        anyField = true;
    };
    const auto pushRecord = [&]() {
        if (!record.empty()) {
            records.push_back(std::move(record));
            record = CsvRecordUVE{};
        }
        anyField = false;
    };

    std::size_t index = 0U;
    while (index < document.size()) {
        const char character = document[index];
        if (quoted) {
            if (character == '"') {
                if (index + 1U < document.size() && document[index + 1U] == '"') {
                    field.push_back('"');
                    index += 2U;
                    column += 2U;
                    continue;
                }
                quoted = false;
                afterQuote = true;
                ++index;
                ++column;
                continue;
            }
            field.push_back(character);
            if (IsNewlineUVE(document, index)) {
                const std::size_t width = NewlineWidthUVE(document, index);
                index += width;
                ++line;
                column = 1U;
            } else {
                ++index;
                ++column;
            }
            continue;
        }

        if (afterQuote) {
            if (character == delimiter) {
                pushField();
                afterQuote = false;
                ++index;
                ++column;
                fieldLine = line;
                fieldColumn = column;
                continue;
            }
            if (IsNewlineUVE(document, index)) {
                pushField();
                afterQuote = false;
                pushRecord();
                const std::size_t width = NewlineWidthUVE(document, index);
                index += width;
                ++line;
                column = 1U;
                fieldLine = line;
                fieldColumn = column;
                continue;
            }
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidDocument,
                             line, column, "Unexpected characters follow a closing CSV quote.");
            return false;
        }

        if (character == '"' && field.empty()) {
            quoted = true;
            anyField = true;
            ++index;
            ++column;
            continue;
        }
        if (character == delimiter) {
            pushField();
            ++index;
            ++column;
            fieldLine = line;
            fieldColumn = column;
            continue;
        }
        if (IsNewlineUVE(document, index)) {
            pushField();
            pushRecord();
            const std::size_t width = NewlineWidthUVE(document, index);
            index += width;
            ++line;
            column = 1U;
            fieldLine = line;
            fieldColumn = column;
            continue;
        }
        field.push_back(character);
        anyField = true;
        ++index;
        ++column;
    }

    if (quoted) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidDocument,
                         line, column, "The CSV document ends inside a quoted field.");
        return false;
    }
    if (afterQuote || anyField || !field.empty() || !record.empty()) {
        pushField();
        pushRecord();
    }
    if (records.empty()) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidDocument,
                         1U, 1U, "The CSV document contains no records.");
        return false;
    }
    return true;
}

[[nodiscard]] bool ParseBooleanUVE(const std::string_view value, bool& result) noexcept {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

[[nodiscard]] bool ParseIntegerUVE(const std::string_view value, std::int64_t& result) noexcept {
    if (value.empty()) {
        return false;
    }
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc{} && end == value.data() + value.size();
}

[[nodiscard]] bool ParseNumberUVE(const std::string_view value, double& result) {
    if (value.empty() || value.size() > DataTableUVE::kMaximumStringBytesUVE) {
        return false;
    }
    std::string copy(value);
    char* end = nullptr;
    errno = 0;
    result = std::strtod(copy.c_str(), &end);
    if (errno == ERANGE || end != copy.c_str() + copy.size() || !std::isfinite(result)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool ConvertValueUVE(const CsvFieldUVE& field,
                                   const DataTableColumnUVE& column,
                                   DataTableValueUVE& value,
                                   std::vector<DataTableDiagnosticUVE>& diagnostics,
                                   bool& diagnosticsTruncated) {
    if (field.value.size() > DataTableUVE::kMaximumStringBytesUVE) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::ValueOutOfBounds,
                         field.line, field.column, "The field exceeds the supported string byte limit.");
        return false;
    }
    switch (column.type) {
        case DataTableColumnTypeUVE::Boolean: {
            bool parsed = false;
            if (!ParseBooleanUVE(field.value, parsed)) {
                AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidValue,
                                 field.line, field.column, "Expected lowercase true or false for column " + column.name + ".");
                return false;
            }
            value = parsed;
            return true;
        }
        case DataTableColumnTypeUVE::Integer: {
            std::int64_t parsed = 0;
            if (!ParseIntegerUVE(field.value, parsed)) {
                AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidValue,
                                 field.line, field.column, "Expected a signed 64-bit integer for column " + column.name + ".");
                return false;
            }
            value = parsed;
            return true;
        }
        case DataTableColumnTypeUVE::Number: {
            double parsed = 0.0;
            if (!ParseNumberUVE(field.value, parsed)) {
                AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidValue,
                                 field.line, field.column, "Expected a finite number for column " + column.name + ".");
                return false;
            }
            value = parsed;
            return true;
        }
        case DataTableColumnTypeUVE::String:
            value = field.value;
            return true;
    }
    return false;
}

} // namespace

DataTableUVE::DataTableUVE(std::string name) : m_name(std::move(name)) {
    if (m_name.size() > kMaximumIdentifierBytesUVE || (!m_name.empty() && !IsBoundedIdentifierUVE(m_name))) {
        m_name.clear();
    }
}

bool DataTableUVE::IsBoundedIdentifierUVE(const std::string_view value) noexcept {
    if (value.empty() || value.size() > kMaximumIdentifierBytesUVE) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](const char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.';
    });
}

bool DataTableUVE::IsValueCompatibleUVE(const DataTableValueUVE& value,
                                        const DataTableColumnTypeUVE type) noexcept {
    switch (type) {
        case DataTableColumnTypeUVE::Boolean:
            return std::holds_alternative<bool>(value);
        case DataTableColumnTypeUVE::Integer:
            return std::holds_alternative<std::int64_t>(value);
        case DataTableColumnTypeUVE::Number:
            return std::holds_alternative<double>(value) && std::isfinite(std::get<double>(value));
        case DataTableColumnTypeUVE::String:
            return std::holds_alternative<std::string>(value) &&
                   std::get<std::string>(value).size() <= kMaximumStringBytesUVE;
    }
    return false;
}

bool DataTableUVE::ValidateRowUVE(const std::string_view identifier,
                                  const std::vector<DataTableValueUVE>& values) const noexcept {
    if (!IsBoundedIdentifierUVE(identifier) || values.size() != m_columns.size() ||
        m_rows.size() >= kMaximumRowsUVE) {
        return false;
    }
    if (FindRowUVE(identifier) != nullptr) {
        return false;
    }
    for (std::size_t index = 0U; index < values.size(); ++index) {
        if (!IsValueCompatibleUVE(values[index], m_columns[index].type)) {
            return false;
        }
    }
    return true;
}

bool DataTableUVE::DefineColumnUVE(std::string name, const DataTableColumnTypeUVE type) {
    const bool validType = type == DataTableColumnTypeUVE::Boolean ||
                           type == DataTableColumnTypeUVE::Integer ||
                           type == DataTableColumnTypeUVE::Number ||
                           type == DataTableColumnTypeUVE::String;
    if (!validType || !IsBoundedIdentifierUVE(name) || m_columns.size() >= kMaximumColumnsUVE ||
        std::any_of(m_columns.begin(), m_columns.end(), [&name](const DataTableColumnUVE& column) {
            return column.name == name;
        })) {
        return false;
    }
    m_columns.push_back(DataTableColumnUVE{std::move(name), type});
    IncrementGenerationUVE();
    return true;
}

bool DataTableUVE::AddRowUVE(std::string identifier, std::vector<DataTableValueUVE> values) {
    if (!ValidateRowUVE(identifier, values)) {
        return false;
    }
    m_rows.push_back(DataTableRowUVE{std::move(identifier), std::move(values)});
    IncrementGenerationUVE();
    return true;
}

bool DataTableUVE::ClearRowsUVE() noexcept {
    if (m_rows.empty()) {
        return false;
    }
    m_rows.clear();
    IncrementGenerationUVE();
    return true;
}

const DataTableRowUVE* DataTableUVE::FindRowUVE(const std::string_view identifier) const noexcept {
    const auto iterator = std::find_if(m_rows.begin(), m_rows.end(), [identifier](const DataTableRowUVE& row) {
        return row.identifier == identifier;
    });
    return iterator == m_rows.end() ? nullptr : &*iterator;
}

void DataTableUVE::SetDiagnosticsUVE(std::vector<DataTableDiagnosticUVE> diagnostics, const bool truncated) noexcept {
    m_diagnostics = std::move(diagnostics);
    m_diagnosticsTruncated = truncated;
}

void DataTableUVE::IncrementGenerationUVE() noexcept {
    if (m_generation < std::numeric_limits<std::uint64_t>::max()) {
        ++m_generation;
    }
}

DataTableSnapshotUVE DataTableUVE::GetSnapshotUVE() const {
    return DataTableSnapshotUVE{m_generation, m_name, m_columns, m_rows, m_diagnostics, m_diagnosticsTruncated};
}

bool DataTableUVE::ImportCsvUVE(const std::string_view document) {
    return DataTableCsvImporterUVE::ImportUVE(document, *this);
}

bool DataTableUVE::ImportTsvUVE(const std::string_view document) {
    return DataTableTsvImporterUVE::ImportUVE(document, *this);
}

bool DataTableUVE::ImportJsonUVE(const std::string_view document) {
    return DataTableJsonImporterUVE::ImportUVE(document, *this);
}

bool DataTableCsvImporterUVE::ImportUVE(const std::string_view document, DataTableUVE& table) {
    return ImportDelimitedUVE(document, ',', table);
}

bool DataTableCsvImporterUVE::ImportDelimitedUVE(const std::string_view document, const char delimiter,
                                                  DataTableUVE& table) {
    if (delimiter == '\n' || delimiter == '\r' || delimiter == '"') {
        return false;
    }
    std::vector<DataTableDiagnosticUVE> diagnostics;
    bool diagnosticsTruncated = false;
    std::vector<CsvRecordUVE> records;
    if (!ParseDelimitedRecordsUVE(document, delimiter, records, diagnostics, diagnosticsTruncated)) {
        table.SetDiagnosticsUVE(std::move(diagnostics), diagnosticsTruncated);
        table.IncrementGenerationUVE();
        return false;
    }
    const CsvRecordUVE& header = records.front();
    if (header.size() != table.m_columns.size() + 1U || header.front().value != "id") {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::HeaderMismatch,
                         header.front().line, header.front().column,
                         "The CSV header must start with id and contain the schema columns in order.");
        table.SetDiagnosticsUVE(std::move(diagnostics), diagnosticsTruncated);
        table.IncrementGenerationUVE();
        return false;
    }
    for (std::size_t index = 0U; index < table.m_columns.size(); ++index) {
        if (header[index + 1U].value != table.m_columns[index].name) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::HeaderMismatch,
                             header[index + 1U].line, header[index + 1U].column,
                             "The CSV header does not match the declared schema at column " +
                                 table.m_columns[index].name + ".");
            table.SetDiagnosticsUVE(std::move(diagnostics), diagnosticsTruncated);
            table.IncrementGenerationUVE();
            return false;
        }
    }

    std::vector<DataTableRowUVE> importedRows;
    importedRows.reserve(records.size() - 1U);
    for (std::size_t recordIndex = 1U; recordIndex < records.size(); ++recordIndex) {
        const CsvRecordUVE& record = records[recordIndex];
        if (record.size() != table.m_columns.size() + 1U) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidRow,
                             record.empty() ? 1U : record.front().line,
                             record.empty() ? 1U : record.front().column,
                             "The CSV row does not contain exactly one id and one value per schema column.");
            continue;
        }
        if (!DataTableUVE::IsBoundedIdentifierUVE(record.front().value) ||
            std::any_of(importedRows.begin(), importedRows.end(), [&record](const DataTableRowUVE& row) {
                return row.identifier == record.front().value;
            })) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::DuplicateRow,
                             record.front().line, record.front().column,
                             "The CSV row identifier is invalid or duplicated.");
            continue;
        }
        std::vector<DataTableValueUVE> values;
        values.reserve(table.m_columns.size());
        bool rowValid = true;
        for (std::size_t columnIndex = 0U; columnIndex < table.m_columns.size(); ++columnIndex) {
            DataTableValueUVE value;
            if (!ConvertValueUVE(record[columnIndex + 1U], table.m_columns[columnIndex], value,
                                 diagnostics, diagnosticsTruncated)) {
                rowValid = false;
                continue;
            }
            values.push_back(std::move(value));
        }
        if (!rowValid) {
            continue;
        }
        if (importedRows.size() >= DataTableUVE::kMaximumRowsUVE) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::LimitExceeded,
                             record.front().line, record.front().column,
                             "The CSV document exceeds the maximum row limit.");
            break;
        }
        importedRows.push_back(DataTableRowUVE{record.front().value, std::move(values)});
    }

    if (!diagnostics.empty()) {
        table.SetDiagnosticsUVE(std::move(diagnostics), diagnosticsTruncated);
        table.IncrementGenerationUVE();
        return false;
    }
    table.m_rows = std::move(importedRows);
    table.SetDiagnosticsUVE({}, false);
    table.IncrementGenerationUVE();
    return true;
}

bool DataTableTsvImporterUVE::ImportUVE(const std::string_view document, DataTableUVE& table) {
    return DataTableCsvImporterUVE::ImportDelimitedUVE(document, '\t', table);
}

bool DataTableJsonImporterUVE::ImportUVE(const std::string_view document, DataTableUVE& table) {
    std::vector<DataTableDiagnosticUVE> diagnostics;
    bool diagnosticsTruncated = false;
    const auto fail = [&]() {
        table.SetDiagnosticsUVE(std::move(diagnostics), diagnosticsTruncated);
        table.IncrementGenerationUVE();
        return false;
    };
    if (document.empty() || document.size() > DataTableUVE::kMaximumDocumentBytesUVE) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::LimitExceeded, 1U, 1U,
                         document.empty() ? "The JSON document is empty." : "The JSON document exceeds the supported byte limit.");
        return fail();
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(document.begin(), document.end());
    } catch (const nlohmann::json::exception&) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidDocument, 1U, 1U,
                         "The JSON document is malformed.");
        return fail();
    }
    if (!root.is_array() || root.size() > DataTableUVE::kMaximumRowsUVE) {
        AddDiagnosticUVE(diagnostics, diagnosticsTruncated,
                         root.is_array() ? DataTableDiagnosticCodeUVE::LimitExceeded
                                         : DataTableDiagnosticCodeUVE::InvalidDocument,
                         1U, 1U, root.is_array() ? "The JSON document exceeds the maximum row limit."
                                                  : "The JSON root must be an array of row objects.");
        return fail();
    }

    std::vector<DataTableRowUVE> importedRows;
    importedRows.reserve(root.size());
    for (std::size_t rowIndex = 0U; rowIndex < root.size(); ++rowIndex) {
        const nlohmann::json& object = root[rowIndex];
        const std::size_t line = rowIndex + 1U;
        if (!object.is_object() || object.size() != table.m_columns.size() + 1U ||
            !object.contains("id") || !object.at("id").is_string()) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidRow,
                             line, 1U, "Each JSON row must contain exactly one string id and every schema column.");
            continue;
        }
        const std::string identifier = object.at("id").get<std::string>();
        if (!DataTableUVE::IsBoundedIdentifierUVE(identifier) ||
            std::any_of(importedRows.begin(), importedRows.end(), [&identifier](const DataTableRowUVE& row) {
                return row.identifier == identifier;
            })) {
            AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::DuplicateRow,
                             line, 1U, "The JSON row identifier is invalid or duplicated.");
            continue;
        }

        std::vector<DataTableValueUVE> values;
        values.reserve(table.m_columns.size());
        bool rowValid = true;
        for (const DataTableColumnUVE& column : table.m_columns) {
            if (!object.contains(column.name)) {
                AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidRow,
                                 line, 1U, "The JSON row is missing schema column " + column.name + ".");
                rowValid = false;
                continue;
            }
            const nlohmann::json& jsonValue = object.at(column.name);
            try {
                switch (column.type) {
                    case DataTableColumnTypeUVE::Boolean:
                        if (!jsonValue.is_boolean()) {
                            throw nlohmann::json::type_error::create(302, "expected boolean", &jsonValue);
                        }
                        values.emplace_back(jsonValue.get<bool>());
                        break;
                    case DataTableColumnTypeUVE::Integer:
                        if (!jsonValue.is_number_integer()) {
                            throw nlohmann::json::type_error::create(302, "expected signed integer", &jsonValue);
                        }
                        values.emplace_back(jsonValue.get<std::int64_t>());
                        break;
                    case DataTableColumnTypeUVE::Number: {
                        if (!jsonValue.is_number()) {
                            throw nlohmann::json::type_error::create(302, "expected number", &jsonValue);
                        }
                        const double value = jsonValue.get<double>();
                        if (!std::isfinite(value)) {
                            throw nlohmann::json::type_error::create(302, "expected finite number", &jsonValue);
                        }
                        values.emplace_back(value);
                        break;
                    }
                    case DataTableColumnTypeUVE::String: {
                        if (!jsonValue.is_string()) {
                            throw nlohmann::json::type_error::create(302, "expected string", &jsonValue);
                        }
                        const std::string value = jsonValue.get<std::string>();
                        if (value.size() > DataTableUVE::kMaximumStringBytesUVE) {
                            throw nlohmann::json::type_error::create(302, "string exceeds bound", &jsonValue);
                        }
                        values.emplace_back(value);
                        break;
                    }
                }
            } catch (const nlohmann::json::exception&) {
                AddDiagnosticUVE(diagnostics, diagnosticsTruncated, DataTableDiagnosticCodeUVE::InvalidValue,
                                 line, 1U, "The JSON value does not match schema column " + column.name + ".");
                rowValid = false;
            }
        }
        if (rowValid) {
            importedRows.push_back(DataTableRowUVE{identifier, std::move(values)});
        }
    }

    if (!diagnostics.empty()) {
        return fail();
    }
    table.m_rows = std::move(importedRows);
    table.SetDiagnosticsUVE({}, false);
    table.IncrementGenerationUVE();
    return true;
}

namespace {

[[nodiscard]] const char* ColumnTypeNameUVE(const DataTableColumnTypeUVE type) noexcept {
    switch (type) {
        case DataTableColumnTypeUVE::Boolean:
            return "boolean";
        case DataTableColumnTypeUVE::Integer:
            return "integer";
        case DataTableColumnTypeUVE::Number:
            return "number";
        case DataTableColumnTypeUVE::String:
            return "string";
    }
    return "";
}

[[nodiscard]] std::optional<DataTableColumnTypeUVE> ParseColumnTypeUVE(const std::string_view type) noexcept {
    if (type == "boolean") {
        return DataTableColumnTypeUVE::Boolean;
    }
    if (type == "integer") {
        return DataTableColumnTypeUVE::Integer;
    }
    if (type == "number") {
        return DataTableColumnTypeUVE::Number;
    }
    if (type == "string") {
        return DataTableColumnTypeUVE::String;
    }
    return std::nullopt;
}

[[nodiscard]] nlohmann::json SerializeValueUVE(const DataTableValueUVE& value) {
    nlohmann::json encoded = nlohmann::json::object();
    std::visit([&encoded](const auto& current) {
        using ValueType = std::decay_t<decltype(current)>;
        if constexpr (std::is_same_v<ValueType, bool>) {
            encoded["type"] = "boolean";
        } else if constexpr (std::is_same_v<ValueType, std::int64_t>) {
            encoded["type"] = "integer";
        } else if constexpr (std::is_same_v<ValueType, double>) {
            encoded["type"] = "number";
        } else {
            encoded["type"] = "string";
        }
        encoded["value"] = current;
    }, value);
    return encoded;
}

[[nodiscard]] bool IsCatalogNameUVE(const std::string_view name) noexcept {
    if (name.empty() || name.size() > DataTableUVE::kMaximumIdentifierBytesUVE) {
        return false;
    }
    return std::all_of(name.begin(), name.end(), [](const char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.';
    });
}

} // namespace

bool DataTableAssetSerializerUVE::SerializeUVE(const DataTableUVE& table, std::string& document) {
    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    if (!IsCatalogNameUVE(snapshot.name)) {
        return false;
    }
    nlohmann::json envelope = nlohmann::json::object();
    envelope["format"] = "uve.data_table";
    envelope["version"] = 1U;
    envelope["name"] = snapshot.name;
    envelope["columns"] = nlohmann::json::array();
    for (const DataTableColumnUVE& column : snapshot.columns) {
        envelope["columns"].push_back({{"name", column.name}, {"type", ColumnTypeNameUVE(column.type)}});
    }
    envelope["rows"] = nlohmann::json::array();
    for (const DataTableRowUVE& row : snapshot.rows) {
        nlohmann::json values = nlohmann::json::array();
        for (const DataTableValueUVE& value : row.values) {
            values.push_back(SerializeValueUVE(value));
        }
        envelope["rows"].push_back({{"id", row.identifier}, {"values", std::move(values)}});
    }
    const std::string serialized = envelope.dump();
    if (serialized.size() > DataTableUVE::kMaximumDocumentBytesUVE) {
        return false;
    }
    document = serialized;
    return true;
}

bool DataTableAssetSerializerUVE::DeserializeUVE(const std::string_view document, DataTableUVE& table) {
    if (document.empty() || document.size() > DataTableUVE::kMaximumDocumentBytesUVE) {
        return false;
    }
    try {
        const nlohmann::json envelope = nlohmann::json::parse(document.begin(), document.end());

        if (!envelope.is_object() || envelope.size() != 5U || envelope.value("format", "") != "uve.data_table" ||
            envelope.value("version", 0U) != 1U || !envelope.contains("name") || !envelope.contains("columns") ||
            !envelope.contains("rows") || !envelope.at("name").is_string() ||
            !envelope.at("columns").is_array() || !envelope.at("rows").is_array()) {
            return false;
        }
        const std::string name = envelope.at("name").get<std::string>();
        DataTableUVE candidate(name);
        if (candidate.GetSnapshotUVE().name != name || envelope.at("columns").size() > DataTableUVE::kMaximumColumnsUVE ||
            envelope.at("rows").size() > DataTableUVE::kMaximumRowsUVE) {
            return false;
        }
        for (const nlohmann::json& column : envelope.at("columns")) {
            if (!column.is_object() || column.size() != 2U || !column.contains("name") || !column.contains("type") ||
                !column.at("name").is_string() || !column.at("type").is_string()) {
                return false;
            }
            const std::optional<DataTableColumnTypeUVE> type = ParseColumnTypeUVE(column.at("type").get<std::string>());
            if (!type.has_value() || !candidate.DefineColumnUVE(column.at("name").get<std::string>(), *type)) {
                return false;
            }
        }
        for (const nlohmann::json& row : envelope.at("rows")) {
            if (!row.is_object() || row.size() != 2U || !row.contains("id") || !row.contains("values") ||
                !row.at("id").is_string() || !row.at("values").is_array() ||
                row.at("values").size() != candidate.GetSnapshotUVE().columns.size()) {
                return false;
            }
            std::vector<DataTableValueUVE> values;
            values.reserve(row.at("values").size());
            for (const nlohmann::json& encoded : row.at("values")) {
                if (!encoded.is_object() || encoded.size() != 2U || !encoded.contains("type") || !encoded.contains("value") ||
                    !encoded.at("type").is_string()) {
                    return false;
                }
                const std::string type = encoded.at("type").get<std::string>();
                const nlohmann::json& value = encoded.at("value");
                if (type == "boolean" && value.is_boolean()) {
                    values.emplace_back(value.get<bool>());
                } else if (type == "integer" && value.is_number_integer()) {
                    values.emplace_back(value.get<std::int64_t>());
                } else if (type == "number" && value.is_number() && std::isfinite(value.get<double>())) {
                    values.emplace_back(value.get<double>());
                } else if (type == "string" && value.is_string() && value.get<std::string>().size() <= DataTableUVE::kMaximumStringBytesUVE) {
                    values.emplace_back(value.get<std::string>());
                } else {
                    return false;
                }
            }
            if (!candidate.AddRowUVE(row.at("id").get<std::string>(), std::move(values))) {
                return false;
            }
        }
        table = std::move(candidate);
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool DataTableCatalogUVE::UpsertUVE(const DataTableSnapshotUVE& table) {
    if (!IsCatalogNameUVE(table.name)) {
        return false;
    }
    const DataTableCatalogEntryUVE entry{table.name, table.generation, table.columns.size(), table.rows.size(),
                                         table.diagnostics.empty()};
    const auto iterator = std::lower_bound(m_entries.begin(), m_entries.end(), entry.name,
                                           [](const DataTableCatalogEntryUVE& current, const std::string& name) {
                                               return current.name < name;
                                           });
    if (iterator != m_entries.end() && iterator->name == entry.name) {
        if (*iterator == entry) {
            return false;
        }
        *iterator = entry;
    } else {
        if (m_entries.size() >= kMaximumEntriesUVE) {
            return false;
        }
        m_entries.insert(iterator, entry);
    }
    IncrementGenerationUVE();
    return true;
}

bool DataTableCatalogUVE::RemoveUVE(const std::string_view name) {
    const auto iterator = std::lower_bound(m_entries.begin(), m_entries.end(), name,
                                           [](const DataTableCatalogEntryUVE& current, const std::string_view target) {
                                               return current.name < target;
                                           });
    if (iterator == m_entries.end() || iterator->name != name) {
        return false;
    }
    m_entries.erase(iterator);
    IncrementGenerationUVE();
    return true;
}

DataTableCatalogSnapshotUVE DataTableCatalogUVE::GetSnapshotUVE() const {
    return DataTableCatalogSnapshotUVE{m_generation, m_entries, false};
}

bool DataTableCatalogUVE::ContainsUVE(const std::string_view name) const noexcept {
    return std::any_of(m_entries.begin(), m_entries.end(), [name](const DataTableCatalogEntryUVE& entry) {
        return entry.name == name;
    });
}

void DataTableCatalogUVE::IncrementGenerationUVE() noexcept {
    if (m_generation < std::numeric_limits<std::uint64_t>::max()) {
        ++m_generation;
    }
}

} // namespace UVE::Asset