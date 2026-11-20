package com.stsvita.backend;

/**
 * Fixed-size, allocation-free frame history. Records primitive timings on the
 * render thread and formats text only when a slow frame is detected.
 */
final class FrameTraceBuffer {

    private static final int CAPACITY = 8;
    private static final int FIELDS = 11;
    private static final long DUMP_INTERVAL_MICROS = 2_000_000L;

    private static final String[] NAMES = {
        "frame", "pre", "game", "swap", "ft", "subtex",
        "stb", "dxt", "up", "io", "stat"
    };

    private final long[] values = new long[CAPACITY * FIELDS];
    private int next;
    private int size;
    private long sequence;
    private long lastDumpMicros;

    FrameTraceBuffer(long startMicros) {
        lastDumpMicros = startMicros - DUMP_INTERVAL_MICROS;
    }

    void record(long frame, long pre, long game, long swap, long ft,
                long subtex, long stb, long dxt, long upload,
                long io, long stat) {
        int offset = next * FIELDS;
        values[offset] = frame;
        values[offset + 1] = pre;
        values[offset + 2] = game;
        values[offset + 3] = swap;
        values[offset + 4] = ft;
        values[offset + 5] = subtex;
        values[offset + 6] = stb;
        values[offset + 7] = dxt;
        values[offset + 8] = upload;
        values[offset + 9] = io;
        values[offset + 10] = stat;
        next = (next + 1) % CAPACITY;
        if (size < CAPACITY) {
            size++;
        }
        sequence++;
    }

    boolean shouldDump(long frameMicros, long nowMicros) {
        if (frameMicros < 100_000L
                || nowMicros - lastDumpMicros < DUMP_INTERVAL_MICROS) {
            return false;
        }
        lastDumpMicros = nowMicros;
        return true;
    }

    String dumpRecent() {
        StringBuilder out = new StringBuilder(900);
        out.append("[frame-trace] recent=").append(size)
                .append(" capacity=").append(CAPACITY);
        int first = (next - size + CAPACITY) % CAPACITY;
        long firstSequence = sequence - size;
        for (int i = 0; i < size; i++) {
            int slot = (first + i) % CAPACITY;
            int offset = slot * FIELDS;
            out.append('\n').append("  #").append(firstSequence + i);
            for (int field = 0; field < FIELDS; field++) {
                long value = values[offset + field];
                out.append(' ').append(NAMES[field]).append('=');
                if (field < 4) {
                    out.append(value / 1000L).append('.')
                            .append((value % 1000L) / 100L).append("ms");
                } else {
                    // Native/file counters are packed as milliseconds*1000 + calls.
                    out.append(value / 1000L).append("ms/")
                            .append(value % 1000L).append(" calls");
                }
            }
        }
        return out.toString();
    }
}
