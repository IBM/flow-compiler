FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.13
ARG GRPC_VERSION=1.33.2
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
## Find the version with curl -L https://grpc.io/release
RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    mkdir -p cmake/build && cd cmake/build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ../.. && make -j$(nproc) && make install \
    && find /tmp/grpc -name lib\*.a | while read A; do cp -n $A /usr/local/lib; done \
    && find /tmp/grpc -name \*_plugin | while read A; do cp -n $A /usr/local/bin; done \
    && cd /tmp && rm -fr grpc

ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig:/usr/lib64/pkgconfig:/usr/share/pkgconfig
RUN ldconfig /usr/local/lib && ldconfig /usr/local/lib64 && ldconfig /usr/local/share && ldconfig /usr/lib64 && ldconfig /usr/share


USER worker
## UUID library 
ENV FLOWC_UUID=OSSP
