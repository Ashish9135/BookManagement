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

void AudioBook::printDetails(std::ostream& os) const {
    os << "  narrator: " << narrator_ << "\n"
       << "  duration minutes: " << durationMinutes_ << "\n";
}

}  // namespace bookmgmt
