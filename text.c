#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>

enum ent_noise_type {
    ENT_NOISE_WHITE,
    ENT_NOISE_PINK,
    ENT_NOISE_BROWN
};

enum ent_filter_type {
    ENT_FILTER_NONE,
    ENT_FILTER_LOW_PASS,
    ENT_FILTER_HIGH_PASS
};

int main(void)
{
    _Atomic(float) density;
    _Atomic(double) tempo;
    _Atomic(bool) enabled;

    _Atomic(enum ent_noise_type) noise_type;
    _Atomic(enum ent_filter_type) filter_type;

    atomic_init(&density, 0.5f);
    atomic_init(&tempo, 120.0);
    atomic_init(&enabled, true);
    atomic_init(&noise_type, ENT_NOISE_WHITE);
    atomic_init(&filter_type, ENT_FILTER_NONE);

    printf("float:       %s\n", atomic_is_lock_free(&density) ? "lock-free" : "not lock-free");
    printf("double:      %s\n", atomic_is_lock_free(&tempo) ? "lock-free" : "not lock-free");
    printf("bool:        %s\n", atomic_is_lock_free(&enabled) ? "lock-free" : "not lock-free");
    printf("noise enum:  %s\n", atomic_is_lock_free(&noise_type) ? "lock-free" : "not lock-free");
    printf("filter enum: %s\n", atomic_is_lock_free(&filter_type) ? "lock-free" : "not lock-free");

    return 0;
}
