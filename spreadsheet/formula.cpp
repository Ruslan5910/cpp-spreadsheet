#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
    : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& exc) {
        throw FormulaException(exc.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> args = [&sheet](Position pos) -> double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            if (sheet.GetCell(pos) == nullptr) {
                return 0;
            }
            if (std::holds_alternative<double>(sheet.GetCell(pos)->GetValue())) {
                return std::get<double>(sheet.GetCell(pos)->GetValue());
            } 
            if (std::holds_alternative<std::string>(sheet.GetCell(pos)->GetValue())) {
                size_t position = 0;
                double result;
                try {
                    result = std::stod(std::get<std::string>(sheet.GetCell(pos)->GetValue()), &position);
                    if (position == std::get<std::string>(sheet.GetCell(pos)->GetValue()).length()) {
                        return result;
                    } else {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            }
            throw std::get<FormulaError>(sheet.GetCell(pos)->GetValue());
        };
        try {
            return ast_.Execute(args);
        } catch (const FormulaError& exception) {
            return exception;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> buff;
        for (const Position& pos : ast_.GetCells()) {
            buff.insert(pos);
        }
        std::vector<Position> result(buff.begin(), buff.end());
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
