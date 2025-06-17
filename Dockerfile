FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    php \
    php-cgi \
    php-mysql \
    bash \
    valgrind

WORKDIR /app
COPY . /app

RUN make
RUN chmod +x webserv


CMD ["./webserv", "config.yaml"]