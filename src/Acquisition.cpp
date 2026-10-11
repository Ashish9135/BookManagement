#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget, int printTaxPercent, int electronicTaxPercent)
    : catalog_(catalog), budget_(budget), printTaxPercent_(printTaxPercent), electronicTaxPercent_(electronicTaxPercent) {}

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
        if (quantity <= 0)
            why = "quantity must be positive";
        else
            why = budget_.check(r->category(), quantity, r->costFor(quantity));
    } else {
        why = "resource not found: " + id;
    }
    if (reason) *reason = why;
    return why.empty();
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

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity) {
    const Resource& r = catalog_.get(id);        // may throw NotFoundError
    const Money cost = r.costFor(quantity);      // may throw invalid_argument
    budget_.commit(r.category(), quantity, cost);  // may throw quota/budget errors
    catalog_.addHoldings(id, quantity);
    return record(&r, id, quantity, cost, true, {});
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
            why = budget_.check(r->category(), req.quantity, cost);
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
