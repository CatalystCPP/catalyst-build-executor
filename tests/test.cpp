#include "tests/test_suite.hpp"

#include <cassert>

int main(int argc, char **argv) {
    return !(integration_test() && opaque_deps_test());
}
