// Minimal self-contained test runner (no external framework needed).
// MT26117_ASHISH
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Function>
static bool throwsAs(Function&& function) {
    try { function(); }
    catch (const Exception&) { return true; }
    catch (...) { return false; }
    return false;
}

static void testMoney() {
    assert((Money::of(12, 5).toString() == "12.05"));
    assert((Money::of(-3, 50).toString() == "-3.50"));
    assert((Money::fromMinor(7).toString() == "0.07"));
    assert((Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050)));
    assert((Money::of(3) * 4 == Money::of(12)));
    assert((Money::of(1) < Money::of(2)));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(Money::of(1, 100)); })));

    // MT26117_ASHISH - Q15 currency tests
    Money usd = Money::of(10, 0, "USD");
    Money usd2 = Money::of(5, 0, "USD");
    Money eur = Money::of(5, 0, "EUR");
    assert((usd.currency() == "USD"));
    assert(((usd + usd2).toString() == "15.00"));
    assert(((usd - usd2).toString() == "5.00"));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd + eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd - eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd == eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd < eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd <= eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd > eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(usd >= eur); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(Money::of(1, 0, "")); })));
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert((b.category() == ResourceCategory::Book));
         Book hardcover("B-HC", "Hardcover Test",
                   {"Author"}, "ISBN-HC",
                   "Publisher", 2026,
                   Money::of(100), 1,
                   Binding::Hardcover);

    assert((hardcover.binding() == Binding::Hardcover));
    assert((hardcover.costFor(1) == Money::of(120)));
    assert((hardcover.costFor(5) == Money::of(600)));
