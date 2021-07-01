FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.14
ARG GRPC_VERSION=1.38.0
ARG CARES_VERSION=1.16.1

user root

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install \
    git libtool-bin build-essential autoconf pkg-config libssl-dev libssl1.1 graphviz uuid-dev gdb \
    libgflags-dev libgtest-dev zlib1g-dev cmake && apt-get clean

## Build and install c-ares (https://c-ares.haxx.se) -- not neeed if grpc version is after 1.30
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

## Build and install grpc for C++
RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init \
    && mkdir -p cmake/build && pushd cmake/build \
    && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ../.. && make -j$(nproc) && make install \
    && cmake -DgRPC_BUILD_TESTS=ON ../.. && make -j$(nproc) grpc_cli && strip grpc_cli && cp grpc_cli /usr/local/bin \
    && popd \
    && mkdir -p third_party/abseil-cpp/cmake/build && cd third_party/abseil-cpp/cmake/build \
    && cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make -j$(nproc) && make install \
    && \
     cp -r /tmp/grpc/third_party/abseil-cpp/absl /usr/local/include/ && \
     cd /usr/local/lib/pkgconfig && ls absl_absl_*.pc | while read F; do ln -s $F ${F#absl_*}; done && \
     cd /usr/local/lib/ && ls libabsl_*.a | while read F; do ln -s $F libabsl_${F#lib*}; done \
    && \
    cd /tmp && rm -fr grpc

# version before 1.30
#RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
#    make -j$(nproc) HAS_SYSTEM_PROTOBUF=false && make install && cd /tmp/grpc/third_party/protobuf && make install && cd /tmp && rm -fr grpc 

USER worker

