FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.12
ARG GRPC_VERSION=1.28.1
ARG CARES_VERSION=1.16.0
ARG GFLAGS_VERSION=v2.2.2

user root

COPY runtimes/redhat-ubi7/centos7.gpg runtimes/redhat-ubi7/centos7.repo /etc/yum.repos.d/
RUN rpm --import /etc/yum.repos.d/centos7.gpg

RUN yum -y install vim curl jq bc ssh unzip \
 git make autoconf automake pkgconfig libtool libtool-ltdl gdb \
 openssl-devel redhat-lsb-core libcurl-devel libxml2-devel libicu-devel uuid-devel \
 gcc-c++ file graphviz devtoolset-7 \
 && yum clean all -y

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.14.1/cmake-3.14.1-Linux-x86_64.sh -o cmake.sh \
  && chmod 755 cmake.sh && ./cmake.sh --skip-license --prefix=/usr/local && rm -f cmake.sh

RUN cd /tmp && git clone -b ${GFLAGS_VERSION} https://github.com/gflags/gflags.git && \
    cd gflags && mkdir build && cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release -D GFLAGS_BUILD_SHARED_LIBS=ON -D GFLAGS_BUILD_STATIC_LIBS=OFF -D GFLAGS_BUILD_TESTING=ON .. && \
    make -j$(nproc) && make install && cd /tmp && rm -fr gflags

## Build and install grpc for C++
## Find the version with curl -L https://grpc.io/release

RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    make -j$(nproc) HAS_SYSTEM_PROTOBUF=false && make -j$(nproc) grpc_cli && make install && \
    find . -name grpc_cli | while read F; do cp $F /usr/local/bin; done && \
    cd /tmp/grpc/third_party/protobuf && make install \
    && cd /tmp && rm -fr grpc 

ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib64/pkgconfig
RUN ldconfig /usr/local/lib

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
ADD --chown=worker:worker https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz ./
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz 
RUN cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1
ENV CIVETWEB_INCS="-I/home/worker/civetweb-${CIVETWEB_VERSION}/include"
ENV CIVETWEB_LIBS="/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a -ldl"
## UUID library 
ENV FLOWC_UUID=OSSP
ENV FLOWC_CXX_STD=c++11
## Build c-ares (https://c-ares.haxx.se)
USER root
ADD https://c-ares.haxx.se/download/c-ares-${CARES_VERSION}.tar.gz ./
RUN tar -xzvf c-ares-${CARES_VERSION}.tar.gz && rm -f c-ares-${CARES_VERSION}.tar.gz && \
    cd c-ares-${CARES_VERSION} && ./configure --enable-shared=no && make && make install && \
    cd .. && rm -fr c-ares-${CARES_VERSION}
USER worker
