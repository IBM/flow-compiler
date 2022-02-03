FROM py3-grpc
COPY xenv-run.sh nodes.py three_node_pb2_grpc.py three_node_pb2.py ./
CMD ["./xenv-run.sh", "nodes.py"]
