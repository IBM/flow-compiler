FROM flowc AS base

USER worker
RUN mkdir -p /home/worker/{{NAME}} && mkdir -p /tmp/{{NAME}}
COPY --chown=worker:worker {{MAIN_FILE}} {P:PROTO_FILE{{{PROTO_FILE}} }P} /tmp/{{NAME}}/
RUN cd /tmp/{{NAME}} && flowc --client --server {{MAIN_FILE}} --name {{NAME}} && make -j2 -f {{NAME}}.mak deploy && cd /tmp && rm -fr {{NAME}}

WORKDIR /usr/local/lib
RUN tar -cf /home/worker/so.tar lib*.so lib*.so.* 
WORKDIR /home/worker
RUN tar -cf /home/worker/bin.tar {{NAME}}/*

FROM flow-runtime

USER root
COPY --chown=worker:worker --from=base /home/worker/*.tar /home/worker/
COPY --from=base /usr/local/bin/cosget.sh /usr/local/bin/artiget.sh /usr/local/bin/
WORKDIR /usr/local/lib
RUN tar -xvf /home/worker/so.tar && rm /home/worker/so.tar && ldconfig
WORKDIR /home/worker
RUN tar -xvf /home/worker/bin.tar && rm /home/worker/bin.tar && chown -R worker:worker {{NAME}}
USER worker
