// Minimal self-contained test runner (no external framework needed).
// MT26117_ASHISH
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
         Book hardcover("B-HC", "Hardcover Test",
                   {"Author"}, "ISBN-HC",
                   "Publisher", 2026,
                   Money::of(100), 1,
                   Binding::Hardcover);

    CHECK(hardcover.binding() == Binding::Hardcover);
    CHECK(hardcover.costFor(1) == Money::of(120));
    CHECK(hardcover.costFor(5) == Money::of(600));
// Q6: 10% bulk discount for 10 or more print copies.
    CHECK(b.costFor(9) == Money::of(900));
    CHECK(b.costFor(10) == Money::of(900));
    CHECK(hardcover.costFor(10) == Money::of(1080));
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");
    Journal j("J1", "Nature", "1234-5678", 12, 2,
              "Springer", 2025, Money::of(50));

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(j.costFor(3) == Money::of(300));
    CHECK_THROWS(j.costFor(0), std::invalid_argument);
    CHECK(j.issn() == "1234-5678");
    CHECK(j.issuesPerYear() == 12);
    CHECK(j.subscriptionYears() == 2);
    CHECK_THROWS(Journal("J2", "Science", "1111-2222", 12, 0,
                         "Publisher", 2025, Money::of(50)),
                 std::invalid_argument);
    EBook eb("E1", "Modern C++", {"A", "B"}, "978-1234567890",
             EBookFormat::EPUB, true, "Publisher", 2025,
             Money::of(100), "https://ebook.example");

    CHECK(eb.category() == ResourceCategory::EBook);
    CHECK(eb.isDigital());
    CHECK(eb.costFor(3) == Money::of(300));
        // Q6: Electronic seats beyond the 50th cost half price.
    CHECK(eb.costFor(50) == Money::of(5000));
    CHECK(eb.costFor(51) == Money::of(5050));
    CHECK(eb.costFor(60) == Money::of(5500));
        // Platform fee is added once, including when tiered seat pricing applies.
    ElectronicResource tiered("ER-Q6", "Tiered Test", "Publisher", 2026,
                              Money::of(100), "https://example.com",
                              LicenseModel::AnnualSubscription,
                              Money::of(2000));
    CHECK(tiered.costFor(50) == Money::of(7000));
    CHECK(tiered.costFor(51) == Money::of(7050));
    CHECK(eb.authors().size() == 2);
    CHECK(eb.isbn() == "978-1234567890");
    CHECK(eb.format() == EBookFormat::EPUB);
    CHECK(eb.drmProtected());
        AudioBook audio("A1", "C++ Audio Guide", "Audio Publisher", 2026,
                    Money::of(200), "John Smith", 180);

    CHECK(audio.category() == ResourceCategory::AudioBook);
    CHECK(!audio.isDigital());
    CHECK(audio.costFor(2) == Money::of(400));
    CHECK(audio.narrator() == "John Smith");
    CHECK(audio.durationMinutes() == 180);

    Thesis thesis("T1", "AI Research", "IIT Delhi",
                  "M.Tech", "Dr. Sharma", 2026);

    CHECK(thesis.category() == ResourceCategory::Thesis);
    CHECK(!thesis.isDigital());
    CHECK(thesis.costFor(1) == Money{});
    CHECK(thesis.university() == "IIT Delhi");
    CHECK(thesis.degree() == "M.Tech");
    CHECK(thesis.supervisor() == "Dr. Sharma");
    Magazine magazine("M1", "Tech Monthly", "ISSN-1234",
                      12, 2, "Tech Publisher", 2026,
                      Money::of(100), Money::of(10));

    CHECK(magazine.category() == ResourceCategory::Magazine);
    CHECK(magazine.issuesPerYear() == 12);
    CHECK(magazine.subscriptionYears() == 2);
    CHECK(magazine.postagePerIssue() == Money::of(10));

    // Journal cost = 100 × 2 copies × 2 years = 400
    // Postage = 10 × 12 issues × 2 years × 2 copies = 480
    // Total = 880
    CHECK(magazine.costFor(2) == Money::of(880));

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing

    const Budget nextYear = b.rollover(25);
    CHECK(nextYear.total() == Money::of(175));
    CHECK(nextYear.spent() == Money{});
    CHECK(b.total() == Money::of(1000));
    CHECK(b.remaining() == Money::of(700));
    CHECK(b.spent() == Money::of(300));
    CHECK(b.rollover(0).total() == Money{});
    CHECK(b.rollover(100).total() == Money::of(700));
    CHECK_THROWS(b.rollover(-1), std::invalid_argument);
    CHECK_THROWS(b.rollover(101), std::invalid_argument);
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}

