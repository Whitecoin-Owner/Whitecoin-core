FROM ubuntu:16.04

RUN apt-get update -y
RUN apt-get install -y vim gcc autoconf cmake make automake libtool wget curl git
RUN apt-get install -y libboost-all-dev libssl-dev g++ libcurl4-openssl-dev libleveldb-dev libreadline-dev

RUN mkdir -p /code_deps
WORKDIR /code_deps

# install openssl
RUN wget https://codeload.github.com/openssl/openssl/tar.gz/OpenSSL_1_0_2g
RUN mv OpenSSL_1_0_2g OpenSSL_1_0_2g.tar.gz
RUN tar -xzvf OpenSSL_1_0_2g.tar.gz
WORKDIR /code_deps/openssl-OpenSSL_1_0_2g
RUN ./config
RUN make depend
RUN make
RUN make install


# install fc
WORKDIR /code_deps
RUN git clone https://github.com/BlockLink/fc.git
WORKDIR /code_deps/fc
RUN git submodule update --init --recursive
# RUN cmake -DCMAKE_BUILD_TYPE=Release .
RUN cmake .
RUN make
RUN make install

# install secp256k1
WORKDIR /code_deps/fc/vendor/secp256k1-zkp
RUN ./autogen.sh
RUN ./configure
RUN make
RUN make install

# install golang environment for test
WORKDIR /usr/local
RUN wget https://dl.google.com/go/go1.12.1.linux-amd64.tar.gz
RUN tar -C /usr/local -xzf go1.12.1.linux-amd64.tar.gz

ENV GOROOT /usr/local/go
ENV GOPATH /gohome
ENV PATH $PATH:/usr/local/go/bin:/gohome/bin

WORKDIR /code


