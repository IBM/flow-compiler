FROM debian:bullseye-slim AS py3-grpc

SHELL ["/bin/bash", "-c"]

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install lsb-release \
    curl jq bc ssh vim unzip binutils python3-venv python3-pip \
    && apt-get clean

ENV LANG C.UTF-8

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
WORKDIR /home/worker

RUN mkdir -p .pyenv && cd .pyenv && python3 -m venv xenv
RUN source .pyenv/xenv/bin/activate && python -m pip install protobuf grpcio grpcio-tools 
COPY xenv-run.sh ./