// Q6: 10% bulk discount for 10 or more print copies.
    assert((b.costFor(9) == Money::of(900)));
    assert((b.costFor(10) == Money::of(900)));
    assert((hardcover.costFor(10) == Money::of(1080)));
    assert((!b.isDigital()));
    assert((b.costFor(3) == Money::of(300)));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(b.costFor(0)); })));
    assert((joinAuthors(b.authors()) == "A, B and C"));
    Journal j("J1", "Nature", "1234-5678", 12, 2,
              "Springer", 2025, Money::of(50));

    assert((j.category() == ResourceCategory::Journal));
    assert((j.costFor(3) == Money::of(300)));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(j.costFor(0)); })));
    assert((j.issn() == "1234-5678"));
    assert((j.issuesPerYear() == 12));
    assert((j.subscriptionYears() == 2));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(Journal("J2", "Science", "1111-2222", 12, 0,
                         "Publisher", 2025, Money::of(50))); })));
    EBook eb("E1", "Modern C++", {"A", "B"}, "978-1234567890",
             EBookFormat::EPUB, true, "Publisher", 2025,
             Money::of(100), "https://ebook.example");

    assert((eb.category() == ResourceCategory::EBook));
    assert((eb.isDigital()));
    assert((eb.costFor(3) == Money::of(300)));
        // Q6: Electronic seats beyond the 50th cost half price.
    assert((eb.costFor(50) == Money::of(5000)));
    assert((eb.costFor(51) == Money::of(5050)));
    assert((eb.costFor(60) == Money::of(5500)));
        // Platform fee is added once, including when tiered seat pricing applies.
    ElectronicResource tiered("ER-Q6", "Tiered Test", "Publisher", 2026,
                              Money::of(100), "https://example.com",
                              LicenseModel::AnnualSubscription,
                              Money::of(2000));
    assert((tiered.costFor(50) == Money::of(7000)));
    assert((tiered.costFor(51) == Money::of(7050)));
    assert((eb.authors().size() == 2));
    assert((eb.isbn() == "978-1234567890"));
    assert((eb.format() == EBookFormat::EPUB));
    assert((eb.drmProtected()));
        AudioBook audio("A1", "C++ Audio Guide", "Audio Publisher", 2026,
                    Money::of(200), "John Smith", 180);

    assert((audio.category() == ResourceCategory::AudioBook));
    assert((!audio.isDigital()));
    assert((audio.costFor(2) == Money::of(400)));
    assert((audio.costFor(10) == Money::of(1800)));
    assert((audio.narrator() == "John Smith"));
    assert((audio.durationMinutes() == 180));

    Thesis thesis("T1", "AI Research", "IIT Delhi",
                  "M.Tech", "Dr. Sharma", 2026);

    assert((thesis.category() == ResourceCategory::Thesis));
    assert((!thesis.isDigital()));
    assert((thesis.costFor(1) == Money{}));
    assert((thesis.university() == "IIT Delhi"));
    assert((thesis.degree() == "M.Tech"));
    assert((thesis.supervisor() == "Dr. Sharma"));
    Magazine magazine("M1", "Tech Monthly", "ISSN-1234",
                      12, 2, "Tech Publisher", 2026,
                      Money::of(100), Money::of(10));

    assert((magazine.category() == ResourceCategory::Magazine));
    assert((magazine.issuesPerYear() == 12));
    assert((magazine.subscriptionYears() == 2));
    assert((magazine.postagePerIssue() == Money::of(10)));

    // Journal cost = 100 × 2 copies × 2 years = 400
    // Postage = 10 × 12 issues × 2 years × 2 copies = 480
    // Total = 880
    assert((magazine.costFor(2) == Money::of(880)));

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert((e.isDigital()));
    assert((e.costFor(5) == Money::of(150)));

    assert((e.category() == ResourceCategory::ElectronicResource));

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert((r.costFor(1) == Money::of(110)));
    std::ostringstream os;
    os << r;
    assert((os.str().find("platform fee: 100.00") != std::string::npos));

    assert((throwsAs<std::invalid_argument>([&]() { (void)(Book("", "T", {}, "", "", 2000, Money::of(1))); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1))); })));
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<EBook>("E1", "Modern C++", std::vector<std::string>{"Alice Smith"},
                                 "978-111", EBookFormat::EPUB, false, "TechPub", 2021,
                                 Money::of(20), "url");
    c.emplace<Journal>("J1", "Computing Journal", "ISSN-1234", 12, 1,
                                  "SciencePub", 2022, Money::of(30));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert((c.size() == 5));
    assert((c.contains("B1")));
    assert((c.find("nope") == nullptr));
    assert((throwsAs<NotFoundError>([&]() { (void)(c.get("nope")); })));
    assert((throwsAs<DuplicateIdError>([&]() { (void)(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1))); })));

    assert((c.searchTitle("clean").size() == 2));
    assert((c.searchAuthor("alice").size() == 1));
    assert((c.searchAuthor("ALICE SMITH").size() == 1));
    assert((c.searchISBN("978-111").size() == 1));
    assert((c.searchISSN("ISSN-1234").size() == 1));
    assert((c.searchISBN("missing").empty()));
    assert((c.searchISSN("missing").empty()));
    assert((c.searchYearRange(2021, 2022).size() == 2));
    assert((c.searchYearRange(2008, 2008).size() == 1));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.searchYearRange(2023, 2020)); })));
    assert((c.byCategory(ResourceCategory::ElectronicResource).size() == 1));
    assert((c.where([](const Resource& r) { return r.isDigital(); }).size() == 2));

    assert((c.holdings("B1") == 0));
    c.addHoldings("B1", 3);
    assert((c.holdings("B1") == 3));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.addHoldings("B1", -5)); })));

    // MT26117_ASHISH - Q14 Lending tests
    c.borrowPrintCopy("B1", "Patron1");
    c.borrowPrintCopy("B1", "Patron2");
    c.borrowPrintCopy("B1", "Patron3");
    assert((c.activeLoans("B1") == 3));
    assert((throwsAs<std::runtime_error>([&]() { (void)(c.borrowPrintCopy("B1", "Patron4")); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.borrowPrintCopy("E1", "Patron4")); })));
    c.returnPrintCopy("B1", "Patron2");
    assert((c.activeLoans("B1") == 2));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.returnPrintCopy("B1", "Patron2")); })));

    c.addHoldings("E1", 2);
    c.openElectronicSession("E1", "Patron1");
    c.openElectronicSession("E1", "Patron2");
    assert((c.activeSessions("E1") == 2));
    assert((throwsAs<std::runtime_error>([&]() { (void)(c.openElectronicSession("E1", "Patron3")); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.openElectronicSession("B1", "Patron3")); })));
    c.closeElectronicSession("E1", "Patron1");
    assert((c.activeSessions("E1") == 1));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(c.closeElectronicSession("E1", "Patron1")); })));

    c.remove("R1");
    assert((c.size() == 4));
    assert((throwsAs<NotFoundError>([&]() { (void)(c.remove("R1")); })));
    // MT26117_ASHISH
    c.saveToFile("/tmp/bookmgmt_catalog_test.txt");

    Catalog restored;
    restored.loadFromFile("/tmp/bookmgmt_catalog_test.txt");

    assert((restored.size() == c.size()));
    assert((restored.contains("B1")));
    assert((restored.contains("E1")));
    assert((restored.contains("J1")));
    assert((restored.holdings("B1") == 3));
    assert((restored.searchTitle("clean").size() == 2));
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    assert((b.check(ResourceCategory::Book, 2, Money::of(200)).empty()));
    assert((!b.check(ResourceCategory::Book, 6, Money::of(10)).empty()));   // units
    assert((!b.check(ResourceCategory::Book, 1, Money::of(401)).empty()));  // spend
    assert((!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty()));  // overall
    assert((b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty()));    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert((b.spent() == Money::of(300)));
    assert((*b.unitsRemaining(ResourceCategory::Book) == 1));
    assert((*b.spendRemaining(ResourceCategory::Book) == Money::of(100)));
    assert((!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value()));

    assert((throwsAs<QuotaExceededError>([&]() { (void)(b.commit(ResourceCategory::Book, 2, Money::of(10))); })));
    assert((throwsAs<BudgetExceededError>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800))); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1))); })));
    assert((b.spent() == Money::of(300)));  // failed commits changed nothing

    const Budget nextYear = b.rollover(25);
    assert((nextYear.total() == Money::of(175)));
    assert((nextYear.spent() == Money{}));
    assert((b.total() == Money::of(1000)));
    assert((b.remaining() == Money::of(700)));
    assert((b.spent() == Money::of(300)));
    assert((b.rollover(0).total() == Money{}));
    assert((b.rollover(100).total() == Money::of(700)));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(b.rollover(-1)); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(b.rollover(101)); })));
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<EBook>("E1", "Modern C++", std::vector<std::string>{"Alice Smith"},
                                 "978-111", EBookFormat::EPUB, false, "TechPub", 2021,
                                 Money::of(20), "url");
    c.emplace<Journal>("J1", "Computing Journal", "ISSN-1234", 12, 1,
                                  "SciencePub", 2022, Money::of(30));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    assert((acq.quote("R1", 5) == Money::of(100)));
    std::string why;
    assert((acq.canPurchase("B1", 3, &why) && why.empty()));
    assert((!acq.canPurchase("B1", 4, &why) && !why.empty()));
    assert((!acq.canPurchase("nope", 1, &why)));

    const auto& rec = acq.purchase("B1", 2);
    assert((rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1));
    assert((c.holdings("B1") == 2));

    assert((throwsAs<QuotaExceededError>([&]() { (void)(acq.purchase("B1", 2)); })));
    assert((throwsAs<NotFoundError>([&]() { (void)(acq.purchase("nope", 1)); })));
    assert((acq.history().size() == 1));  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert((res.size() == 5));
    assert((res[0].approved && res[0].cost == Money::of(150)));
    assert((!res[1].approved));  // 1050 > remaining 150
    assert((res[2].approved));
    assert((!res[3].approved && res[3].reason.find("not found") != std::string::npos));
    assert((!res[4].approved));
    assert((acq.totalSpent() == Money::of(450)));
    assert((b.spent() == acq.totalSpent()));
    assert((c.holdings("R1") == 10 && c.holdings("B1") == 3));
    assert((acq.history().size() == 6));
}

