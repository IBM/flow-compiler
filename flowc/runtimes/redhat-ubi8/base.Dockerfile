FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.13
ARG GRPC_VERSION=1.36.3
ARG CARES_VERSION=1.16.1

user root

COPY runtimes/redhat-ubi8/RPM-GPG-KEY-CentOS-Official /etc/pki/rpm-gpg/
RUN rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-Official
RUN echo -e "[centos8-powertools]\nname=centos8-powertools\nbaseurl=http://mirror.centos.org/centos-8/8/PowerTools/x86_64/os/\ngpgcheck=1\nenabled=1" > /etc/yum.repos.d/centos8.repo && \
    echo -e "[centos8-appstream]\nname=centos8-appstream\nbaseurl=http://mirror.centos.org/centos-8/8/AppStream/x86_64/os/\ngpgcheck=1\nenabled=1" >> /etc/yum.repos.d/centos8.repo && \
    echo -e "[centos8-baseos]\nname=centos8-appstream\nbaseurl=http://mirror.centos.org/centos/8/BaseOS/x86_64/os/\ngpgcheck=1\nenabled=1" >> /etc/yum.repos.d/centos8.repo

RUN yum -y install vim curl jq bc tar unzip binutils \
 git make autoconf automake pkgconfig libtool libtool-ltdl gdb cmake \
 openssl-devel redhat-lsb-core libcurl-devel libxml2-devel libicu-devel uuid-devel \
 gcc-c++ file graphviz \
 && yum clean all -y


USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
RUN curl -L https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz -o v${CIVETWEB_VERSION}.tar.gz
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz && cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 
ENV CIVETWEB_INCS=-I/home/worker/civetweb-${CIVETWEB_VERSION}/include
ENV CIVETWEB_LIBS="/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a -ldl"

USER root

## Build and install grpc for C++
RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    mkdir -p cmake/build && cd cmake/build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ../.. && make -j$(nproc) && make install \
    && find /tmp/grpc -name lib\*.a | while read A; do cp -n $A /usr/local/lib; done \
    && find /tmp/grpc -name \*_plugin | while read A; do cp -n $A /usr/local/bin; done \
    && cd /tmp && rm -fr grpc

# version before 1.30
#RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
#    make -j$(nproc) HAS_SYSTEM_PROTOBUF=false && make install && cd /tmp/grpc/third_party/protobuf && make install  \
#    && find /tmp/grpc -name lib\*.a | while read A; do cp -n $A /usr/local/lib; done \
#    && find /tmp/grpc -name \*_plugin | while read A; do cp -n $A /usr/local/bin; done \
#    && ldconfig /usr/local/lib \
#    && cd /tmp && rm -fr grpc

#RUN curl -L https://c-ares.haxx.se/download/c-ares-${CARES_VERSION}.tar.gz -o c-ares-${CARES_VERSION}.tar.gz
#RUN tar -xzvf c-ares-${CARES_VERSION}.tar.gz && rm -f c-ares-${CARES_VERSION}.tar.gz && \
#   cd c-ares-${CARES_VERSION} && ./configure --enable-shared=no && make && make install && \
#    cd .. && rm -fr c-ares-${CARES_VERSION}

USER worker

## UUID library 
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig:/usr/lib64/pkgconfig:/usr/share/pkgconfig
ENV FLOWC_UUID=OSSP
