FROM registry.access.redhat.com/ubi8 AS flow-runtime

RUN ln -s /usr/lib64/libssl.so.1.1 /usr/lib64/libssl.so && ln -s /usr/lib64/libcrypto.so.1.1 /usr/lib64/libcrypto.so

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker


