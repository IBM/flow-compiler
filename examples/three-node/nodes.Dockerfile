FROM py3-grpc
COPY xenv-run.sh nodes.py three-node.proto ./
CMD ["./xenv-run.sh", "nodes.py"]
