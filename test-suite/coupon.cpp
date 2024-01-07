/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2024 Paul Xi Cao

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

#include "toplevelfixture.hpp"
#include <ql/cashflow.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/interestrate.hpp>
#include <ql/patterns/visitor.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/time/schedule.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(CouponTests)

BOOST_AUTO_TEST_CASE(test_Coupon_AccrualPeriod) {
    BOOST_TEST_MESSAGE("Testing accrual period calculations...");

    const Date paymentDate(1, Apr, 2020);
    const Real nominal = 1;
    const Rate rate = 0.05;
    const SimpleDayCounter dayCounter;
    const Date accrualStartDate(1, Jan, 2020);
    const Date accrualEndDate(1, Mar, 2020);
    const FixedRateCoupon coupon(paymentDate, nominal, rate, dayCounter, accrualStartDate,
                                 accrualEndDate);

    // full accrual period
    BOOST_TEST(coupon.accrualStartDate() == accrualStartDate);
    BOOST_TEST(coupon.accrualEndDate() == accrualEndDate);
    BOOST_TEST(coupon.accrualPeriod() == (2.0 / 12.0)); // see SimpleDayCounter
    BOOST_TEST(coupon.accrualDays() == 60);             // see SimpleDayCounter

    // partial accrued period
    const Date middleOfAccrualPeriod(1, Feb, 2020);
    BOOST_TEST(coupon.accruedPeriod(middleOfAccrualPeriod) == (1.0 / 12.0)); // see SimpleDayCounter
    BOOST_TEST(coupon.accruedDays(middleOfAccrualPeriod) == 30);             // see SimpleDayCounter
}

BOOST_AUTO_TEST_CASE(test_FixedRateCoupon_accept) {
    BOOST_TEST_MESSAGE("Test visiting FixedRateCoupon...");

    const Date paymentDate(1, Apr, 2020);
    const Real nominal = 1;
    const Rate rate = 0.05;
    const SimpleDayCounter dayCounter;
    const Date accrualStartDate(1, Jan, 2020);
    const Date accrualEndDate(1, Mar, 2020);
    FixedRateCoupon coupon(paymentDate, nominal, rate, dayCounter, accrualStartDate,
                           accrualEndDate);

    struct TestVisitor final : public AcyclicVisitor, public Visitor<FixedRateCoupon> {
        Rate rate;
        void visit(FixedRateCoupon& c) override { rate = c.rate(); }
    };
    TestVisitor visitor;
    coupon.accept(visitor);

    BOOST_TEST(visitor.rate == 0.05);
}

BOOST_AUTO_TEST_CASE(test_FixedRateLeg_withCouponRates_InterestRate) {
    BOOST_TEST_MESSAGE("Test setting FixedRateLeg via withCouponRates...");

    const Date date(1, Jan, 2020);
    const Schedule schedule({date, date + Period(1, Years), date + Period(4, Years)});

    const Rate notional = 100;
    const InterestRate rate = []() -> InterestRate {
        const Rate r = 0.05;
        const SimpleDayCounter dc;
        const Compounding comp = Simple;
        const Frequency freq = NoFrequency;
        return {r, dc, comp, freq};
    }();
    const FixedRateLeg frLeg = FixedRateLeg(schedule).withNotionals(notional).withCouponRates(rate);

    const Leg leg(frLeg);

    BOOST_ASSERT(leg.size() == 2);
    const auto tol = boost::test_tools::tolerance(1e-15);
    BOOST_TEST(leg.at(0)->amount() == 5, tol);  // 1 year (note: Simple Compounding)
    BOOST_TEST(leg.at(1)->amount() == 15, tol); // 3 more years (note: Simple Compounding)
}

#define UNUSED(x) (void)x

BOOST_AUTO_TEST_CASE(test_FloatingRateCoupon_convexityAdjustment) {
    BOOST_TEST_MESSAGE("Testing convexity adjustment of FloatingRateCoupon...");

    const Date paymentDate(1, July, 2020);
    Real nominal = 123;
    const Date startDate(1, April, 2020);
    const Date endDate(30, June, 2020);
    Natural fixingDays = 0;
    const ext::shared_ptr<InterestRateIndex> index = ext::make_shared<Euribor6M>();
    // ext::shared_ptr<InterestRateIndex> i1 = ext::make_shared<Euribor6M>();
    // UNUSED(i1);

    const auto& ts = index->timeSeries();
    std::cout << "ts size: " << ts.size() << '\n';
    for (const auto kv : ts) {
        std::cout << "date: " << kv.first << ", fixing: " << kv.second << '\n';
        ;
    }
    const Real gearing = 1.5;
    const Spread spread = 0.3;
    const FloatingRateCoupon coupon(paymentDate, nominal, startDate, endDate, fixingDays, index,
                                    gearing, spread);

    index->addFixing(startDate, 0.0123);

    std::cout << "fixing: " << coupon.fixingDate() << '\n';
    std::cout << "fixing: " << coupon.indexFixing() << '\n';


    UNUSED(index);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
