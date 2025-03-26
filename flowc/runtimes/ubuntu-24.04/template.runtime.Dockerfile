FROM ubuntu:24.04 AS flow-runtime

SHELL ["/bin/bash", "-c"]

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install lsb-release \
    curl jq bc ssh vim unzip binutils \
    && apt-get clean

ENV LANG=C.UTF-8

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
