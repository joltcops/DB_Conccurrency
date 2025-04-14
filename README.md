# DB_Conccurrency

## To run this code on Windows

   **Install the latest version of LLVM/Clang** in your system from the official website  "https://github.com/llvm/llvm-project/releases/tag/llvmorg-17.0.6"

    Reboot your system and check the version using command `clang++ --version`

    Install C++ STL headers and visual studio build tools from the website "https://visualstudio.microsoft.com/"

    Open the “x64 Native Tools Command Prompt for VS 20XX”

    Navigate to your project directory

    To compile the code, run
    `clang++ -std=c++23 -o lock lockmanager.cpp test00.cpp`

    Then type `rename lock lock.exe`

    To run the executable, type `.\lock.exe`

## To run the code on Linux:

    Install g++14 using the following commands
    `sudo add-apt-repository ppa:ubuntu-toolchain-r/test`
    `sudo apt update`
    `sudo apt install g++-14`

    Navigate to the appropriate directory

    To compile the code, run
    `g++-14 -std=c++23 -o lock lockmanager.cpp test00.cpp`

    To run the executable, type `.\lock`