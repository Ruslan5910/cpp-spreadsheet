#include "cell.h"
#include "sheet.h"

std::vector<Position> Impl::GetDependentCells() const {
    return {};
}

bool Impl::HasValue() const {
    return false;
}

std::string EmptyImpl::GetText() {
    return "";
}
CellInterface::Value EmptyImpl::GetValue() {
    return 0.;
}

std::string TextImpl::GetText() {
    return text_;
}

CellInterface::Value TextImpl::GetValue() {
    if (text_[0] != ESCAPE_SIGN) {
        return text_;
    } else {
        return std::string(text_.begin() + 1, text_.end());
    }
}

FormulaImpl::FormulaImpl(std::string text,  SheetInterface& sheet)
: sheet_(sheet)
{
    formula_ = ParseFormula(text);
}

std::string FormulaImpl::GetText() {
    return "=" + formula_->GetExpression();
}

CellInterface::Value FormulaImpl::GetValue() {
    if (!cache_) {
        cache_ = formula_->Evaluate(sheet_);
    }
    auto new_cell = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(new_cell)) {
         return std::get<double>(new_cell);
    }
    return std::get<FormulaError>(new_cell);
}

std::vector<Position> FormulaImpl::GetDependentCells() const {
    return formula_->GetReferencedCells();
}

void FormulaImpl::InvalidateCache() const {
    cache_.reset();
}
    
bool FormulaImpl::HasValue() const {
    return cache_.has_value();
}

Cell::Cell(Sheet& sheet)
: sheet_reference_(sheet) {}

void Cell::Set(std::string text) {
    std::string result;
    if (text[0] == FORMULA_SIGN && text.size() > 1) {
        impl_ = std::make_unique<FormulaImpl>(std::string(text.begin() + 1, text.end()), sheet_reference_);

        if (FindCircularDependences(*impl_)) {
            throw CircularDependencyException("Circular Dependency");
        }

        for (auto ptr_cell : which_depends_on_) {
            ptr_cell->dependent_cells_.erase(this);
        }

        which_depends_on_.clear();
    
        for (Position cell_pos : impl_->GetDependentCells()) {
            if (sheet_reference_.GetCell(cell_pos) == nullptr) {
                sheet_reference_.SetCell(cell_pos, "");
            }
            which_depends_on_.insert(sheet_reference_.GetConcreteCell(cell_pos));
            sheet_reference_.GetConcreteCell(cell_pos)->GetDependentCells().insert(this);
        }
    } else if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else {
        impl_ = std::make_unique<TextImpl>(text);
    }
    InvalidateDependentCellsCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetDependentCells();
}

bool Cell::IsReferenced() const {
    return !dependent_cells_.empty();
}

bool DFSfunc(Cell* cell, std::unordered_set<Cell*>& visit, std::unordered_set<Cell*>& to_visit) {
    if (to_visit.find(cell) != to_visit.end()) {
        return true;
    }
    if (visit.find(cell) != visit.end()) {
        return false;
    }
    to_visit.insert(cell);
    visit.insert(cell);
    for (const auto& cell_ptr : cell->GetDependentCells()) {
        if (DFSfunc(cell_ptr, visit, to_visit)) {
            return true;
        }
    }
    to_visit.erase(cell);
    return false;
}

bool Cell::FindCircularDependences(const Impl& impl_copy) {
    std::unordered_set<Cell*> visit;
    std::unordered_set<Cell*> to_visit;
    to_visit.insert(this);
    for (const Position& cell_pos : impl_copy.GetDependentCells()) {
        if (DFSfunc(sheet_reference_.GetConcreteCell(cell_pos), visit, to_visit)) {
            return true;
        }
    }
    return false;
}

std::unordered_set<Cell*>& Cell::GetDependentCells() {
    return dependent_cells_;
}

void Cell::InvalidateDependentCellsCache() const {
    if (!dependent_cells_.empty()) {
        for (const auto& cell : dependent_cells_) {
            if (!cell->impl_->HasValue()) {
                continue;
            }
            impl_->InvalidateCache();
            cell->InvalidateDependentCellsCache();
        }
    }
}
