// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.
// MT26117_ASHISH
#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<Book>("B003", "Hardcover Test",
                       std::vector<std::string>{"Author"},
                       "ISBN-HC", "Publisher", 2026,
                       Money::of(100), 1,
                       Binding::Hardcover);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<Journal>("J001", "Nature", "2049-3630", 12, 2,
                             "Springer Nature", 2025, Money::of(500));
    catalog.emplace<EBook>("E001", "Modern C++",std::vector<std::string>{"A", "B"},
                           "978-1234567890", EBookFormat::EPUB, true,
                           "Publisher", 2025, Money::of(100),
                           "https://ebook.example");
    catalog.emplace<AudioBook>("A001", "C++ Audio Guide", "Audio Publisher",
                                2026, Money::of(200), "John Smith", 180);

    catalog.emplace<Thesis>("T001", "AI Research", "IIT Delhi",
                             "M.Tech", "Dr. Sharma", 2026);
    catalog.emplace<Magazine>("M001", "Tech Monthly", "ISSN-1234",
                               12, 2, "Tech Publisher", 2026,
                               Money::of(100), Money::of(10));
    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");

    Budget budget(Money::of(20000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {50, Money::of(17000)});
    budget.setQuota(ResourceCategory::EBook, {20, Money::of(3000)});
    AcquisitionManager acq(catalog, budget);

    // MT26117_ASHISH - Q1 to Q15 feature demonstrations
    std::cout << "\n=== Added resource types (Q1-Q3) ===\n";
    std::cout << catalog.get("J001") << catalog.get("E001")
              << catalog.get("A001") << catalog.get("T001");

    std::cout << "\n=== Pricing and taxes (Q4-Q6) ===\n";
    std::cout << "Hardcover 10-copy quote: " << acq.quote("B002", 10) << "\n";
    std::cout << "Electronic 55-seat quote: " << acq.quote("R001", 55) << "\n";
    AcquisitionManager taxDemo(catalog, budget, 5, 18);
    std::cout << "Pre-tax B001 quote: " << taxDemo.quote("B001", 2)
              << ", post-tax at 5% print tax: " << taxDemo.totalWithTax("B001", 2) << "\n";

    std::cout << "\n=== Title limits, cancellation and departments (Q7-Q10) ===\n";
    acq.setTitleLimit(ResourceCategory::Journal, 2);
    std::cout << "Journal title limit: " << *acq.titleLimitFor(ResourceCategory::Journal) << "\n";
    acq.setDepartmentBudget("Demonstration Department", Money::of(1000));
    const auto& deptOrder = acq.purchase("T001", 1, "Demonstration Department");
    std::cout << "Department order #: " << deptOrder.orderNo << "\n";
    const auto& cancelled = acq.cancelOrder(deptOrder.orderNo);
    std::cout << "Cancellation record #: " << cancelled.orderNo << "\n";
    std::cout << "Next-year rollover from demo budget at 25%: "
              << budget.rollover(25).total() << "\n";

    std::cout << "\n=== Batch, vendors and catalogue searches (Q11-Q13) ===\n";
    auto atomicResults = acq.processBatch({{"B001", 1}, {"UNKNOWN", 1}}, true);
    std::cout << "All-or-nothing batch requests: " << atomicResults.size() << "\n";
    {
        Catalog vendorCatalog;
        auto& vendorTitle = vendorCatalog.emplace<Book>("V001", "Vendor Demo Title",
            std::vector<std::string>{"Demo Author"}, "V-ISBN", "Demo Publisher", 2026,
            Money::of(500));
        vendorTitle.addVendorOffer("Vendor A", Money::of(450));
        vendorTitle.addVendorOffer("Vendor B", Money::of(300));
        Budget vendorBudget(Money::of(1000));
        AcquisitionManager vendorAcq(vendorCatalog, vendorBudget);
        const auto& vendorOrder = vendorAcq.purchaseCheapestVendor("Vendor Demo Title", 1);
        std::cout << "Cheapest vendor selected: " << vendorOrder.vendor
                  << " at " << vendorOrder.cost << "\n";
    }
    catalog.get("B001").setVendor("Demo Books");
    std::cout << "Author search results: " << catalog.searchAuthor("Robert C. Martin").size() << "\n";
    std::cout << "ISBN search results: " << catalog.searchISBN("978-0132350884").size() << "\n";
    std::cout << "Year range results: " << catalog.searchYearRange(2000, 2026).size() << "\n";

    std::cout << "\n=== Lending and money currencies (Q14-Q15) ===\n";
    catalog.addHoldings("B001", 1);
    catalog.borrowPrintCopy("B001", "Demo Patron");
    std::cout << "Active print loans: " << catalog.activeLoans("B001") << "\n";
    catalog.returnPrintCopy("B001", "Demo Patron");
    catalog.addHoldings("E001", 1);
    catalog.openElectronicSession("E001", "Demo Patron");
    std::cout << "Active e-resource sessions: " << catalog.activeSessions("E001") << "\n";
    catalog.closeElectronicSession("E001", "Demo Patron");
    std::cout << "USD + USD: " << (Money::of(1, 0, "USD") + Money::of(2, 0, "USD")) << "\n";

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";

    acq.processBatch({
        {"B001", 4},   // 1800  ok
        {"B002", 5},   // 6000  ok  -> book spend 7800
        {"B001", 1},   // 450   rejected: book spend quota (200 left)
        {"R001", 20},  // 5000  ok
        {"R002", 35},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},   // rejected: unknown id
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 10);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    return 0;
}
