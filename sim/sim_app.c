#include <stdio.h>
#include "gateway.h"
#include "frame_timer_sw.h"

int main(void) {
    printf("=== Gateway Simulator v1.0 ===\n");
    printf("Libraries linked: hal_core hal_sw gateway_core gateway_brands\n");
    printf("Ready for interactive testing.\n");
    printf("(Full sim pending - currently validating build system)\n");

    /* Smoke test: create a software timer */
    frame_timer_t *t = frame_timer_sw_create(1000);
    if (t) {
        printf("[OK] Software timer created\n");
        frame_timer_sw_destroy(t);
    } else {
        printf("[FAIL] Timer creation failed\n");
        return 1;
    }

    printf("[OK] All smoke tests passed\n");
    return 0;
}
