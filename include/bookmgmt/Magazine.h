// MT26117_ASHISH
#pragma once

#include "bookmgmt/Journal.h"

namespace bookmgmt {

class Magazine : public Journal {
public:
    Magazine(std::string id, std::string title, std::string issn,
             int issuesPerYear, int subscriptionYears,
             std::string publisher, int year, Money unitPrice,
             Money postagePerIssue);

    Money postagePerIssue() const { return postagePerIssue_; }

    ResourceCategory category() const override {
        return ResourceCategory::Magazine;
    }

    Money costFor(int copies) const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    Money postagePerIssue_;
};

}  // namespace bookmgmt
