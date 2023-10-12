FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.16
ARG GRPC_VERSION=latest

USER root

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install \
    git libtool-bin build-essential autoconf pkg-config libssl-dev libre2-dev graphviz uuid-dev gdb \
    libgflags-dev libgtest-dev zlib1g-dev cmake && apt-get clean

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
RUN curl -L https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz -o v${CIVETWEB_VERSION}.tar.gz
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_ZLIB=1 WITH_IPV6=1 WITH_WEBSOCKET=1 
ENV CIVETWEB_INCS=-I/home/worker/civetweb-${CIVETWEB_VERSION}/include
ENV CIVETWEB_LIBS="/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a -ldl -lz"

USER root

## Build and install grpc for C++
RUN cd /tmp && if [ "$GRPC_VERSION" == "latest" ]; then \
        git clone -b  $(git ls-remote --tags --refs https://github.com/grpc/grpc.git | cut -d / -f3  | grep -E 'v[0-9]+\.[0-9]+\.[0-9]+$' | sed "h;p;x" | paste -d ' ' - - | sed -e 's/v//' -e s'/\.//' | sort -n | tail -1 | cut -d ' ' -f2) https://github.com/grpc/grpc; \
    else \
        git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc; \
    fi \
    && cd /tmp/grpc && git submodule update --init \
    && mkdir -p cmake/build && cd cmake/build \
    && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DABSL_PROPAGATE_CXX_STD=ON \
    -DgRPC_ABSL_PROVIDER=module     \
    -DgRPC_SSL_PROVIDER=package \
    -DgRPC_CARES_PROVIDER=module    \
    -DgRPC_PROTOBUF_PROVIDER=module \
    -DgRPC_RE2_PROVIDER=package     \
    -DgRPC_ZLIB_PROVIDER=package     \
    ../.. && make -j$(nproc) install \
        && cp /tmp/grpc/third_party/protobuf/src/google/protobuf/compiler/cpp/cpp_generator.h /usr/local/include/google/protobuf/compiler/cpp/ \
    && cmake -DgRPC_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release ../.. && make -j$(nproc) grpc_cli && strip grpc_cli && cp grpc_cli /usr/local/bin \
    && cd /tmp && rm -fr grpc

USER worker

