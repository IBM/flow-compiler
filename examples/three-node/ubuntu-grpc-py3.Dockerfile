FROM ubuntu:18.04 AS ubuntu-grpc-py3

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install locales lsb-release \
    jq bc ssh vim curl \
    python3-pip \
    && apt-get clean

RUN locale-gen en_US.UTF-8 && /usr/sbin/update-locale LANG=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

## Install grpc for python3
RUN python3 -m pip install --upgrade pip && python3 -m pip install protobuf grpcio grpcio-tools && python3 -m pip install boto3 statsd

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