static void testTaxes() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<EBook>("E1", "Modern C++", std::vector<std::string>{"Alice Smith"},
                                 "978-111", EBookFormat::EPUB, false, "TechPub", 2021,
                                 Money::of(20), "url");
    c.emplace<Journal>("J1", "Computing Journal", "ISSN-1234", 12, 1,
                                  "SciencePub", 2022, Money::of(30));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(100), "url");
    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b, 5, 18);

    assert((acq.quote("B1", 2) == Money::of(200)));
    assert((acq.taxFor("B1", 2) == Money::of(10)));
    assert((acq.totalWithTax("B1", 2) == Money::of(210)));

    assert((acq.quote("R1", 2) == Money::of(200)));
    assert((acq.taxFor("R1", 2) == Money::of(36)));
    assert((acq.totalWithTax("R1", 2) == Money::of(236)));

    acq.purchase("B1", 1);
    acq.purchase("R1", 1);
    std::ostringstream report;
    acq.printReport(report);
    assert((report.str().find("Pre-tax total: 200.00") != std::string::npos));
    assert((report.str().find("Tax: 23.00") != std::string::npos));
    assert((report.str().find("Post-tax total: 223.00") != std::string::npos));
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

    assert((acq.titleLimitFor(ResourceCategory::Book) == std::optional<int>(1)));
    assert((!acq.titleLimitFor(ResourceCategory::Journal).has_value()));
    assert((acq.canPurchase("B1", 3)));
    acq.purchase("B1", 3);
    assert((acq.canPurchase("B1", 1)));
    assert((!acq.canPurchase("B2", 1)));
    assert((throwsAs<QuotaExceededError>([&]() { (void)(acq.purchase("B2", 1)); })));
    assert((acq.canPurchase("E1", 1)));
    acq.purchase("E1", 1);
    assert((c.holdings("B1") == 3));
    assert((c.holdings("B2") == 0));
    assert((c.holdings("E1") == 1));

    auto results = acq.processBatch({{"B2", 1}, {"B1", 1}});
    assert((results.size() == 2));
    assert((!results[0].approved));
    assert((results[0].reason.find("title limit exceeded") != std::string::npos));
    assert((results[1].approved));
    assert((c.holdings("B1") == 4));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(acq.setTitleLimit(ResourceCategory::Book, -1)); })));
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
    assert((b.spent() == Money::of(300)));
    assert((c.holdings("B1") == 3));

    const auto cancellation = acq.cancelOrder(originalOrderNo);
    assert((cancellation.cancellation));
    assert((cancellation.cancelsOrderNo == originalOrderNo));
    assert((cancellation.approved));
    assert((acq.history().size() == 2));
    assert((acq.history()[0].orderNo == originalOrderNo));
    assert((!acq.history()[0].cancellation));
    assert((c.holdings("B1") == 0));
    assert((b.spent() == Money{}));
    assert((acq.totalSpent() == Money{}));
    assert((acq.canPurchase("B2", 1)));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(acq.cancelOrder(originalOrderNo)); })));
    assert((throwsAs<NotFoundError>([&]() { (void)(acq.cancelOrder(999)); })));

    std::ostringstream report;
    acq.printReport(report);
    assert((report.str().find("CANCELLED") != std::string::npos));
    assert((report.str().find("Pre-tax total: 0.00") != std::string::npos));
    assert((report.str().find("Tax: 0.00") != std::string::npos));
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
    assert((csOrder.approved));
    assert((csOrder.department == "Computer Science"));
    assert((acq.departmentBudget("Computer Science").spent() == Money::of(200)));
    assert((acq.departmentBudget("Physics").spent() == Money{}));
    assert((defaultBudget.spent() == Money{}));

    const auto physicsOrder = acq.purchase("B1", 1, "Physics");
    assert((physicsOrder.approved));
    assert((physicsOrder.department == "Physics"));
    assert((acq.departmentBudget("Physics").spent() == Money::of(100)));
    assert((acq.departmentBudget("Computer Science").spent() == Money::of(200)));

    assert((!acq.canPurchase("B1", 1, "Physics")));
    assert((!acq.canPurchase("B1", 1, "Unknown Department")));

    const auto cancellation = acq.cancelOrder(csOrder.orderNo);
    assert((cancellation.cancellation));
    assert((cancellation.department == "Computer Science"));
    assert((acq.departmentBudget("Computer Science").spent() == Money{}));
    assert((acq.departmentBudget("Physics").spent() == Money::of(100)));
    assert((c.holdings("B1") == 1));
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
    assert((report.str().find("WARNING") != std::string::npos));
    assert((report.str().find("Book") != std::string::npos));
    assert((report.str().find("unit quota") != std::string::npos));
    assert((report.str().find("spend quota") == std::string::npos));
}

