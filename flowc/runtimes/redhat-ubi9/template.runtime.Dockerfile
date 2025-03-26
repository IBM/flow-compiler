FROM registry.access.redhat.com/ubi9-minimal AS flow-runtime

RUN microdnf -y update && microdnf -y install shadow-utils tar gzip binutils \
    && microdnf -y upgrade && microdnf clean all

RUN adduser --uid 1001 worker
RUN chown -R worker  /home/worker
USER worker

ENV LANG=C.UTF-8
