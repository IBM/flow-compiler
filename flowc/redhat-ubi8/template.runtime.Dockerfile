FROM registry.access.redhat.com/ubi8 AS flow-runtime

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker


