FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    php \
    php-cgi \
    php-mysql \
    bash \
    python3

WORKDIR /app
COPY . /app

RUN make
RUN chmod +x webserv


#CMD ["valgrind", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes", "--verbose", "./webserv", "config.yaml"]
CMD ["./webserv", "config.yaml"]