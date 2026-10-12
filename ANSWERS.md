# Assignment 2 — Answers and Design Decisions

## Questions attempted
Q1–Q16 are represented in the current project implementation and tests.

## Design decisions
- **Q1 Journal:** derives directly from `Resource`; subscription years affect the total cost.
- **Q2 EBook:** derives from `ElectronicResource` to reuse licensing, URL, platform-fee and seat-pricing behavior. The authors and ISBN metadata are duplicated with `Book`; a shared book-metadata component/base class could remove this duplication.
- **Q3 AudioBook and Thesis:** both derive from `Resource`, since they are catalogue items but do not have the same semantics as print `Book` or licensed `ElectronicResource`. AudioBook applies the print-item bulk discount. Thesis defaults to a zero price.
- **Q4–Q6 pricing:** hardcover pricing, bulk/tiered pricing, and configurable print/electronic tax rates are implemented in the resource/acquisition layers.
- **Q7–Q10 budget controls:** category title limits, cancellation records/refunds, department budgets, and configurable year-end rollover are implemented in `AcquisitionManager`/`Budget`.
- **Q11 batches:** the all-or-nothing batch option validates using simulated budgets before committing.
- **Q12 vendors:** catalogue entries can carry vendor identity; cheapest-vendor selection compares unit prices for matching titles and stores the selected vendor in the order record.
- **Q13 searches:** catalogue supports author, ISBN/ISSN, and year-range searches.
- **Q14 lending:** print-copy loans and electronic sessions are tracked separately and checked against holdings/seats.
- **Q15 Money:** each amount has a currency code; arithmetic and comparisons reject mismatched currencies.
- **Q16 tests:** test checks are expressed with `assert` and a small exception-expectation helper.

## Written answers
1. An e-book is also a kind of book. The duplicated code/data are author names and ISBN handling/printing. A shared `BookMetadata` component or common base class could centralize that data while allowing `EBook` to retain its electronic licensing behavior.
2. `AudioBook` derives from `Resource` because it has a price and catalogue identity but is neither a physical printed book nor a licensed electronic database. `Thesis` derives from `Resource` because its university/degree/supervisor metadata differs from book metadata and it is commonly free.

## Building:

- cmake -S . -B build
- cmake --build build
- ./build/bookmgmt_tests # runs the tests
- ./build/demo # demonstrates every feature
