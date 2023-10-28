from dataclasses import dataclass, field

import QuantLib as ql


@dataclass
class MarketData:
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


def main() -> None:
    prices = {
        "CHSWP20 Curncy": 5.145,
        "CHSWP10 Curncy": 5.015,
        "CHSWP9 Curncy": 5.01,
        "CHSWP8 Curncy": 5.045,
        "CHSWP7 Curncy": 5.085,
        "CHSWP6 Curncy": 5.155,
        "CHSWP5 Curncy": 5.267,
        "CHSWP12 Curncy": 5.055,
        "CHSWP4 Curncy": 5.545,
        "CHSWP2 Curncy": 6.88,
        "CHSWP1F Curncy": 7.84,
        "CHSWP1 Curncy": 9.028,
        "CHSWPI Curncy": 9.755,
        "CHSWPF Curncy": 10.44,
        "CHSWPC Curncy": 10.995,
        "CHSWP3 Curncy": 6.015,
        "CHSWP15 Curncy": 5.075,
    }
    for eval_date in [ql.Date(15, 6, 2023), ql.Date(16, 6, 2023), ql.Date(20, 6, 2023)]:
        ql.Settings.instance().evaluationDate = eval_date
        md = MarketData(prices=prices)

        handle = ql.RelinkableYieldTermStructureHandle()
        index = ql.OvernightIndex(
            "CLICP Index",
            2,
            ql.CLPCurrency(),
            ql.Chile(),
            ql.Actual360(),
            handle,
        )
        calendar = ql.JointCalendar(ql.UnitedStates(ql.UnitedStates.FederalReserve), ql.Chile())
        helpers = [
            ql.OISRateHelper(
                2,
                ql.Period(tenor),
                ql.QuoteHandle(md.get_derived_quote(ticker, divisor=100)),
                index,
                handle,
                telescopicValueDates=False,
                paymentConvention=ql.ModifiedFollowing,
                paymentFrequency=ql.Semiannual if ql.Period(tenor) > ql.Period("18M") else ql.Once,
                paymentCalendar=calendar,
                endOfMonth=False,
            )
            for tenor, ticker in [
                ("3M", "CHSWPC Curncy"),
                ("6M", "CHSWPF Curncy"),
                ("9M", "CHSWPI Curncy"),
                ("1Y", "CHSWP1 Curncy"),
                ("18M", "CHSWP1F Curncy"),
                ("2Y", "CHSWP2 Curncy"),
                ("3Y", "CHSWP3 Curncy"),
                ("4Y", "CHSWP4 Curncy"),
                ("5Y", "CHSWP5 Curncy"),
                ("6Y", "CHSWP6 Curncy"),
                ("7Y", "CHSWP7 Curncy"),
                ("8Y", "CHSWP8 Curncy"),
                ("9Y", "CHSWP9 Curncy"),
                ("10Y", "CHSWP10 Curncy"),
                ("12Y", "CHSWP12 Curncy"),
                ("15Y", "CHSWP15 Curncy"),
                ("20Y", "CHSWP20 Curncy"),
            ]
        ]
        curve = ql.PiecewiseLogCubicDiscount(0, calendar, helpers, ql.Actual360())
        curve.enableExtrapolation()

        print(f"Evaluation Date: {eval_date}")
        for date in set([helper.earliestDate() for helper in helpers]):
            print(" " * 4 + f"Helper Date: {date}")
        print(" " * 4 + f"Calendar Advanced Date: {calendar.advance(eval_date, ql.Period(2, ql.Days))}")


if __name__ == "__main__":
    main()
