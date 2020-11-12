FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.13
ARG GRPC_VERSION=1.33.2
ARG CARES_VERSION=1.16.1

user root

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install \
    git libtool-bin build-essential autoconf pkg-config libssl-dev libssl1.1 graphviz uuid-dev gdb \
    libgflags-dev libgtest-dev zlib1g-dev cmake && apt-get clean

## Build and install c-ares (https://c-ares.haxx.se)
#RUN curl -L https://c-ares.haxx.se/download/c-ares-${CARES_VERSION}.tar.gz -o c-ares-${CARES_VERSION}.tar.gz
#RUN tar -xzvf c-ares-${CARES_VERSION}.tar.gz && rm -f c-ares-${CARES_VERSION}.tar.gz && \
#    cd c-ares-${CARES_VERSION} && ./configure --enable-shared=no && make && make install && \
#    cd .. && rm -fr c-ares-${CARES_VERSION}

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
RUN curl -L https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz -o v${CIVETWEB_VERSION}.tar.gz
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 
ENV CIVETWEB_INCS=-I/home/worker/civetweb-${CIVETWEB_VERSION}/include
ENV CIVETWEB_LIBS="/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a -ldl"

USER root

RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    mkdir -p cmake/build && cd cmake/build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF ../.. && make -j$(nproc) && make install && cd /tmp && rm -fr grpc

USER worker

