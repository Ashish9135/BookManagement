// MT26117_ASHISH
#pragma once

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

enum class EBookFormat { PDF, EPUB, HTML };

class EBook : public ElectronicResource {
public:
    EBook(std::string id, std::string title, std::vector<std::string> authors,
          std::string isbn, EBookFormat format, bool drmProtected,
          std::string publisher, int year, Money pricePerSeat,
          std::string accessUrl,
          LicenseModel license = LicenseModel::AnnualSubscription,
          Money platformFee = Money{});

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    EBookFormat format() const { return format_; }
    bool drmProtected() const { return drmProtected_; }

    ResourceCategory category() const override {
        return ResourceCategory::EBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    EBookFormat format_;
    bool drmProtected_;
};

// An EBook is also a kind of book, so authors and ISBN are duplicated
// between Book and EBook. This could be avoided by extracting shared
// book metadata into a common base class or a reusable component.

const char* eBookFormatName(EBookFormat format);

}  // namespace bookmgmt

