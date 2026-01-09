#include <fstream>

void create_dummy_file(const std::string &name) {
    std::ofstream f(name);
    f << "int main() {}";
    f.close();
}
