package com.stsvita.backend;

/**
 * Low-overhead frame-time distribution for diagnosing ordinary frame pacing.
 *
 * The existing [hitch] log is deliberately limited to frames over 200ms.
 * This accumulator records primitive counters per frame and formats a single
 * summary every five seconds. Phase averages/maxima distinguish pre-render
 * work, game rendering, and swap/vsync waiting without per-frame logging.
 */
final class FramePacingStats {

    private static final long REPORT_INTERVAL_MICROS = 5_000_000L;

    private long windowStartMicros;
    private long frameCount;
    private long totalFrameMicros;
    private long maxFrameMicros;
    private long preTotalMicros;
    private long preMaxMicros;
    private long gameTotalMicros;
    private long gameMaxMicros;
    private long swapTotalMicros;
    private long swapMaxMicros;
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

    void record(long frameMicros, long preMicros, long gameMicros, long swapMicros) {
        if (frameMicros < 0) {
            return;
        }
        frameCount++;
        totalFrameMicros += frameMicros;
        if (frameMicros > maxFrameMicros) {
            maxFrameMicros = frameMicros;
        }

        preTotalMicros += preMicros;
        if (preMicros > preMaxMicros) {
            preMaxMicros = preMicros;
        }
        gameTotalMicros += gameMicros;
        if (gameMicros > gameMaxMicros) {
            gameMaxMicros = gameMicros;
        }
        swapTotalMicros += swapMicros;
        if (swapMicros > swapMaxMicros) {
            swapMaxMicros = swapMicros;
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
        long preAverageMicros = frameCount == 0 ? 0 : preTotalMicros / frameCount;
        long gameAverageMicros = frameCount == 0 ? 0 : gameTotalMicros / frameCount;
        long swapAverageMicros = frameCount == 0 ? 0 : swapTotalMicros / frameCount;
        StringBuilder report = new StringBuilder(280);
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
                .append(" >100ms=").append(over100ms)
                .append(" pre_avg=");
        appendMillis(report, preAverageMicros);
        report.append(" pre_max=");
        appendMillis(report, preMaxMicros);
        report.append(" game_avg=");
        appendMillis(report, gameAverageMicros);
        report.append(" game_max=");
        appendMillis(report, gameMaxMicros);
        report.append(" swap_avg=");
        appendMillis(report, swapAverageMicros);
        report.append(" swap_max=");
        appendMillis(report, swapMaxMicros);

        windowStartMicros = nowMicros;
        frameCount = 0;
        totalFrameMicros = 0;
        maxFrameMicros = 0;
        preTotalMicros = 0;
        preMaxMicros = 0;
        gameTotalMicros = 0;
        gameMaxMicros = 0;
        swapTotalMicros = 0;
        swapMaxMicros = 0;
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
