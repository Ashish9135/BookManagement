// MT26117_ASHISH
#include "bookmgmt/Magazine.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

Magazine::Magazine(std::string id, std::string title, std::string issn,
                   int issuesPerYear, int subscriptionYears,
                   std::string publisher, int year, Money unitPrice,
                   Money postagePerIssue)
    : Journal(std::move(id), std::move(title), std::move(issn),
              issuesPerYear, subscriptionYears,
              std::move(publisher), year, unitPrice),
      postagePerIssue_(postagePerIssue) {}

Money Magazine::costFor(int copies) const {
    requirePositive(copies);

    Money totalCost =
        unitPrice() * (copies * subscriptionYears());

    Money postageCost =
        postagePerIssue() *
        (issuesPerYear() * subscriptionYears() * copies);

    totalCost = totalCost + postageCost;

    if (copies >= 10) {
        totalCost = Money::fromMinor(
            (totalCost.minorUnits() * 90 + 50) / 100);
    }

    return totalCost;
}

void Magazine::printDetails(std::ostream& os) const {
    Journal::printDetails(os);
    os << "  postage per issue: " << postagePerIssue_ << "\n";
}

}  // namespace bookmgmt
