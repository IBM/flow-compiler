FROM registry.access.redhat.com/ubi9-minimal AS flow-runtime

COPY runtimes/redhat-ubi9/centos-gpg-keys-9.0-24.el9.noarch.rpm /etc
RUN rpm -i /etc/centos-gpg-keys-9.0-24.el9.noarch.rpm
COPY runtimes/redhat-ubi9/centos9.repo /etc/yum.repos.d/

RUN microdnf -y update && microdnf -y install tar jq gzip unzip psmisc binutils \
    && microdnf -y upgrade && microdnf clean all

RUN adduser --uid 1001 worker
RUN chown -R worker  /home/worker
USER worker


