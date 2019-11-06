FROM flowc AS base

USER worker
RUN mkdir -p /home/worker/{{NAME}}/docs && mkdir -p /tmp/{{NAME}}/docs
COPY --chown=worker:worker docs/{{MAIN_FILE}} {P:PROTO_FILE{docs/{{PROTO_FILE}} }P} /tmp/{{NAME}}/
RUN cd /tmp/{{NAME}} && flowc --client --server {{MAIN_FILE}} --name {{NAME}} && make -j2 -f {{NAME}}.mak deploy && cd /tmp && rm -fr {{NAME}}

WORKDIR /home/worker
RUN tar -cf /home/worker/bin.tar {{NAME}}/*
RUN mkdir -p /home/worker/lib && chown -R worker:worker /home/worker/lib
RUN ldd /home/worker/{{NAME}}/{{NAME}}-client | awk '$1 !~ "/.*" && $3 ~ "/usr/.*" {print $3}' >> list.txt
RUN ldd /home/worker/{{NAME}}/{{NAME}}-server | awk '$1 !~ "/.*" && $3 ~ "/usr/.*" {print $3}' >> list.txt
RUN sort -u list.txt | while read F; do cp "$F" /home/worker/lib; done 
RUN tar -cf /home/worker/so.tar lib/*

FROM flow-runtime

USER root
COPY --chown=worker:worker --from=base /home/worker/*.tar /home/worker/
COPY --from=base /usr/local/bin/cosget.sh /usr/local/bin/artiget.sh /usr/local/bin/
WORKDIR /usr/local
RUN tar -xvf /home/worker/so.tar && rm /home/worker/so.tar && ldconfig /usr/local/lib
WORKDIR /home/worker
RUN tar -xvf /home/worker/bin.tar && rm /home/worker/bin.tar && chown -R worker:worker {{NAME}}
RUN mkdir -p {{NAME}}/www
COPY --chown=worker:worker --from=base /home/worker/www/* /home/worker/{{NAME}}/www/
USER worker
