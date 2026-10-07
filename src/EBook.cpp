// MT26117_ASHISH
#include "bookmgmt/EBook.h"
#include "bookmgmt/Book.h"
#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

const char* eBookFormatName(EBookFormat format) {
    switch (format) {
        case EBookFormat::PDF: return "PDF";
        case EBookFormat::EPUB: return "EPUB";
        case EBookFormat::HTML: return "HTML";
    }
    return "Unknown";
}

EBook::EBook(std::string id, std::string title, std::vector<std::string> authors,
             std::string isbn, EBookFormat format, bool drmProtected,
             std::string publisher, int year, Money pricePerSeat,
             std::string accessUrl, LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher),
                         year, pricePerSeat, std::move(accessUrl), license, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  format: " << eBookFormatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt
