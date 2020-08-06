#if !defined(TEST_CPP)
#define TEST_CPP

#define CATCH_CONFIG_MAIN // main is defined

#include "catch.hpp"

int hello(int x) {
    if (x <= 5)
        return 1;
    else if (x <= 100)
        return 2;
    else
        return 3;
}

TEST_CASE("TEST hello", "[hello]") {
    REQUIRE(hello(-3) == 1);
    REQUIRE(hello(4) == 1);
    REQUIRE(hello(5) == 1);
    REQUIRE(hello(6) == 2);
    REQUIRE(hello(99) == 2);
    REQUIRE(hello(101) == 3);
}

#endif // TEST_CPP
