// MT26117_ASHISH
#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;  // copies for print, seats for electronic
    std::string department = "Default";
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money cost;
    bool digital = false;
    bool approved;
    std::string reason;  // why it was rejected; empty if approved
    bool cancellation = false;
    int cancelsOrderNo = 0;
    std::string department = "Default";
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget, int printTaxPercent = 5, int electronicTaxPercent = 18);
    void setDepartmentBudget(const std::string& department, Money total);
    Budget& departmentBudget(const std::string& department);
    const Budget& departmentBudget(const std::string& department) const;
    void setTitleLimit(ResourceCategory category, int limit);
    std::optional<int> titleLimitFor(ResourceCategory category) const;

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;
    Money taxFor(const std::string& id, int quantity) const;
    Money totalWithTax(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity, const std::string& department,
                     std::string* reason = nullptr) const;
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    const PurchaseRecord& purchase(const std::string& id, int quantity);
    const PurchaseRecord& purchase(const std::string& id, int quantity, const std::string& department);

    // Cancels an approved order, refunds its budget and reduces holdings.
    // Keeps the original order and adds a cancellation record.
    const PurchaseRecord& cancelOrder(int orderNo);

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
    std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs, bool allOrNothing = false);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    bool isOrderCancelled(int orderNo) const;
    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money cost, bool approved, std::string reason, std::string department = "Default");

    Catalog& catalog_;
    Budget& budget_;
    std::map<ResourceCategory, int> titleLimits_;
    std::map<std::string, std::unique_ptr<Budget>> departmentBudgets_;
    int printTaxPercent_;
    int electronicTaxPercent_;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
