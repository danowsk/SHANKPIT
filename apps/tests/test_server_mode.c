#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

#include "../server/src/server_mode.h"
#include "../../packages/common/protocol.h"

int tests_run = 0;
int tests_passed = 0;

#define ASSERT_EQ(a, b, msg) do { \
    tests_run++; \
    if ((a) != (b)) { \
        printf("❌ FAIL: %s (Expected %d, Got %d)\n", msg, (int)(a), (int)(b)); \
    } else { \
        printf("✅ PASS: %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

static void test_default_mode(void) {
    printf("--- Testing Default Server Mode ---\n");
    char *args[] = { "server" };
    ASSERT_EQ(parse_server_mode(1, args), MODE_DEATHMATCH, "Default mode is deathmatch");
}

static void test_tdm_mode(void) {
    printf("--- Testing TDM Server Mode ---\n");
    char *args[] = { "server", "--tdm" };
    ASSERT_EQ(parse_server_mode(2, args), MODE_TDM, "TDM flag sets team deathmatch");
}

static void test_tdmo_mode(void) {
    printf("--- Testing TDMO Server Mode ---\n");
    char *args[] = { "server", "--tdmo" };
    ASSERT_EQ(parse_server_mode(2, args), MODE_TDMO, "TDMO flag sets online team deathmatch");
}

int parse_server_mode(int argc, char **argv) {
    int mode = MODE_DEATHMATCH;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tdm") == 0) {
            mode = MODE_TDM;
        } else if (strcmp(argv[i], "--tdmo") == 0) {
            mode = MODE_TDMO;
        } else if (strcmp(argv[i], "--deathmatch") == 0) {
            mode = MODE_DEATHMATCH;
        }
    }
    return mode;
}

int main(void) {
    printf("🛡️ SHANKPIT SERVER MODE CHECK 🛡️\n");
    test_default_mode();
    test_tdm_mode();
    test_tdmo_mode();
    printf("\n--------------------------------------\n");
    printf("SUMMARY: %d/%d Tests Passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