static void testRequestPriority() {
    Catalog c;
    c.emplace<Book>("P1", "Low Priority Book", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<Book>("P2", "High Priority Book", std::vector<std::string>{"B"}, "i", "P", 2021, Money::of(100));
    Budget b(Money::of(100));
    AcquisitionManager acq(c, b);

    auto results = acq.processBatch({{"P1", 1, "Default", 1}, {"P2", 1, "Default", 10}});
    assert((results.size() == 2));
    assert((!results[0].approved));
    assert((results[1].approved));
    assert((c.holdings("P1") == 0));
    assert((c.holdings("P2") == 1));
}


static void testAllOrNothingBatch() {
    Catalog c;
    c.emplace<Book>("B1", "Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<Book>("B2", "Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget b(Money::of(250));
    AcquisitionManager acq(c, b);

    auto failed = acq.processBatch({{"B1", 1}, {"B2", 2}}, true);
    assert((failed.size() == 2));
    assert((!failed[0].approved && !failed[1].approved));
    assert((acq.totalSpent() == Money::of(0)));
    assert((b.spent() == Money::of(0)));
    assert((c.holdings("B1") == 0 && c.holdings("B2") == 0));

    auto succeeded = acq.processBatch({{"B1", 1}, {"B2", 1}}, true);
    assert((succeeded.size() == 2));
    assert((succeeded[0].approved && succeeded[1].approved));
    assert((acq.totalSpent() == Money::of(200)));
    assert((c.holdings("B1") == 1 && c.holdings("B2") == 1));

    // Test cumulative category quota: the entire batch must be rejected.
    Catalog quotaCatalog;
    quotaCatalog.emplace<Book>("Q1", "Quota Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    quotaCatalog.emplace<Book>("Q2", "Quota Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget quotaBudget(Money::of(1000));
    quotaBudget.setQuota(ResourceCategory::Book, {1, Money::of(1000)});
    AcquisitionManager quotaAcq(quotaCatalog, quotaBudget);
    auto quotaFailed = quotaAcq.processBatch({{"Q1", 1}, {"Q2", 1}}, true);
    assert((quotaFailed.size() == 2));
    assert((!quotaFailed[0].approved && !quotaFailed[1].approved));
    assert((quotaBudget.spent() == Money::of(0)));
    assert((quotaCatalog.holdings("Q1") == 0 && quotaCatalog.holdings("Q2") == 0));

    // Test title limits: one invalid request rejects the entire batch.
    Catalog titleCatalog;
    titleCatalog.emplace<Book>("T1", "Title Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    titleCatalog.emplace<Book>("T2", "Title Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget titleBudget(Money::of(1000));
    AcquisitionManager titleAcq(titleCatalog, titleBudget);
    titleAcq.setTitleLimit(ResourceCategory::Book, 1);
    auto titleFailed = titleAcq.processBatch({{"T1", 1}, {"T2", 1}}, true);
    assert((titleFailed.size() == 2));
    assert((!titleFailed[0].approved && !titleFailed[1].approved));
    assert((titleBudget.spent() == Money::of(0)));
    assert((titleCatalog.holdings("T1") == 0 && titleCatalog.holdings("T2") == 0));

    // Test cumulative department budget: the entire batch must be rejected.
    Catalog deptCatalog;
    deptCatalog.emplace<Book>("D1", "Department Book One", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    deptCatalog.emplace<Book>("D2", "Department Book Two", std::vector<std::string>{"A"}, "i", "P", 2021, Money::of(100));
    Budget deptDefault(Money::of(1000));
    AcquisitionManager deptAcq(deptCatalog, deptDefault);
    deptAcq.setDepartmentBudget("Physics", Money::of(150));
    auto deptFailed = deptAcq.processBatch({{"D1", 1, "Physics"}, {"D2", 1, "Physics"}}, true);
    assert((deptFailed.size() == 2));
    assert((!deptFailed[0].approved && !deptFailed[1].approved));
    assert((deptAcq.departmentBudget("Physics").spent() == Money::of(0)));
    assert((deptCatalog.holdings("D1") == 0 && deptCatalog.holdings("D2") == 0));


}
static void testCheapestVendor() {
    Catalog c;
    auto& expensive = c.emplace<Book>("V1", "C++ Programming", std::vector<std::string>{"A"}, "i1", "P", 2020, Money::of(500));
    auto& cheap = c.emplace<Book>("V2", "C++ Programming", std::vector<std::string>{"A"}, "i2", "P", 2020, Money::of(350));
    expensive.setVendor("Vendor A");
    cheap.setVendor("Vendor B");
    Budget b(Money::of(1000));
    AcquisitionManager acq(c, b);

    const auto& rec = acq.purchaseCheapestVendor("C++ Programming", 1);
    assert((rec.approved));
    assert((rec.resourceId == "V2"));
    assert((rec.vendor == "Vendor B"));
    assert((rec.cost == Money::of(350)));
    assert((c.holdings("V2") == 1));
    assert((c.holdings("V1") == 0));
    assert((throwsAs<NotFoundError>([&]() { (void)(acq.purchaseCheapestVendor("Unknown Title", 1)); })));
    assert((throwsAs<std::invalid_argument>([&]() { (void)(acq.purchaseCheapestVendor("C++ Programming", 0)); })));
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
    testRequestPriority();
    testCheapestVendor();
    testAllOrNothingBatch();
    std::cout << "All tests passed (assertions enabled).\n";
    return 0;
}
