#include <boost/asio.hpp>
#include <iostream>

int main()
{
    boost::asio::io_context io_context;
    std::cout << "Boost Asio works!" << std::endl;
    return 0;
}