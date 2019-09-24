PYTHON?=python3

## Change this to the actual docker repository 
PUSH_REPO?=example.docker.repository.com


NODES_IMAGE=three-node-example-nodes:latest
IMAGE=three-node:1

GRPC_GENERATED=three_node_pb2_grpc.py three_node_pb2.py

$(GRPC_GENERATED): three-node.proto
	$(PYTHON) -m grpc_tools.protoc -I . --python_out=. --grpc_python_out=. $^

nodes-image: $(GRPC_GENERATED) nodes.py nodes.Dockerfile
	cat ubuntu-grpc-py3.Dockerfile nodes.Dockerfile | docker build -t $(NODES_IMAGE) -f - .

nodes: $(GRPC_GENERATED) nodes.py
	@echo "to run: python3 nodes.py"

outd/three-node-client: three-node.flow three-node.proto
	mkdir -p outd
	flowc -o outd $< -c

outd/three-node-server: three-node.flow three-node.proto
	mkdir -p outd
	flowc -o outd $< -s

image: three-node.flow three-node.proto
	mkdir -p outd
	flowc -o outd $< -kdic

drivers: three-node.flow three-node.proto
	mkdir -p outd
	flowc -o outd $< -kd

server: outd/three-node-server

client: outd/three-node-client

push: 
	docker tag $(NODES_IMAGE) $(PUSH_REPO)/$(NODES_IMAGE)
	docker push $(PUSH_REPO)/$(NODES_IMAGE)
	docker tag $(IMAGE) $(PUSH_REPO)/$(IMAGE)
	docker push $(PUSH_REPO)/$(IMAGE)

