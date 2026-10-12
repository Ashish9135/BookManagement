// MT26117_ASHISH
#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

// AudioBook derives from Resource because it is a separately priced media item,
// not a printed Book; it still uses the common catalogue and acquisition API.
class AudioBook : public Resource {
public:
    AudioBook(std::string id, std::string title, std::string publisher,
              int year, Money unitPrice, std::string narrator,
              int durationMinutes);

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

    Money costFor(int copies) const override;

    ResourceCategory category() const override {
        return ResourceCategory::AudioBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt
