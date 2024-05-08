FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.16
ARG GRPC_VERSION=latest

user root

COPY runtimes/redhat-ubi9/centos-gpg-keys-9.0-24.el9.noarch.rpm /etc
RUN rpm -i /etc/centos-gpg-keys-9.0-24.el9.noarch.rpm
COPY runtimes/redhat-ubi9/centos9.repo /etc/yum.repos.d/

RUN microdnf -y install --nobest bind-utils \
    jq unzip psmisc \
    iputils net-tools procps-ng glibc-langpack-en.x86_64 \
    git make autoconf automake pkgconfig libtool cmake zlib gdb \
    vim wget findutils gcc-c++ file \
    iproute uuid-devel libcurl-devel zlib-devel \
    && microdnf clean all

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
    -DgRPC_SSL_PROVIDER=module \
    -DgRPC_CARES_PROVIDER=module    \
    -DgRPC_PROTOBUF_PROVIDER=module \
    -DgRPC_RE2_PROVIDER=module     \
    -DgRPC_ZLIB_PROVIDER=package     \
    ../.. && make -j$(nproc) install \
        && ( [ -f /usr/local/lib/pkgconfig/re2.pc ] || ( pushd /tmp/grpc/third_party/re2 && make common-install prefix=/usr/local && popd ) ) \
        && cp /tmp/grpc/third_party/protobuf/src/google/protobuf/compiler/cpp/cpp_generator.h /usr/local/include/google/protobuf/compiler/cpp/ \
    && cmake -DgRPC_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release ../.. && make -j$(nproc) grpc_cli && strip grpc_cli && cp grpc_cli /usr/local/bin \
    && cd /tmp && rm -fr grpc

USER worker

## UUID library 
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig:/usr/lib64/pkgconfig:/usr/share/pkgconfig
ENV FLOWC_UUID=OSSP
