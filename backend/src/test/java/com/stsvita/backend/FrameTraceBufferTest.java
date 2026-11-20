package com.stsvita.backend;

/** Standalone host regression test; no game classes or Vita SDK required. */
public final class FrameTraceBufferTest {

    public static void main(String[] args) {
        FrameTraceBuffer trace = new FrameTraceBuffer(0L);
        check(!trace.shouldDump(99_999L, 0L), "dumped below threshold");
        trace.record(16_000L, 1_000L, 5_000L, 10_000L,
                2507L, 2003L, 3001L, 4002L, 5004L, 6005L, 7006L);
        check(trace.shouldDump(100_000L, 0L), "first slow frame not dumped");
        String first = trace.dumpRecent();
        contains(first, "[frame-trace] recent=1 capacity=8");
        contains(first, "#0 frame=16.0ms");
        contains(first, "game=5.0ms");
        contains(first, "ft=2ms/507 calls");
        contains(first, "io=6ms/5 calls");
        check(!trace.shouldDump(200_000L, 1_999_999L),
                "dump rate limit ignored");
        check(trace.shouldDump(200_000L, 2_000_000L),
                "dump did not resume after rate limit");

        for (int i = 1; i <= 10; i++) {
            trace.record(i * 1_000L, 0L, 0L, 0L, 0L, 0L,
                    0L, 0L, 0L, 0L, 0L);
        }
        String wrapped = trace.dumpRecent();
        contains(wrapped, "recent=8 capacity=8");
        check(!wrapped.contains("#2 "), "oldest entry was not overwritten");
        contains(wrapped, "#3 frame=3.0ms");
        contains(wrapped, "#10 frame=10.0ms");
        System.out.println("FrameTraceBufferTest: PASS");
    }

    private static void contains(String actual, String expected) {
        check(actual.contains(expected),
                "expected <" + expected + "> in <" + actual + ">");
    }

    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
