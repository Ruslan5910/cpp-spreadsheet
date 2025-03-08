#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>
#include <optional>

class Sheet;

class Impl {
public:
    virtual ~Impl() = default;
    
    virtual std::string GetText() = 0;
    virtual CellInterface::Value GetValue() = 0;
    virtual std::vector<Position> GetDependentCells() const;
    virtual bool HasValue() const;
    virtual void InvalidateCache() const {}
};

class EmptyImpl : public Impl {
public:
    std::string GetText() override;
    CellInterface::Value GetValue();
};

class TextImpl : public Impl {
public:
    TextImpl(std::string text)
        : text_(text) {}

    std::string GetText() override;
    CellInterface::Value GetValue();
private:
    std::string text_;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text, SheetInterface& sheet_);

    std::string GetText();
    CellInterface::Value GetValue();

    std::vector<Position> GetDependentCells() const;

    void InvalidateCache() const;

    bool HasValue() const;
    
private:
    SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell() = default;
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;
    std::unordered_set<Cell*>& GetDependentCells();

private:
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_reference_;
    std::unordered_set<Cell*> dependent_cells_; // ячейки которые зависят
    std::unordered_set<Cell*> which_depends_on_; // от которых зависит
    bool FindCircularDependences(const Impl& impl_copy);
    void InvalidateDependentCellsCache() const;
};
