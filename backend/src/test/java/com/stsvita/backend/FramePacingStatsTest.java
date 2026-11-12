package com.stsvita.backend;

/** Standalone host regression test; no game classes or Vita SDK required. */
public final class FramePacingStatsTest {

    public static void main(String[] args) {
        FramePacingStats stats = new FramePacingStats(0L);
        stats.record(16_667L);
        stats.record(16_668L);
        stats.record(20_000L);
        stats.record(25_000L);
        stats.record(33_333L);
        stats.record(50_000L);
        stats.record(100_000L);
        stats.record(250_000L);

        check(!stats.shouldReport(4_999_999L), "reported before five seconds");
        check(stats.shouldReport(5_000_000L), "did not report at five seconds");

        String report = stats.reportAndReset(5_000_000L);
        contains(report, "window=5000ms");
        contains(report, "frames=8");
        contains(report, "avg=63.9ms");
        contains(report, "max=250.0ms");
        contains(report, "<=16.7ms=1");
        contains(report, "<=20ms=2");
        contains(report, "<=25ms=1");
        contains(report, "<=33.3ms=1");
        contains(report, "<=50ms=1");
        contains(report, "<=100ms=1");
        contains(report, ">100ms=1");

        check(!stats.shouldReport(9_999_999L), "window did not reset");
        check(stats.shouldReport(10_000_000L), "second window did not elapse");
        String empty = stats.reportAndReset(10_000_000L);
        contains(empty, "frames=0");
        contains(empty, "avg=0.0ms");
        contains(empty, "max=0.0ms");

        System.out.println("FramePacingStatsTest: PASS");
    }

    private static void contains(String actual, String expected) {
        check(actual.contains(expected),
                "expected <" + expected + "> in <" + actual + ">");
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }
}
