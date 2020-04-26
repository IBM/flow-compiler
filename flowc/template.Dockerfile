FROM flowc:{{FLOWC_VERSION}}.{{BASE_IMAGE}} AS base
ARG DEBUG_IMAGE=no

USER worker
RUN mkdir -p /home/worker/{{NAME}}/docs && mkdir -p /tmp/{{NAME}}/docs && mkdir -p /home/worker/{{NAME}}/www && mkdir  -p /home/worker/{{NAME}}/src
COPY --chown=worker:worker {{NAME}}-htdocs.tar.gz /home/worker/{{NAME}}/
COPY --chown=worker:worker docs/{{MAIN_FILE}} {P:SERVER_XTRA_H{{{SERVER_XTRA_H}} }P} {P:SERVER_XTRA_C{{{SERVER_XTRA_C}} }P} {P:PROTO_FILE{docs/{{PROTO_FILE}} }P} /home/worker/{{NAME}}/src/
WORKDIR /home/worker/{{NAME}}
RUN tar -xzvf {{NAME}}-htdocs.tar.gz && rm -f {{NAME}}-htdocs.tar.gz
WORKDIR /home/worker/{{NAME}}/src
RUN flowc --client --server {{MAIN_FILE}} --name {{NAME}} && make -j2 DBG=${DEBUG_IMAGE} -f {{NAME}}.mak deploy
WORKDIR /home/worker/{{NAME}}
ENV GRPC_POLL_STRATEGY "poll"
ENTRYPOINT []
