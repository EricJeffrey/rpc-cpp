#include <exception>

#include "logger.h"
#include "lib/json.hpp"
#include <sstream>
#include <set>

using nlohmann::json;
using std::exception;
using std::stringstream;

// todo error handler
// todo exception


int main(int argc, char const *argv[]) {
    try {
    } catch (const std::exception &e) {
        Logger::getInstance()->error({e.what()});
    }

    return 0;
}
