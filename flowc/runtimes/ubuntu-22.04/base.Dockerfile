FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.15
ARG GRPC_VERSION=1.49.1

user root

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install \
    git libtool-bin build-essential autoconf pkg-config libssl-dev graphviz uuid-dev gdb \
    libgflags-dev libgtest-dev zlib1g-dev cmake && apt-get clean

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
RUN curl -L https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz -o v${CIVETWEB_VERSION}.tar.gz
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 WITH_X_DOM_SOCKET=1
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
    && cd /tmp && rm -fr grpc

USER worker

