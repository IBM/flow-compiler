FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.14

USER root

RUN apk add --no-cache gcc musl-dev g++ grpc grpc-dev protobuf protobuf-dev protoc make graphviz pkgconf grpc-dev libexecinfo-dev util-linux-dev c-ares c-ares-dev gdb

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
ADD --chown=worker:worker https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz ./
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 
ENV CIVETWEB_INCS=-I/home/worker/civetweb-${CIVETWEB_VERSION}/include
ENV CIVETWEB_LIBS=/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a

