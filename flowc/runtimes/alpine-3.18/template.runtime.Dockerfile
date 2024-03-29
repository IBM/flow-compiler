FROM alpine:3.18 AS flow-runtime

RUN apk add --no-cache curl vim gzip bzip2 tar bash binutils

RUN adduser -u 1001 -D worker
RUN chown -R worker /home/worker
USER worker

