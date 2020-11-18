FROM alpine:3.12 AS flow-runtime

RUN apk add --no-cache curl vim gzip bzip2 tar bash binutils
RUN ln -s /usr/lib/libssl.so.1.1  /usr/lib/libssl.so && ln -s /usr/lib/libcrypto.so.1.1  /usr/lib/libcrypto.so

RUN adduser -u 1001 -D worker
RUN chown -R worker /home/worker
USER worker

