#include <iostream>
#include <stdio.h>
#include <sstream>
#include "OverPatch.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_executable> [additional_arguments]" << std::endl;
        return 1; // Abort
    }
    std::cout << "Launching with " << argv[1] << " as Overwatch executable." << std::endl;

    std::stringstream command;
    command << argv[1]; // Path to assumed executable

    // Target server
    command << " --BNetServer=bnet-emu.fish:1119";

    // Further command line options
    for (int i = 2; i < argc; i++) {
        command << " " << argv[i];
    }

    std::string commandStr = command.str();
    std::cout << "Launching entirely with: " << commandStr << std::endl;

    if (!OverPatch::LaunchAndPatch(commandStr)) {
        std::cerr << "Failed to launch and patch." << std::endl;
        return 1;
    }
}