static void testTaxes() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(100), "url");
    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("B1", 2) == Money::of(200));
    CHECK(acq.taxFor("B1", 2) == Money::of(10));
    CHECK(acq.totalWithTax("B1", 2) == Money::of(210));

    CHECK(acq.quote("R1", 2) == Money::of(200));
    CHECK(acq.taxFor("R1", 2) == Money::of(36));
    CHECK(acq.totalWithTax("R1", 2) == Money::of(236));

    acq.purchase("B1", 1);
    acq.purchase("R1", 1);
    std::ostringstream report;
    acq.printReport(report);
    CHECK(report.str().find("Pre-tax total: 200.00") != std::string::npos);
    CHECK(report.str().find("Tax: 23.00") != std::string::npos);
    CHECK(report.str().find("Post-tax total: 223.00") != std::string::npos);
}

static void testTitleLimits() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<Book>("B2", "Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    c.emplace<EBook>("E1", "EBook One", std::vector<std::string>{"A"}, "ISBN1", EBookFormat::PDF, false, "P", 2022, Money::of(100), "url");
    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b);
    acq.setTitleLimit(ResourceCategory::Book, 1);
    acq.setTitleLimit(ResourceCategory::EBook, 1);

    CHECK(acq.titleLimitFor(ResourceCategory::Book) == std::optional<int>(1));
    CHECK(!acq.titleLimitFor(ResourceCategory::Journal).has_value());
    CHECK(acq.canPurchase("B1", 3));
    acq.purchase("B1", 3);
    CHECK(acq.canPurchase("B1", 1));
    CHECK(!acq.canPurchase("B2", 1));
    CHECK_THROWS(acq.purchase("B2", 1), QuotaExceededError);
    CHECK(acq.canPurchase("E1", 1));
    acq.purchase("E1", 1);
    CHECK(c.holdings("B1") == 3);
    CHECK(c.holdings("B2") == 0);
    CHECK(c.holdings("E1") == 1);

    auto results = acq.processBatch({{"B2", 1}, {"B1", 1}});
    CHECK(results.size() == 2);
    CHECK(!results[0].approved);
    CHECK(results[0].reason.find("title limit exceeded") != std::string::npos);
    CHECK(results[1].approved);
    CHECK(c.holdings("B1") == 4);
    CHECK_THROWS(acq.setTitleLimit(ResourceCategory::Book, -1), std::invalid_argument);
}

static void testCancellation() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<Book>("B2", "Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget b(Money::of(1000));
    AcquisitionManager acq(c, b);
    acq.setTitleLimit(ResourceCategory::Book, 1);

    const auto original = acq.purchase("B1", 3);
    const int originalOrderNo = original.orderNo;
    CHECK(b.spent() == Money::of(300));
    CHECK(c.holdings("B1") == 3);

    const auto cancellation = acq.cancelOrder(originalOrderNo);
    CHECK(cancellation.cancellation);
    CHECK(cancellation.cancelsOrderNo == originalOrderNo);
    CHECK(cancellation.approved);
    CHECK(acq.history().size() == 2);
    CHECK(acq.history()[0].orderNo == originalOrderNo);
    CHECK(!acq.history()[0].cancellation);
    CHECK(c.holdings("B1") == 0);
    CHECK(b.spent() == Money{});
    CHECK(acq.totalSpent() == Money{});
    CHECK(acq.canPurchase("B2", 1));
    CHECK_THROWS(acq.cancelOrder(originalOrderNo), std::invalid_argument);
    CHECK_THROWS(acq.cancelOrder(999), NotFoundError);

    std::ostringstream report;
    acq.printReport(report);
    CHECK(report.str().find("CANCELLED") != std::string::npos);
    CHECK(report.str().find("Pre-tax total: 0.00") != std::string::npos);
    CHECK(report.str().find("Tax: 0.00") != std::string::npos);
}

static void testDepartmentBudgets() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget defaultBudget(Money::of(1000));
    AcquisitionManager acq(c, defaultBudget);
    acq.setDepartmentBudget("Computer Science", Money::of(300));
    acq.setDepartmentBudget("Physics", Money::of(200));

    acq.departmentBudget("Computer Science").setQuota(ResourceCategory::Book, {2, Money::of(200)});
    acq.departmentBudget("Physics").setQuota(ResourceCategory::Book, {1, Money::of(100)});

    const auto csOrder = acq.purchase("B1", 2, "Computer Science");
    CHECK(csOrder.approved);
    CHECK(csOrder.department == "Computer Science");
    CHECK(acq.departmentBudget("Computer Science").spent() == Money::of(200));
    CHECK(acq.departmentBudget("Physics").spent() == Money{});
    CHECK(defaultBudget.spent() == Money{});

    const auto physicsOrder = acq.purchase("B1", 1, "Physics");
    CHECK(physicsOrder.approved);
    CHECK(physicsOrder.department == "Physics");
    CHECK(acq.departmentBudget("Physics").spent() == Money::of(100));
    CHECK(acq.departmentBudget("Computer Science").spent() == Money::of(200));

    CHECK(!acq.canPurchase("B1", 1, "Physics"));
    CHECK(!acq.canPurchase("B1", 1, "Unknown Department"));

    const auto cancellation = acq.cancelOrder(csOrder.orderNo);
    CHECK(cancellation.cancellation);
    CHECK(cancellation.department == "Computer Science");
    CHECK(acq.departmentBudget("Computer Science").spent() == Money{});
    CHECK(acq.departmentBudget("Physics").spent() == Money::of(100));
    CHECK(c.holdings("B1") == 1);
}

