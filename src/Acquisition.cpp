#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget, int printTaxPercent, int electronicTaxPercent)
    : catalog_(catalog), budget_(budget), printTaxPercent_(printTaxPercent), electronicTaxPercent_(electronicTaxPercent) {}

void AcquisitionManager::setTitleLimit(ResourceCategory category, int limit) {
    if (limit < 0) throw std::invalid_argument("title limit must not be negative");
    titleLimits_[category] = limit;
}

std::optional<int> AcquisitionManager::titleLimitFor(ResourceCategory category) const {
    auto it = titleLimits_.find(category);
    if (it == titleLimits_.end()) return std::nullopt;
    return it->second;
}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

Money AcquisitionManager::taxFor(const std::string& id, int quantity) const {
    const Resource& resource = catalog_.get(id);
    const Money cost = resource.costFor(quantity);
    const int rate = resource.isDigital() ? electronicTaxPercent_ : printTaxPercent_;
    return Money::fromMinor((cost.minorUnits() * rate + 50) / 100);
}

Money AcquisitionManager::totalWithTax(const std::string& id, int quantity) const {
    return quote(id, quantity) + taxFor(id, quantity);

}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    std::string why;
    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            bool titleExists = false;
            for (const auto& rec : history_) {
                if (rec.approved && rec.category == r->category() && rec.title == r->title()) {
                    titleExists = true;
                    break;
                }
            }
            int differentTitles = 0;
            for (const auto& rec : history_) {
                if (rec.approved && rec.category == r->category()) {
                    bool firstOccurrence = true;
                    for (const auto& earlier : history_) {
                        if (earlier.orderNo >= rec.orderNo) break;
                        if (earlier.approved && earlier.category == rec.category && earlier.title == rec.title) {
                            firstOccurrence = false;
                            break;
                        }
                    }
                    if (firstOccurrence) ++differentTitles;
                }
            }
            auto limit = titleLimitFor(r->category());
            if (limit && !titleExists && differentTitles >= *limit) {
                why = std::string(categoryName(r->category())) + " title limit exceeded";
            } else {
                why = budget_.check(r->category(), quantity, r->costFor(quantity));
            }
        }
    } else {
        why = "resource not found: " + id;
    }
    if (reason) *reason = why;
    return why.empty();
}


const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity) {
    const Resource& r = catalog_.get(id);
    std::string reason;
    if (!canPurchase(id, quantity, &reason)) {
        if (reason.find("title limit exceeded") != std::string::npos)
            throw QuotaExceededError(reason);
        if (quantity <= 0)
            throw std::invalid_argument(reason);
        if (reason.find("budget exceeded") != std::string::npos)
            throw BudgetExceededError(reason);
        throw QuotaExceededError(reason);
    }
    const Money cost = r.costFor(quantity);
    budget_.commit(r.category(), quantity, cost);
    catalog_.addHoldings(id, quantity);
    return record(&r, id, quantity, cost, true, {});
}

PurchaseRecord& AcquisitionManager::record(const Resource* r, const std::string& id,
                                           int qty, Money cost, bool approved,
                                           std::string reason) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book, qty, cost, r ? r->isDigital() : false, approved,
        std::move(reason)});
    return history_.back();
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs) {
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);
        Money cost;
        std::string why;
        if (!r) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            cost = r->costFor(req.quantity);
            canPurchase(req.resourceId, req.quantity, &why);
        }

        if (why.empty()) {
            results.push_back(purchase(req.resourceId, req.quantity));
        } else {
            results.push_back(record(r, req.resourceId, req.quantity, cost, false, why));
        }
    }
    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_)
        if (rec.approved) sum += rec.cost;
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << (rec.approved ? "APPROVED" : "REJECTED") << "  " << std::setw(6)
           << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
           << std::setw(12) << std::right << rec.cost.toString() << std::left << "  "
           << rec.title;
        if (!rec.approved) os << "\n        reason: " << rec.reason;
        os << "\n";
    }
    os << "Pre-tax total: " << totalSpent() << "\n";
    Money totalTax;
    for (const auto& rec : history_) {
        if (rec.approved) {
            const int rate = rec.digital ? electronicTaxPercent_ : printTaxPercent_;
            totalTax += Money::fromMinor((rec.cost.minorUnits() * rate + 50) / 100);
        }
    }
    os << "Tax: " << totalTax << "\n";
    os << "Post-tax total: " << totalSpent() + totalTax << "\n";
}

}  // namespace bookmgmt
