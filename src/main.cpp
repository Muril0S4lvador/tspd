#include <clocale>
#ifdef _WIN32
#include <windows.h>
#endif
#include "experiment_runner.hpp"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "pt_BR.UTF-8");
    return tspd::experiments::run(argc, argv);
}
