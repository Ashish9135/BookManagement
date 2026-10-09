#include "bookmgmt/Journal.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string issn,
                 int issuesPerYear, int subscriptionYears,
                 std::string publisher, int year, Money unitPrice)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (subscriptionYears_ < 1) {
        throw std::invalid_argument("subscriptionYears must be >= 1");
    }
}

Money Journal::costFor(int copies) const {
    requirePositive(copies);

    Money cost = unitPrice() * (copies * subscriptionYears_);

    if (copies >= 10) {
        cost = Money::fromMinor(
            (cost.minorUnits() * 90 + 50) / 100);
    }

    return cost;
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt
