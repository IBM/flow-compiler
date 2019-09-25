FROM flow-runtime AS flow-base

USER root

RUN apk add --no-cache gcc musl-dev g++ grpc make graphviz pkgconf grpc-dev libexecinfo-dev 

USER worker
