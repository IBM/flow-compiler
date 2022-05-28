PYTHON?=python3

## Change this to the actual docker repository 
PUSH_REPO?=example.docker.repository.com


NODES_IMAGE=three-node-example-nodes:latest
IMAGE=three-node:1


nodes-image: nodes.py nodes.Dockerfile
	cat py3.Dockerfile nodes.Dockerfile | docker build -t $(NODES_IMAGE) -f - .

nodes: nodes.py
	@echo "to run: python3 nodes.py"

image: three-node.flow three-node.proto
	flowc -i $< 

driver: three-node.flow three-node.proto
	flowc -o . $< -d

server: outd/three-node-server

client: outd/three-node-client

push: 
	docker tag $(NODES_IMAGE) $(PUSH_REPO)/$(NODES_IMAGE)
	docker push $(PUSH_REPO)/$(NODES_IMAGE)
	docker tag $(IMAGE) $(PUSH_REPO)/$(IMAGE)
	docker push $(PUSH_REPO)/$(IMAGE)

