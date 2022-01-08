FROM flow-base

ARG BUILD_ID=?
ARG BUILD_VERSION=?
ARG BASE_IMAGE=?
ARG DEBUG_FLOWC=?
ARG RUNTIME=?
ARG STACK_TRACE=?
LABEL version="$BUILD_VERSION"
LABEL release="flowc $BUILD_VERSION $RUNTIME"
LABEL description="Flow Compiler"

## Build and install flow compiler
COPY --chown=worker:worker flow-compiler-minsrc.tgz /tmp
RUN  tar -xzvf /tmp/flow-compiler-minsrc.tgz && rm -f /tmp/flow-compiler-minsrc.tgz
USER root

RUN cd flow-compiler && \
    make -j $(nproc) "STACK_TRACE=$STACK_TRACE" "RUNTIME_TEMPLATES=" "RUNTIMES=$RUNTIME" "DBG=$DEBUG_FLOWC" "BUILD_ID=$BUILD_ID" "BUILD_VERSION=$BUILD_VERSION" "BUILD_IMAGE=$BUILD_IMAGE" install && \
    ( [ "$DEBUG_FLOWC" == "yes" ] || cd .. && rm -fr flow-compiler )

USER worker

ENTRYPOINT ["flowc"]
