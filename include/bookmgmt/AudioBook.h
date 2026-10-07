// MT26117_ASHISH
#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class AudioBook : public Resource {
public:
    AudioBook(std::string id, std::string title, std::string publisher,
              int year, Money unitPrice, std::string narrator,
              int durationMinutes);

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

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
