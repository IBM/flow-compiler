FROM flow-runtime AS flow-base
ARG CIVETWEB_VERSION=1.12
ARG GRPC_VERSION=1.28.1
ARG CARES_VERSION=1.16.0

user root

COPY redhat-ubi8/RPM-GPG-KEY-CentOS-Official /etc/pki/rpm-gpg/
RUN rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-Official
RUN echo -e "[centos8-powertools]\nname=centos8-powertools\nbaseurl=http://mirror.centos.org/centos-8/8/PowerTools/x86_64/os/\ngpgcheck=1\nenabled=1" > /etc/yum.repos.d/centos8.repo && \
    echo -e "[centos8-appstream]\nname=centos8-appstream\nbaseurl=http://mirror.centos.org/centos-8/8/AppStream/x86_64/os/\ngpgcheck=1\nenabled=1" >> /etc/yum.repos.d/centos8.repo && \
    echo -e "[centos8-baseos]\nname=centos8-appstream\nbaseurl=http://mirror.centos.org/centos/8/BaseOS/x86_64/os/\ngpgcheck=1\nenabled=1" >> /etc/yum.repos.d/centos8.repo

RUN yum -y install vim curl jq bc tar unzip binutils \
 git make autoconf automake pkgconfig libtool libtool-ltdl gdb \
 openssl-devel redhat-lsb-core libcurl-devel libxml2-devel libicu-devel uuid-devel \
 gcc-c++ file graphviz \
 && yum clean all -y

## Build and install grpc for C++
## Find the version with curl -L https://grpc.io/release

RUN cd /tmp && git clone -b v${GRPC_VERSION} https://github.com/grpc/grpc && cd grpc && git submodule update --init && \
    make -j$(nproc) HAS_SYSTEM_PROTOBUF=false && make install && cd /tmp/grpc/third_party/protobuf && make install && cd /tmp && rm -fr grpc 

# RUN make -j$(nproc) && make -j$(nproc) grpc_cli && make install && cd third_party/protobuf && make install && ldconfig

# RUN cp /var/local/git/grpc/bins/opt/grpc_cli /usr/local/bin/
#RUN cp /var/local/git/grpc/bins/opt/protobuf/protoc /usr/local/bin/
#RUN strip /usr/local/lib/*.so* && find /usr/local/bin/ -type f ! -name "*.sh" -exec strip {} \;
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib64/pkgconfig
RUN ldconfig /usr/local/lib

USER worker
WORKDIR /home/worker

## Build civetweb (https://github.com/civetweb/civetweb)
ADD --chown=worker:worker https://github.com/civetweb/civetweb/archive/v${CIVETWEB_VERSION}.tar.gz ./
RUN tar -xzvf v${CIVETWEB_VERSION}.tar.gz && rm -f v${CIVETWEB_VERSION}.tar.gz 
#COPY civetweb.c civetweb-${CIVETWEB_VERSION}/src
#RUN cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1 WITH_DEBUG=1
RUN cd civetweb-${CIVETWEB_VERSION} && make lib WITH_IPV6=1
ENV CIVETWEB_INCS="-I/home/worker/civetweb-${CIVETWEB_VERSION}/include"
ENV CIVETWEB_LIBS="/home/worker/civetweb-${CIVETWEB_VERSION}/libcivetweb.a -ldl"
## UUID library 
ENV FLOWC_UUID=OSSP
## Build c-ares (https://c-ares.haxx.se)
USER root
ADD https://c-ares.haxx.se/download/c-ares-${CARES_VERSION}.tar.gz ./
RUN tar -xzvf c-ares-${CARES_VERSION}.tar.gz && rm -f c-ares-${CARES_VERSION}.tar.gz && \
    cd c-ares-${CARES_VERSION} && ./configure --enable-shared=no && make && make install && \
    cd .. && rm -fr c-ares-${CARES_VERSION}
USER worker
