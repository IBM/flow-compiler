.PHONY: info image clean all image-info-Darwin image-info-Linux client server deploy

########################################################################
# {{NAME}}.mak 
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
-include makefile.local

PUSH_REPO?={{PUSH_REPO:}}
IMAGE_TAG?={{IMAGE_TAG}}
DEFAULT_IMAGE:={{NAME}}:$(IMAGE_TAG)
IMAGE?={{IMAGE:$(DEFAULT_IMAGE)}}	
IMAGE_REPO:=$(shell echo "$(IMAGE)" | sed 's/:.*$$//')
IMAGE_TAG:=$(shell echo "$(IMAGE)" | sed 's/^.*://')
DOCKERFILE?={{NAME}}.Dockerfile
IMAGE_PROXY?={{NAME}}-image-info.json

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
ifeq ($(GRPC_STATIC), yes)
GRPC_LIBS?=$(shell pkg-config --static --libs grpc++ protobuf)
else 
GRPC_LIBS?=$(shell pkg-config --libs grpc++ protobuf)
endif

CIVETWEB_INCS?=$(shell pkg-config --cflags civetweb) 
CIVETWEB_LIBS?=$(shell pkg-config --libs civetweb)

SERVER_LFLAGS+= $(GRPC_LIBS) -luuid
SERVER_CFLAGS+= $(GRPC_INCS)

ifeq ($(NO_REST), 1)
SERVER_CFLAGS+= -DNO_REST
else 	
SERVER_LFLAGS+= $(CIVETWEB_LIBS)
SERVER_CFLAGS+= $(CIVETWEB_INCS)
endif

ifeq ($(PUSH_REPO), )
DOCKER:=docker-info
else 
DOCKER:=docker-push
PUSH_IMAGE:=$(PUSH_REPO:/=)/$(IMAGE_REPO):$(IMAGE_TAG)
PUSH_IMAGE_LATEST:=$(PUSH_REPO:/=)/$(IMAGE_REPO):latest
endif

THIS_FILE:=$(lastword $(MAKEFILE_LIST))

info:
	@echo "Target \"image\" will build a docker image, with the tag \"$(IMAGE)\""
	@echo "Override IMAGE_TAG or IMAGE to change the image tag from the default"
	@echo "Set PUSH_REPO to the remote repository where the freshly built image needs to be pushed"
	@echo ""
	@echo "make -f $(THIS_FILE) IMAGE={{NAME}}:0.5 image"
	@echo ""
	@echo "Note that the gRPC libraries are searched for with pkg-config."
	@echo "If pkg-config is not available, set GRPC_LIBS and/or GRPC_INCS with the desired link and/or cflags options"
	@echo ""
	@echo "Targets \"server\" and \"client\" will build the server and the client binaries respectively"
	@echo "Target \"all\" will build both the server and client binaries"
	@echo ""
	@echo "make -f $(THIS_FILE) client" 

docker-push: docker-info
	@-docker rmi $(PUSH_IMAGE) 2>&1 > /dev/null
	@docker tag $(IMAGE) $(PUSH_IMAGE) 
	docker push $(PUSH_IMAGE)

docker-info:
	@docker images $(IMAGE_REPO)

$(IMAGE_PROXY): docs/{{MAIN_FILE}} {P:PROTO_FILE{docs/{{PROTO_FILE}} }P} $(DOCKERFILE)
	-docker rmi -f $(IMAGE) 2>&1 > /dev/null
	docker build --force-rm -t $(IMAGE) -f $(DOCKERFILE) .
	-docker rmi $(IMAGE_REPO):latest 2>&1 > /dev/null
	docker image inspect $(IMAGE) > $@

image-info-Darwin:
	@rm -f $(IMAGE_PROXY)
	@-docker image inspect $(IMAGE) 2>&1 > /dev/null && docker image inspect $(IMAGE) > $(IMAGE_PROXY) && touch -t $(shell /bin/date -j -f '%Y%m%d%H%M%S%Z' `docker image inspect --format '{{.Created}}' $(IMAGE) 2> /dev/null | sed -E 's/([:T-]|[.][0-9]+)//g' | sed 's/Z/GMT/'` +'%Y%m%d%H%M.%S' 2>/dev/null) $(IMAGE_PROXY)

image-info-Linux:
	@rm -f $(IMAGE_PROXY)
	@-docker image inspect $(IMAGE) 2>&1 > /dev/null && docker image inspect $(IMAGE) > $(IMAGE_PROXY) && touch --date=`docker image inspect --format '{{.Created}}' $(IMAGE) 2>/dev/null` $(IMAGE_PROXY)

image: image-info-$(shell uname -s)
	@$(MAKE) -s -f $(THIS_FILE) IMAGE=$(IMAGE) DOCKERFILE=$(DOCKERFILE) IMAGE_PROXY=$(IMAGE_PROXY) PUSH_REPO=$(PUSH_REPO) $(IMAGE_PROXY)
	@$(MAKE) -s -f $(THIS_FILE) IMAGE=$(IMAGE) DOCKERFILE=$(DOCKERFILE) IMAGE_PROXY=$(IMAGE_PROXY) PUSH_REPO=$(PUSH_REPO) $(DOCKER)

PB_GENERATED_CC:={P:PB_GENERATED_C{{{PB_GENERATED_C}} }P} {P:GRPC_GENERATED_C{{{GRPC_GENERATED_C}} }P}
PB_GENERATED_H:={P:PB_GENERATED_H{{{PB_GENERATED_H}} }P} {P:GRPC_GENERATED_H{{{GRPC_GENERATED_H}} }P}

{{NAME}}-server: {{NAME}}-server.C $(PB_GENERATED_CC) $(PB_GENERATED_H) 
	${CXX} -std=c++11 $(SERVER_CFLAGS) -O3 -o $@  $< $(PB_GENERATED_CC) $(SERVER_LFLAGS)

{{NAME}}-client: {{NAME}}-client.C $(PB_GENERATED_CC) $(PB_GENERATED_H) 
	${CXX} -std=c++11 $(GRPC_INCS) -O3 -o $@  $<  $(PB_GENERATED_CC) $(GRPC_LIBS)

clean:
	rm -f $(IMAGE_PROXY) {{NAME}}-server {{NAME}}-client 

all: {{NAME}}-server {{NAME}}-client 

client: {{NAME}}-client

server: {{NAME}}-server 

deploy: {{NAME}}-server {{NAME}}-client
	strip $^
	mkdir -p ~/{{NAME}}/docs 
	mkdir -p ~/{{NAME}}/www
	cp $^ ~/{{NAME}}
	cp $(wildcard docs/*.proto) $(wildcard docs/*.svg) $(wildcard docs/*.flow) ~/{{NAME}}/docs
	cp $(wildcard www/*.html) $(wildcard www/*.css) $(wildcard www/*.js) ~/{{NAME}}/www
