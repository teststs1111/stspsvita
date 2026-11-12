package com.stsvita.backend;

/**
 * Low-overhead frame-time distribution for diagnosing ordinary frame pacing.
 *
 * The existing [hitch] log is deliberately limited to frames over 200ms.
 * This accumulator records one primitive counter per frame and formats a
 * single summary every five seconds, exposing missed 60Hz frames without
 * allocating or writing to storage in the render hot path.
 */
final class FramePacingStats {

    private static final long REPORT_INTERVAL_MICROS = 5_000_000L;

    private long windowStartMicros;
    private long frameCount;
    private long totalFrameMicros;
    private long maxFrameMicros;
    private long le16_7ms;
    private long le20ms;
    private long le25ms;
    private long le33_3ms;
    private long le50ms;
    private long le100ms;
    private long over100ms;

    FramePacingStats(long windowStartMicros) {
        this.windowStartMicros = windowStartMicros;
    }

    void record(long frameMicros) {
        if (frameMicros < 0) {
            return;
        }
        frameCount++;
        totalFrameMicros += frameMicros;
        if (frameMicros > maxFrameMicros) {
            maxFrameMicros = frameMicros;
        }

        if (frameMicros <= 16_667L) {
            le16_7ms++;
        } else if (frameMicros <= 20_000L) {
            le20ms++;
        } else if (frameMicros <= 25_000L) {
            le25ms++;
        } else if (frameMicros <= 33_333L) {
            le33_3ms++;
        } else if (frameMicros <= 50_000L) {
            le50ms++;
        } else if (frameMicros <= 100_000L) {
            le100ms++;
        } else {
            over100ms++;
        }
    }

    boolean shouldReport(long nowMicros) {
        return nowMicros - windowStartMicros >= REPORT_INTERVAL_MICROS;
    }

    String reportAndReset(long nowMicros) {
        long elapsedMicros = nowMicros - windowStartMicros;
        long averageMicros = frameCount == 0 ? 0 : totalFrameMicros / frameCount;
        StringBuilder report = new StringBuilder(176);
        report.append("[frame-pacing] window=")
                .append(elapsedMicros / 1000L)
                .append("ms frames=")
                .append(frameCount)
                .append(" avg=");
        appendMillis(report, averageMicros);
        report.append(" max=");
        appendMillis(report, maxFrameMicros);
        report.append(" <=16.7ms=").append(le16_7ms)
                .append(" <=20ms=").append(le20ms)
                .append(" <=25ms=").append(le25ms)
                .append(" <=33.3ms=").append(le33_3ms)
                .append(" <=50ms=").append(le50ms)
                .append(" <=100ms=").append(le100ms)
                .append(" >100ms=").append(over100ms);

        windowStartMicros = nowMicros;
        frameCount = 0;
        totalFrameMicros = 0;
        maxFrameMicros = 0;
        le16_7ms = 0;
        le20ms = 0;
        le25ms = 0;
        le33_3ms = 0;
        le50ms = 0;
        le100ms = 0;
        over100ms = 0;
        return report.toString();
    }

    private static void appendMillis(StringBuilder out, long micros) {
        out.append(micros / 1000L)
                .append('.')
                .append((micros % 1000L) / 100L)
                .append("ms");
    }
}
