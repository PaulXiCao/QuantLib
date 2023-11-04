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
#include <ql/currencies/america.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/instruments/bonds/amortizingfixedratebond.hpp>
#include <ql/quotes/derivedquote.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/time/calendars/chile.hpp>
#include <ql/time/calendars/jointcalendar.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/date.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/business252.hpp>
#include <ql/time/frequency.hpp>
#include <ql/time/period.hpp>
#include <ql/time/schedule.hpp>
#include <charconv>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

using namespace QuantLib;
using namespace boost::unit_test_framework;

namespace {

    struct Divider {
        Divider(const int divisor) : divisor(divisor) {}
        int divisor;
        Real operator()(const Real x) const { return x / divisor; }
    };

    class MarketData {
      private:
        std::map<std::string_view, SimpleQuote> quotes;

      public:
        MarketData() {
            const std::map<std::string_view, double> prices = {
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

      private:
        SimpleQuote get_quote(const std::string_view ticker) {
            if (!quotes.contains(ticker))
                quotes[ticker] = SimpleQuote(0.0);
            return quotes[ticker];
        }

      public:
        DerivedQuote<Divider> get_derived_quote(const std::string_view ticker, const int divisor) {
            const auto quote = get_quote(ticker);
            const Handle<Quote> handle(std::make_shared<SimpleQuote>(quote));
            return {handle, {divisor}};
        }
    };
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
        MarketData md;

        RelinkableHandle<YieldTermStructure> handle;
        const auto index = std::make_shared<OvernightIndex>("CLICP Index", 2, CLPCurrency(),
                                                            Chile(), Actual360(), handle);

        JointCalendar calendar({UnitedStates(UnitedStates::Market::FederalReserve)}, Chile());
        const auto GetPeriod = [](const std::string_view tenor) -> Period {
            const std::string_view digits = tenor.substr(0, (tenor.size() == 2) ? 1 : 2);
            int n;
            std::from_chars(digits.data(), digits.data() + digits.size(), n);
            const auto units = (tenor.back() == 'M') ? TimeUnit::Months : TimeUnit::Years;
            return {n, units};
        };

        std::vector<ext::shared_ptr<RateHelper>> helpers;
        // std::vector<ext::shared_ptr<OISRateHelper>> helpers;
        const std::array<std::array<std::string_view, 2>, 17> tenorsAndTickers(
            {{"3M", "CHSWPC Curncy"},
             {"6M", "CHSWPF Curncy"},
             {"9M", "CHSWPI Curncy"},
             {"1Y", "CHSWP1 Curncy"},
             {"18M", "CHSWP1F Curncy"},
             {"2Y", "CHSWP2 Curncy"},
             {"3Y", "CHSWP3 Curncy"},
             {"4Y", "CHSWP4 Curncy"},
             {"5Y", "CHSWP5 Curncy"},
             {"6Y", "CHSWP6 Curncy"},
             {"7Y", "CHSWP7 Curncy"},
             {"8Y", "CHSWP8 Curncy"},
             {"9Y", "CHSWP9 Curncy"},
             {"10Y", "CHSWP10 Curncy"},
             {"12Y", "CHSWP12 Curncy"},
             {"15Y", "CHSWP15 Curncy"},
             {"20Y", "CHSWP20 Curncy"}});

        for (const auto [tenor, ticker] : tenorsAndTickers) {
            const Natural settlementDays = 2;
            const Period tenorPeriod = GetPeriod(tenor);
            constexpr auto divisor = 100;
            const DerivedQuote<Divider> dq = md.get_derived_quote(ticker, divisor);
            const Handle<Quote> fixedRate(std::make_shared<DerivedQuote<Divider>>(dq));
            constexpr auto telescopicValueDates = false;
            constexpr Integer paymentLag = 0; // default
            constexpr auto paymentConvention = BusinessDayConvention::ModifiedFollowing;
            const Frequency paymentFrequency =
                (tenorPeriod > GetPeriod("18M")) ? Frequency::Semiannual : Frequency::Once;
            const Period forwardStart = 0 * Days;                                // default
            const Spread overnightSpread = 0.0;                                  // default
            const Pillar::Choice pillar = Pillar::LastRelevantDate;              // default
            const Date customPillarDate = Date();                                // default
            const RateAveraging::Type averagingMethod = RateAveraging::Compound; // default
            const ext::optional<bool> endOfMonth = false;

            const OISRateHelper helper(settlementDays, tenorPeriod, fixedRate, index, handle,
                                       telescopicValueDates, paymentLag, paymentConvention,
                                       paymentFrequency, calendar, forwardStart, overnightSpread,
                                       pillar, customPillarDate, averagingMethod, endOfMonth);

            helpers.push_back(std::make_shared<OISRateHelper>(helper));
        }
        /*
         */

        const Natural settlementDays = 0;
        const DayCounter dayCounter = Actual360();
        const std::vector<Handle<Quote>> jumps = {}; // default
        const std::vector<Date> jumpDates = {};      // default
        const LogCubic& interpolator = DefaultLogCubic();
        PiecewiseYieldCurve<Discount, LogCubic> PiecewiseLogCubicDiscount(
            settlementDays, calendar, helpers, dayCounter, jumps, jumpDates, interpolator);
    }


    BOOST_TEST_MESSAGE("Testing amortizing fixed rate bond...");
}


test_suite* CalendarsTest::suite() {
    auto* suite = BOOST_TEST_SUITE("Calendars tests");
    suite->add(QUANTLIB_TEST_CASE(&CalendarsTest::testCalendars));
    return suite;
}
