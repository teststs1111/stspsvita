#ifndef STSV_SPILL_TRACKER_H
#define STSV_SPILL_TRACKER_H

#include <stddef.h>

typedef struct {
    int id;
    size_t bytes;
} stsv_spill_entry;

static inline int stsv_spill_tracker_slot(const stsv_spill_entry *entries,
                                          size_t capacity, int id) {
    if (id == 0) return -1;
    int free_slot = -1;
    for (size_t i = 0; i < capacity; i++) {
        if (entries[i].id == id) return (int) i;
        if (entries[i].id == 0 && free_slot < 0) free_slot = (int) i;
    }
    return free_slot;
}

static inline int stsv_spill_tracker_can_admit(const stsv_spill_entry *entries,
                                                size_t capacity, int id,
                                                size_t bytes, size_t budget,
                                                size_t used) {
    int slot = stsv_spill_tracker_slot(entries, capacity, id);
    if (slot < 0) return 0;
    size_t previous = entries[slot].id == id ? entries[slot].bytes : 0;
    if (used < previous) return 0;
    size_t projected = used - previous;
    return projected <= budget && bytes <= budget - projected;
}

static inline int stsv_spill_tracker_admit(stsv_spill_entry *entries,
                                            size_t capacity, int id,
                                            size_t bytes, size_t budget,
                                            size_t *used) {
    if (used == NULL
            || !stsv_spill_tracker_can_admit(entries, capacity, id,
                                              bytes, budget, *used)) {
        return 0;
    }
    int slot = stsv_spill_tracker_slot(entries, capacity, id);
    size_t previous = entries[slot].id == id ? entries[slot].bytes : 0;
    *used = *used - previous + bytes;
    entries[slot].id = id;
    entries[slot].bytes = bytes;
    return 1;
}

static inline int stsv_spill_tracker_refund(stsv_spill_entry *entries,
                                             size_t capacity, int id,
                                             size_t *used) {
    if (id == 0 || used == NULL) return 0;
    int refunded = 0;
    for (size_t i = 0; i < capacity; i++) {
        if (entries[i].id == id) {
            if (*used >= entries[i].bytes) *used -= entries[i].bytes;
            else *used = 0;
            entries[i].id = 0;
            entries[i].bytes = 0;
            refunded = 1;
        }
    }
    return refunded;
}

#endif
