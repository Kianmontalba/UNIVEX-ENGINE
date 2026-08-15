#include "uve/asset/data_table_uve.h"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

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

[[nodiscard]] bool ParseCsvRecordsUVE(std::string_view document,
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
            if (character == ',') {
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
        if (character == ',') {
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
    if (!IsBoundedIdentifierUVE(name) || m_columns.size() >= kMaximumColumnsUVE ||
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

bool DataTableCsvImporterUVE::ImportUVE(const std::string_view document, DataTableUVE& table) {
    std::vector<DataTableDiagnosticUVE> diagnostics;
    bool diagnosticsTruncated = false;
    std::vector<CsvRecordUVE> records;
    if (!ParseCsvRecordsUVE(document, records, diagnostics, diagnosticsTruncated)) {
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

} // namespace UVE::Asset
