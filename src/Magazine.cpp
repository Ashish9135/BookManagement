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

    Money journalCost = Journal::costFor(copies);
    Money postageCost =
        postagePerIssue() * (issuesPerYear() * subscriptionYears() * copies);

    return journalCost + postageCost;
}

void Magazine::printDetails(std::ostream& os) const {
    Journal::printDetails(os);
    os << "  postage per issue: " << postagePerIssue_ << "\n";
}

}  // namespace bookmgmt
