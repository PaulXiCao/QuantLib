/*
 Copyright (C) 2014 Cheng Li

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "aaa_calendars.hpp"
#include "utilities.hpp"
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/instruments/bonds/amortizingfixedratebond.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendars/brazil.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/date.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/business252.hpp>
#include <iostream>
#include <map>
#include <string>

using namespace QuantLib;
using namespace boost::unit_test_framework;

namespace {

    class MarketData {
      private:
        std::map<std::string, SimpleQuote> quotes;

      public:
        MarketData() {
            const std::map<std::string, double> prices = {
                {"CHSWP20 Curncy", 5.145}, {"CHSWP10 Curncy", 5.015}, {"CHSWP9 Curncy", 5.01},
                {"CHSWP8 Curncy", 5.045},  {"CHSWP7 Curncy", 5.085},  {"CHSWP6 Curncy", 5.155},
                {"CHSWP5 Curncy", 5.267},  {"CHSWP12 Curncy", 5.055}, {"CHSWP4 Curncy", 5.545},
                {"CHSWP2 Curncy", 6.88},   {"CHSWP1F Curncy", 7.84},  {"CHSWP1 Curncy", 9.028},
                {"CHSWPI Curncy", 9.755},  {"CHSWPF Curncy", 10.44},  {"CHSWPC Curncy", 10.995},
                {"CHSWP3 Curncy", 6.015},  {"CHSWP15 Curncy", 5.075},
            };
            for (const auto& [ticker, price] : prices)
                quotes[ticker] = SimpleQuote(price);
        }
    };

    /*
        prices: dict[str, float]
        _quotes: dict[str, ql.SimpleQuote] = field(init=False, default_factory=dict)

        def __post_init__(self) -> None:
            for ticker, price in self.prices.items():
                self._quotes[ticker] = ql.SimpleQuote(price)

        def get_quote(self, ticker: str) -> ql.SimpleQuote:
            if ticker not in self._quotes:
                self._quotes[ticker] = ql.SimpleQuote(0.0)
            return self._quotes[ticker]

        def get_derived_quote(self, ticker: str, divisor: int) -> ql.DerivedQuote:
            quote = self.get_quote(ticker)
            return ql.DerivedQuote(ql.QuoteHandle(quote), lambda x: x / divisor)
        };
    */
}

void CalendarsTest::testCalendars() {
    const auto DaysInJune2023 = []<auto N>(const std::array<int, N>& days) -> std::array<Date, N> {
        std::array<Date, N> dates;
        for (auto i = 0u; i < N; ++i)
            dates[i] = Date(days[i], Month::June, 2023);
        return dates;
    };

    for (const auto eval_date : DaysInJune2023(std::array{15, 16, 20})) {
        Settings::instance().evaluationDate() = eval_date;
        const auto md = MarketData();
    }


    Handle<YieldTermStructure> handle;
    const OvernightIndex index("CLICP Index", 2, ql.CLPCurrency(), ql.Chile(), ql.Actual360(),
                               handle, );


    BOOST_TEST_MESSAGE("Testing amortizing fixed rate bond...");
}


test_suite* CalendarsTest::suite() {
    auto* suite = BOOST_TEST_SUITE("Calendars tests");
    suite->add(QUANTLIB_TEST_CASE(&CalendarsTest::testCalendars));
    return suite;
}
