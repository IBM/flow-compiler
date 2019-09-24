FROM ubuntu-grpc-py3
USER worker
WORKDIR /home/worker/three-node-example
COPY nodes.py three_node_pb2_grpc.py three_node_pb2.py ./
CMD ["python3", "nodes.py"]
