FROM ubuntu:20.04 AS flow-runtime

SHELL ["/bin/bash", "-c"]

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install locales lsb-release \
    curl jq bc ssh vim unzip binutils \
    && apt-get clean

RUN locale-gen en_US.UTF-8 && /usr/sbin/update-locale LANG=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
