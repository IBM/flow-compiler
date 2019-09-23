FROM flow-base

ARG BUILD_ID=?
ARG BUILD_VERSION=?

USER root

## Build and install flow compiler
COPY flow-compiler-minsrc.tgz /tmp
COPY cosget.sh artiget.sh /usr/local/bin/

RUN cd /tmp && tar -xzvf flow-compiler-minsrc.tgz && cd /tmp/flow-compiler && make "BUILD_ID=$BUILD_ID" "BUILD_VERSION=$BUILD_VERSION" install && cd /tmp && rm -fr flow-compiler flow-compiler-minsrc.tgz

USER worker
