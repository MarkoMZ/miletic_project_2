# miletic_project_2 - Basic HTTP 1.1 Client

### The goal of this project is to implement an simple HTTP 1.1 Client that is able to download and upload files to a server.

### HTTP-Basic Authentication should also be implemented.

### This programm should be used via CLI.

### But out of own interest i want to later add an GUI to get a better user expirience.

## Usage

### Flags
#### -m, --method          : Sets HTTP-Method (GET, PUT, POST, DELETE)
#### -f, --file            : Sets the file which should be send.
#### -u, --uri, -l, --link : Sets the request URI
#### -a, --auth            : Sets the Authentication Data ([Username:Password])
#### -j, --json            : Sets the Authentication Data in JSON format (JSON-File).


### Example
#### ./http_client -u https://github.com/MarkoMZ/miletic_project_2 -m GET


