FROM registry.access.redhat.com/ubi7 AS flow-runtime

RUN ln -s /usr/lib64/libcrypto.so.10 /usr/lib64/libcrypto.so && ln -s /usr/lib64/libssl.so.10 /usr/lib64/libssl.so

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
