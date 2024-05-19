#include <iostream>
#include <stdio.h>
#include "OverPatch.h"

int main(int argc, char** argv)
{
    std::cout << "Launching with " << argv[1] << " as overwatch executable.";
    if (argv[1] == NULL) {
        abort();
    }
    const char* variables = " --BNetServer=bnet-emu.fish:1119";

    std::string whatever = (std::string)argv[1] + (std::string)variables;
    std::cout << whatever;
    OverPatch::LaunchAndPatch(whatever);
}