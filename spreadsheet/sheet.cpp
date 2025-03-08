#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& os, const CellInterface::Value& value) {
    std::visit([&os](const auto& val) {
        os << val;
    }, value);
    return os;
}

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (sheet_.size() <= static_cast<size_t>(pos.row)) {
        sheet_.resize(pos.row + 1);
    }
    if (sheet_[0].size() <= static_cast<size_t>(pos.col)) {
        for (auto& row_vec : sheet_) {
            row_vec.resize(pos.col + 1);
        }
    } else {
        for (auto& row_vec : sheet_) {
            row_vec.resize(sheet_[0].size());
        }        
    }
    if (sheet_[pos.row][pos.col] == nullptr) {
        sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    sheet_[pos.row][pos.col]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (GetConcreteCell(pos) == nullptr) {
        return;
    }
    if (GetConcreteCell(pos)->IsReferenced()) {
        sheet_[pos.row][pos.col] ->Clear();
    } else {
        sheet_[pos.row][pos.col] = nullptr;
    }
}

Size Sheet::GetPrintableSize() const {
    size_t max_width = 0;
    size_t max_height = 0;

    for (size_t i = 0; i < sheet_.size(); ++i) {
        for (size_t m = 0; m < sheet_[i].size(); ++m) {
            if (sheet_[i][m] != nullptr && sheet_[i][m]->GetText() != "") {
                max_width = std::max(max_width, m + 1);
                max_height = std::max(max_height, i + 1);
            }
        }
    }
    return {static_cast<int>(max_height), static_cast<int>(max_width)};
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int m = 0; m < size.rows; ++m) {
        for (int i = 0; i < size.cols; ++i) {
            if (sheet_[m][i]) {
                output << sheet_[m][i]->GetValue();
            }
            if (i != size.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int m = 0; m < size.rows; ++m) {
        for (int i = 0; i < size.cols; ++i) {
            if (sheet_[m][i]) {
                output << sheet_[m][i]->GetText();
            }
            if (i != size.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (sheet_.size() > static_cast<size_t>(pos.row)) {
        if (sheet_[0].size() > static_cast<size_t>(pos.col)) {
            return sheet_[pos.row][pos.col].get();
        }
    }
    return nullptr;
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (sheet_.size() > static_cast<size_t>(pos.row)) {
        if (sheet_[0].size() > static_cast<size_t>(pos.col)) {
            return sheet_[pos.row][pos.col].get();
        }
    }
    return nullptr;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
