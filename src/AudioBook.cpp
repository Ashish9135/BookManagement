// MT26117_ASHISH
#include "bookmgmt/AudioBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

AudioBook::AudioBook(std::string id, std::string title,
                     std::string publisher, int year, Money unitPrice,
                     std::string narrator, int durationMinutes)
    : Resource(std::move(id), std::move(title), std::move(publisher),
               year, unitPrice),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {}

Money AudioBook::costFor(int copies) const {
    requirePositive(copies);
    Money cost = unitPrice() * copies;
    if (copies >= 10) cost = Money::fromMinor((cost.minorUnits() * 90 + 50) / 100);
    return cost;
}

void AudioBook::printDetails(std::ostream& os) const {
    os << "  narrator: " << narrator_ << "\n"
       << "  duration minutes: " << durationMinutes_ << "\n";
}

}  // namespace bookmgmt