static void testQuotaWarnings() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(60));
    Budget b(Money::of(1000));
    AcquisitionManager acq(c, b);
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});
    acq.purchase("B1", 5);

    std::ostringstream report;
    acq.printReport(report);
    CHECK(report.str().find("WARNING") != std::string::npos);
    CHECK(report.str().find("Book") != std::string::npos);
    CHECK(report.str().find("unit quota") != std::string::npos);
    CHECK(report.str().find("spend quota") == std::string::npos);
}

static void testAllOrNothingBatch() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<Book>("B2", "Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget b(Money::of(250));
    AcquisitionManager acq(c, b);

    auto failed = acq.processBatch({{"B1", 1}, {"B2", 2}}, true);
    CHECK(failed.size() == 2);
    CHECK(!failed[0].approved && !failed[1].approved);
    CHECK(acq.totalSpent() == Money::of(0));
    CHECK(b.spent() == Money::of(0));
    CHECK(c.holdings("B1") == 0 && c.holdings("B2") == 0);

    auto succeeded = acq.processBatch({{"B1", 1}, {"B2", 1}}, true);
    CHECK(succeeded.size() == 2);
    CHECK(succeeded[0].approved && succeeded[1].approved);
    CHECK(acq.totalSpent() == Money::of(200));
    CHECK(c.holdings("B1") == 1 && c.holdings("B2") == 1);

    // Test cumulative category quota: the entire batch must be rejected.
    Catalog quotaCatalog;
    quotaCatalog.emplace<Book>("Q1", "Quota Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    quotaCatalog.emplace<Book>("Q2", "Quota Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget quotaBudget(Money::of(1000));
    quotaBudget.setQuota(ResourceCategory::Book, {1, Money::of(1000)});
    AcquisitionManager quotaAcq(quotaCatalog, quotaBudget);
    auto quotaFailed = quotaAcq.processBatch({{"Q1", 1}, {"Q2", 1}}, true);
    CHECK(quotaFailed.size() == 2);
    CHECK(!quotaFailed[0].approved && !quotaFailed[1].approved);
    CHECK(quotaBudget.spent() == Money::of(0));
    CHECK(quotaCatalog.holdings("Q1") == 0 && quotaCatalog.holdings("Q2") == 0);

    // Test title limits: one invalid request rejects the entire batch.
    Catalog titleCatalog;
    titleCatalog.emplace<Book>("T1", "Title Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    titleCatalog.emplace<Book>("T2", "Title Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget titleBudget(Money::of(1000));
    AcquisitionManager titleAcq(titleCatalog, titleBudget);
    titleAcq.setTitleLimit(ResourceCategory::Book, 1);
    auto titleFailed = titleAcq.processBatch({{"T1", 1}, {"T2", 1}}, true);
    CHECK(titleFailed.size() == 2);
    CHECK(!titleFailed[0].approved && !titleFailed[1].approved);
    CHECK(titleBudget.spent() == Money::of(0));
    CHECK(titleCatalog.holdings("T1") == 0 && titleCatalog.holdings("T2") == 0);

    // Test cumulative department budget: the entire batch must be rejected.
    Catalog deptCatalog;
    deptCatalog.emplace<Book>("D1", "Department Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    deptCatalog.emplace<Book>("D2", "Department Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget deptDefault(Money::of(1000));
    AcquisitionManager deptAcq(deptCatalog, deptDefault);
    deptAcq.setDepartmentBudget("Physics", Money::of(150));
    auto deptFailed = deptAcq.processBatch({{"D1", 1, "Physics"}, {"D2", 1, "Physics"}}, true);
    CHECK(deptFailed.size() == 2);
    CHECK(!deptFailed[0].approved && !deptFailed[1].approved);
    CHECK(deptAcq.departmentBudget("Physics").spent() == Money::of(0));
    CHECK(deptCatalog.holdings("D1") == 0 && deptCatalog.holdings("D2") == 0);


}
int main() {
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testTaxes();
    testTitleLimits();
    testCancellation();
    testDepartmentBudgets();
    testQuotaWarnings();
    testAllOrNothingBatch();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
