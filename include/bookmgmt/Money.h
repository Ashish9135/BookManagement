#pragma once
// MT26117_ASHISH
// Money stores a fixed-point amount and its currency code.

#include <cstdint>
#include <iosfwd>
#include <string>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    // Existing calls default to USD for backward compatibility.
    static Money fromMinor(std::int64_t minor, const std::string& currency = "USD") {
        return Money(minor, currency);
    }

    static Money of(std::int64_t major, int minor = 0,
                    const std::string& currency = "USD");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currency() const { return currency_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(const Money& other) {
        requireSameCurrency(other);
        minor_ += other.minor_;
        return *this;
    }

    Money& operator-=(const Money& other) {
        requireSameCurrency(other);
        minor_ -= other.minor_;
        return *this;
    }

    Money& operator*=(std::int64_t k) {
        minor_ *= k;
        return *this;
    }

    friend Money operator+(Money a, const Money& b) { return a += b; }
    friend Money operator-(Money a, const Money& b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ == b.minor_;
    }
    friend bool operator!=(const Money& a, const Money& b) { return !(a == b); }
    friend bool operator<(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ < b.minor_;
    }
    friend bool operator<=(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ <= b.minor_;
    }
    friend bool operator>(const Money& a, const Money& b) { return b < a; }
    friend bool operator>=(const Money& a, const Money& b) { return b <= a; }

private:
    Money(std::int64_t minor, std::string currency)
        : minor_(minor), currency_(std::move(currency)) {
        if (currency_.empty()) {
            throw std::invalid_argument("currency code cannot be empty");
        }
    }

    void requireSameCurrency(const Money& other) const {
        if (currency_ != other.currency_) {
            throw std::invalid_argument("cannot operate on different currencies");
        }
    }

    std::int64_t minor_ = 0;
    std::string currency_ = "USD";
};

std::ostream& operator<<(std::ostream& os, Money m);

}  // namespace bookmgmt
