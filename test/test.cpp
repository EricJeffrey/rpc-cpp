#if !defined(TEST_CPP)
#define TEST_CPP

#define CATCH_CONFIG_MAIN // main is defined

#include "catch.hpp"
#include "rpc-client.h"

// int hello(int x) {
//     if (x <= 5)
//         return 1;
//     else if (x <= 100)
//         return 2;
//     else
//         return 3;
// }

// TEST_CASE("TEST hello", "[hello]") {
//     REQUIRE(hello(-3) == 1);
//     REQUIRE(hello(4) == 1);
//     REQUIRE(hello(5) == 1);
//     REQUIRE(hello(6) == 2);
//     REQUIRE(hello(99) == 2);
//     REQUIRE(hello(101) == 3);
// }

TEST_CASE("TEST ClientRequest-toRequestString") {
    using jeff_rpc::ClientRequest;
    ClientRequest req(2, "hello", "{\"para\": 2333}"_json);
    REQUIRE(strcmp(req.toRequestString().c_str(), "\x02\x00\x00\x00\x23\x00\x00\x00{\"para\":666},\"name\":\"halo\"}") == 0);
}

#endif // TEST_CPP
