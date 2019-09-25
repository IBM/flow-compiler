FROM flow-runtime AS flow-base

user root

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install \
    git libtool-bin build-essential autoconf pkg-config libssl1.0.0 \
    libgflags-dev libgtest-dev && apt-get clean

## Build and install grpc for C++
## Find the version with curl -L https://grpc.io/release

RUN cd /tmp && git clone -b v1.22.0 https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    make -j4 HAS_SYSTEM_PROTOBUF=false && make install && cd /tmp/grpc/third_party/protobuf && make install && cd /tmp && rm -fr grpc 

USER worker
