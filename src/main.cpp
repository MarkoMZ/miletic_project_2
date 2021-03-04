/*

    author:     MILETIC Marko
    class:      5CHIF
    description:
        The goal of this project is to implement an simple HTTP 1.1 Client that is able to download
        and upload files to a server.

        HTTP-Basic Authentication should also be implemented.

        This programm should be used via CLI.

        But out of own interest i want to later add an GUI to get a better user expirience.

*/

#include <iostream>
#include <map>
#include <thread>

#include "../include/http_client.hpp"

using namespace std;

//int main(int argc, char* argv[]) {
int main() {
    thread t{[]{ cout << "Hello"; }};
    t.join();
    cout << " world!" << endl;
    return 0;
}
