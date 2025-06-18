

### Webserv

This project is about writing our own HTTP/1.1-compliant web server in C++, with non-blocking I/O and support for various web server features. It is part of the 42 School curriculum and follows strict coding standards.

## Features

- 


## Run Locally

Clone the project

```bash
git clone https://github.com/v1dusss/webserv.git
```

Go to the project directory

```bash
cd webserv
```

Start the server

```bash
make docker
./webserv config.yaml
```

## Configuration

The server is configured using a custom nginx-style configuration file.

# Simple Example

```config.yml
 http{
    client_max_header_count 100;
    client_max_header_size 1MB;

    server {
    listen                 8080;
    server_name            localhost;
    client_max_body_size   200MB;
    client_max_header_size 10KB;
    
    location /{
    return 200 "Hello, World!";
    }
  }
  }
```



## Authors

- [Emil Ebert](https://github.com/Peu77)
- [Vidus Sivanathan](https://github.com/v1dusss)
- [Daniel Ilin)(https://github.com/ilindaniel)
