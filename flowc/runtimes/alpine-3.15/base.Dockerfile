FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.15

USER root

RUN apk add --no-cache gcc musl-dev g++ grpc grpc-dev protobuf protobuf-dev protoc make graphviz pkgconf libexecinfo-dev util-linux-dev c-ares c-ares-dev gdb grpc-cli re2 
RUN ln -s /usr/lib/libre2.so.9 /usr/lib/libre2.so

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
ADD --chown=worker:worker https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz ./
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 WITH_X_DOM_SOCKET=1
ENV CIVETWEB_INCS=-I/home/worker/civetweb-${CIVETWEB_VERSION}/include
ENV CIVETWEB_LIBS=/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a

