#include <iostream>
#include <memory>

#include "Listener.hpp"

int main()
{
    try
    {
        auto listener = std::make_unique<Listener>(44100, 512, 2);
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << error.what() << '\n';
    }
}
