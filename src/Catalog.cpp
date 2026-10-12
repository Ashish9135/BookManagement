#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"
#include "bookmgmt/AudioBook.h"
#include "bookmgmt/Thesis.h"
#include "bookmgmt/Magazine.h"

namespace bookmgmt {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");
    const std::string id = r->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& e = items_[id];
    e.resource = std::move(r);
    return *e.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) > 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");
    it->second.holdings += units;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {
    std::vector<const Resource*> out;
    for (const auto& [id, entry] : items_)
        if (pred(*entry.resource)) out.push_back(entry.resource.get());
    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) { return true; });
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory c) const {
    return where([c](const Resource& r) { return r.category() == c; });
}

std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        return lower(r.title()).find(needle) != std::string::npos;
    });
}

std::vector<const Resource*> Catalog::searchAuthor(const std::string& author) const {
    const std::string needle = lower(author);
    return where([&needle](const Resource& r) {
        if (const auto* b = dynamic_cast<const Book*>(&r)) {
            for (const auto& a : b->authors())
                if (lower(a).find(needle) != std::string::npos) return true;
        }
        if (const auto* e = dynamic_cast<const EBook*>(&r)) {
            for (const auto& a : e->authors())
                if (lower(a).find(needle) != std::string::npos) return true;
        }
        return false;
    });
}

std::vector<const Resource*> Catalog::searchISBN(const std::string& isbn) const {
    return where([&isbn](const Resource& r) {
        if (const auto* b = dynamic_cast<const Book*>(&r)) return b->isbn() == isbn;
        if (const auto* e = dynamic_cast<const EBook*>(&r)) return e->isbn() == isbn;
        return false;
    });
}

std::vector<const Resource*> Catalog::searchISSN(const std::string& issn) const {
    return where([&issn](const Resource& r) {
        if (const auto* j = dynamic_cast<const Journal*>(&r)) return j->issn() == issn;
        return false;
    });
}

std::vector<const Resource*> Catalog::searchYearRange(int firstYear, int lastYear) const {
    if (firstYear > lastYear) throw std::invalid_argument("first year must not exceed last year");
    return where([firstYear, lastYear](const Resource& r) {
        return r.year() >= firstYear && r.year() <= lastYear;
    });
}

// MT26117_ASHISH
// MT26117_ASHISH
int Catalog::activeLoans(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return static_cast<int>(it->second.activeLoans.size());
}

int Catalog::activeSessions(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return static_cast<int>(it->second.activeSessions.size());
}

void Catalog::borrowPrintCopy(const std::string& id, const std::string& patron) {
    if (patron.empty()) throw std::invalid_argument("patron cannot be empty");

    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);

    Entry& entry = it->second;
    if (entry.resource->isDigital())
        throw std::invalid_argument("cannot borrow a print copy of an electronic resource");

    if (static_cast<int>(entry.activeLoans.size()) >= entry.holdings)
        throw std::runtime_error("no print copies available");

    if (std::find(entry.activeLoans.begin(), entry.activeLoans.end(), patron)
        != entry.activeLoans.end())
        throw std::invalid_argument("patron already has this resource on loan");

    entry.activeLoans.push_back(patron);
}

void Catalog::returnPrintCopy(const std::string& id, const std::string& patron) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);

    auto& loans = it->second.activeLoans;
    auto loan = std::find(loans.begin(), loans.end(), patron);
    if (loan == loans.end())
        throw std::invalid_argument("no active loan for this patron");

    loans.erase(loan);
}

void Catalog::openElectronicSession(const std::string& id, const std::string& patron) {
    if (patron.empty()) throw std::invalid_argument("patron cannot be empty");

    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);

    Entry& entry = it->second;
    if (!entry.resource->isDigital())
        throw std::invalid_argument("cannot open an electronic session for a print resource");

    if (static_cast<int>(entry.activeSessions.size()) >= entry.holdings)
        throw std::runtime_error("no electronic seats available");

    if (std::find(entry.activeSessions.begin(), entry.activeSessions.end(), patron)
        != entry.activeSessions.end())
        throw std::invalid_argument("patron already has an active session");

    entry.activeSessions.push_back(patron);
}

void Catalog::closeElectronicSession(const std::string& id, const std::string& patron) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);

    auto& sessions = it->second.activeSessions;
    auto session = std::find(sessions.begin(), sessions.end(), patron);
    if (session == sessions.end())
        throw std::invalid_argument("no active session for this patron");

    sessions.erase(session);
}
void Catalog::saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("cannot open catalog file for writing");
    }

    out << items_.size() << '\n';

    for (const auto& [id, entry] : items_) {
        const Resource& r = *entry.resource;

        out << std::quoted(id) << ' '
            << entry.holdings << ' '
            << std::quoted(r.title()) << ' '
            << std::quoted(r.publisher()) << ' '
            << r.year() << ' '
            << r.unitPrice().minorUnits() << ' '
            << std::quoted(r.vendor()) << ' ';

        if (const auto* m = dynamic_cast<const Magazine*>(&r)) {
            out << "Magazine " << std::quoted(m->issn()) << ' '
                << m->issuesPerYear() << ' ' << m->subscriptionYears()
                << ' ' << m->postagePerIssue().minorUnits();
        } else if (const auto* e = dynamic_cast<const EBook*>(&r)) {
            out << "EBook " << e->authors().size() << ' ';
            for (const auto& a : e->authors()) out << std::quoted(a) << ' ';
            out << std::quoted(e->isbn()) << ' '
                << static_cast<int>(e->format()) << ' '
                << e->drmProtected() << ' '
                << std::quoted(e->accessUrl()) << ' '
                << static_cast<int>(e->license()) << ' '
                << e->platformFee().minorUnits();
        } else if (const auto* b = dynamic_cast<const Book*>(&r)) {
            out << "Book " << b->authors().size() << ' ';
            for (const auto& a : b->authors()) out << std::quoted(a) << ' ';
            out << std::quoted(b->isbn()) << ' '
                << b->edition() << ' '
                << static_cast<int>(b->binding());
        } else if (const auto* j = dynamic_cast<const Journal*>(&r)) {
            out << "Journal " << std::quoted(j->issn()) << ' '
                << j->issuesPerYear() << ' ' << j->subscriptionYears();
        } else if (const auto* a = dynamic_cast<const AudioBook*>(&r)) {
            out << "AudioBook " << std::quoted(a->narrator()) << ' '
                << a->durationMinutes();
        } else if (const auto* t = dynamic_cast<const Thesis*>(&r)) {
            out << "Thesis " << std::quoted(t->university()) << ' '
                << std::quoted(t->degree()) << ' '
                << std::quoted(t->supervisor());
        } else if (const auto* e = dynamic_cast<const ElectronicResource*>(&r)) {
            out << "ElectronicResource " << std::quoted(e->accessUrl()) << ' '
                << static_cast<int>(e->license()) << ' '
                << e->platformFee().minorUnits();
        } else {
            throw std::runtime_error("unsupported resource type");
        }

        out << '\n';
    }

    if (!out) throw std::runtime_error("failed writing catalog file");
}
// MT26117_ASHISH
void Catalog::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("cannot open catalog file for reading");
    }

    std::size_t count = 0;
    if (!(in >> count)) {
        throw std::runtime_error("invalid catalog file header");
    }

    Catalog loaded;

    for (std::size_t i = 0; i < count; ++i) {
        std::string id, title, publisher, vendor, type;
        int holding = 0, year = 0;
        std::int64_t priceMinor = 0;

        if (!(in >> std::quoted(id) >> holding >> std::quoted(title)
                 >> std::quoted(publisher) >> year >> priceMinor
                 >> std::quoted(vendor) >> type)) {
            throw std::runtime_error("invalid catalog resource record");
        }

        const Money price = Money::fromMinor(priceMinor);
        std::unique_ptr<Resource> resource;

        if (type == "Magazine") {
            std::string issn;
            int issues = 0, years = 0;
            std::int64_t postage = 0;
            if (!(in >> std::quoted(issn) >> issues >> years >> postage))
                throw std::runtime_error("invalid Magazine record");
            resource = std::make_unique<Magazine>(
                id, title, issn, issues, years, publisher, year, price,
                Money::fromMinor(postage));
        } else if (type == "EBook") {
            std::size_t n = 0;
            if (!(in >> n)) throw std::runtime_error("invalid EBook record");
            std::vector<std::string> authors(n);
            for (auto& a : authors)
                if (!(in >> std::quoted(a)))
                    throw std::runtime_error("invalid EBook author");
            std::string isbn, url;
            int format = 0, drm = 0, license = 0;
            std::int64_t fee = 0;
            if (!(in >> std::quoted(isbn) >> format >> drm >> std::quoted(url)
                     >> license >> fee) ||
                format < 0 || format > 2 || license < 0 || license > 1)
                throw std::runtime_error("invalid EBook record");
            resource = std::make_unique<EBook>(
                id, title, authors, isbn, static_cast<EBookFormat>(format),
                drm != 0, publisher, year, price, url,
                static_cast<LicenseModel>(license), Money::fromMinor(fee));
        } else if (type == "Book") {
            std::size_t n = 0;
            if (!(in >> n)) throw std::runtime_error("invalid Book record");
            std::vector<std::string> authors(n);
            for (auto& a : authors)
                if (!(in >> std::quoted(a)))
                    throw std::runtime_error("invalid Book author");
            std::string isbn;
            int edition = 1, binding = 0;
            if (!(in >> std::quoted(isbn) >> edition >> binding) ||
                binding < 0 || binding > 1)
                throw std::runtime_error("invalid Book record");
            resource = std::make_unique<Book>(
                id, title, authors, isbn, publisher, year, price, edition,
                static_cast<Binding>(binding));
        } else if (type == "Journal") {
            std::string issn;
            int issues = 0, years = 0;
            if (!(in >> std::quoted(issn) >> issues >> years))
                throw std::runtime_error("invalid Journal record");
            resource = std::make_unique<Journal>(
                id, title, issn, issues, years, publisher, year, price);
        } else if (type == "AudioBook") {
            std::string narrator;
            int duration = 0;
            if (!(in >> std::quoted(narrator) >> duration))
                throw std::runtime_error("invalid AudioBook record");
            resource = std::make_unique<AudioBook>(
                id, title, publisher, year, price, narrator, duration);
        } else if (type == "Thesis") {
            std::string university, degree, supervisor;
            if (!(in >> std::quoted(university) >> std::quoted(degree)
                     >> std::quoted(supervisor)))
                throw std::runtime_error("invalid Thesis record");
            resource = std::make_unique<Thesis>(
                id, title, university, degree, supervisor, year, price);
        } else if (type == "ElectronicResource") {
            std::string url;
            int license = 0;
            std::int64_t fee = 0;
            if (!(in >> std::quoted(url) >> license >> fee) ||
                license < 0 || license > 1)
                throw std::runtime_error("invalid ElectronicResource record");
            resource = std::make_unique<ElectronicResource>(
                id, title, publisher, year, price, url,
                static_cast<LicenseModel>(license), Money::fromMinor(fee));
        } else {
            throw std::runtime_error("unknown resource type in catalog file");
        }

        resource->setVendor(vendor);
        loaded.add(std::move(resource));
        if (holding != 0) loaded.addHoldings(id, holding);
    }

    items_.swap(loaded.items_);
}

} // namespace bookmgmt
