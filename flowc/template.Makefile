.PHONY: info image clean all image-info-Darwin image-info-Linux client server deploy
.SILENT: image-info-Darwin image-info-Linux 

########################################################################
# {{NAME}}.mak 
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
-include makefile.local

FLOWC_CXX_STD?=c++17
HOST_OS:=$(shell uname -s)
DBG?={{DEBUG_IMAGE}}
REST?={{REST_API}}
CFLAGS+= -std=$(FLOWC_CXX_STD)

ifeq ($(DBG), yes)
	CFLAGS+= -g -Og
else
	CFLAGS+= -O3
endif

PUSH_REPO?={{PUSH_REPO-}}
IMAGE?={{IMAGE}}
IMAGE_NAME?=$(shell echo $(IMAGE) | sed 's/:.*$$//')
IMAGE_TAG?=$(shell echo $(IMAGE) | sed 's/^.*://')

IMAGE_PROXY?={{NAME}}-image-info.json
HTDOCS_PATH?={{HTDOCS_PATH-}}

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
ifeq ($(HOST_OS), Darwin)
GRPC_LIBS?=$(shell pkg-config --libs grpc++ protobuf) -lgrpc++_reflection -ldl
else
GRPC_LIBS?=$(shell pkg-config --libs grpc++ protobuf) -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl
endif

CIVETWEB_INCS?=$(shell pkg-config --cflags civetweb) 
CIVETWEB_LIBS?=$(shell pkg-config --libs civetweb)

CARES_INCS?=$(shell pkg-config --cflags libcares) 
CARES_LIBS?=$(shell pkg-config --libs libcares)

SERVER_LFLAGS+= $(GRPC_LIBS) $(CARES_LIBS) 
SERVER_CFLAGS+= $(GRPC_INCS) $(CARES_INCS)

ifeq ($(REST), no)
SERVER_CFLAGS+= -DNO_REST
else 	
SERVER_LFLAGS+= $(CIVETWEB_LIBS)
SERVER_CFLAGS+= $(CIVETWEB_INCS)
endif

ifeq ($(FLOWC_UUID), OSSP)
SERVER_LFLAGS+= $(shell pkg-config --libs ossp-uuid)
SERVER_CFLAGS+= -DOSSP_UUID
endif
ifeq ($(FLOWC_UUID), )
SERVER_LFLAGS+= $(shell pkg-config --libs uuid)
endif
ifeq ($(FLOWC_UUID), NONE)
SERVER_CFLAGS+= -DNO_UUID
endif

ifeq ($(PUSH_REPO), )
DOCKER:=docker-info
else 
DOCKER:=docker-push
PUSH_IMAGE:=$(PUSH_REPO:/=)/$(IMAGE_NAME):$(IMAGE_TAG)
endif

ifeq ($(HTDOCS_PATH), )
{{NAME}}-htdocs.tar.gz:
	tar -czf $@ --files-from=/dev/null
else
{{NAME}}-htdocs.tar.gz:
	ln -s "$(HTDOCS_PATH)" app
	tar -czf $@ -h app
endif

THIS_FILE:=$(lastword $(MAKEFILE_LIST))

info:
	@echo "Target \"image\" will build a docker image tagged \"$(IMAGE_NAME):$(IMAGE_TAG)\""
	@echo "Set PUSH_REPO to a remote repository if the freshly built image needs to be pushed"
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

PB_GENERATED_CC:={P:PB_GENERATED_C{{{PB_GENERATED_C}} }P} {P:GRPC_GENERATED_C{{{GRPC_GENERATED_C}} }P}
PB_GENERATED_H:={P:PB_GENERATED_H{{{PB_GENERATED_H}} }P} {P:GRPC_GENERATED_H{{{GRPC_GENERATED_H}} }P}
SERVER_XTRA_H:={P:SERVER_XTRA_H{{{SERVER_XTRA_H}} }P} 
SERVER_XTRA_C:={P:SERVER_XTRA_C{{{SERVER_XTRA_C}} }P} 

docker-push: docker-info
	@-docker rmi $(PUSH_IMAGE) > /dev/null 2>&1
	@docker tag $(IMAGE_NAME):$(IMAGE_TAG) $(PUSH_IMAGE) 
	docker push $(PUSH_IMAGE)

docker-info:
	@docker images $(IMAGE_NAME)

$(IMAGE_PROXY): docs/{{MAIN_FILE}} {P:PROTO_FILE{docs/{{PROTO_FILE}} }P} {{NAME}}.Dockerfile {{NAME}}.slim.Dockerfile $(SERVER_XTRA_H) $(SERVER_XTRA_C) {{NAME}}-htdocs.tar.gz
	@-docker rmi -f $(IMAGE_NAME):$(IMAGE_TAG) 2> /dev/null
ifeq ($(DBG), yes)
	docker build --build-arg DEBUG_IMAGE=$(DBG) --build-arg REST_API=$(REST) --force-rm -t $(IMAGE_NAME):$(IMAGE_TAG) -f {{NAME}}.Dockerfile .
else
	cat {{NAME}}.Dockerfile {{NAME}}.slim.Dockerfile | docker build --build-arg DEBUG_IMAGE=$(DBG) --build-arg REST_API=$(REST) --force-rm -t $(IMAGE_NAME):$(IMAGE_TAG) -f - .
endif
	docker image inspect $(IMAGE_NAME):$(IMAGE_TAG) > $@

image-info-Darwin:
	@rm -f $(IMAGE_PROXY)
	@-docker image inspect $(IMAGE_NAME):$(IMAGE_TAG) > /dev/null 2>&1 && docker image inspect $(IMAGE_NAME):$(IMAGE_TAG) > $(IMAGE_PROXY) > /dev/null 2>&1 \
	&& touch -t $(shell /bin/date -j -f '%Y%m%d%H%M%S%Z' `docker image inspect --format '{{.Created}}' $(IMAGE_NAME):$(IMAGE_TAG) 2> /dev/null | sed -E 's/([:T-]|[.][0-9]+)//g' | sed 's/Z/GMT/'` +'%Y%m%d%H%M.%S' 2>/dev/null) $(IMAGE_PROXY) \
	|| true

image-info-Linux:
	@rm -f $(IMAGE_PROXY)
	@-docker image inspect $(IMAGE_NAME):$(IMAGE_TAG) > /dev/null 2>&1 > /dev/null && docker image inspect $(IMAGE_NAME):$(IMAGE_TAG) > $(IMAGE_PROXY) > /dev/null 2>&1 \
	&& touch --date=`docker image inspect --format '{{.Created}}' $(IMAGE_NAME):$(IMAGE_TAG) 2>/dev/null` $(IMAGE_PROXY) \
	|| true

image: image-info-$(shell uname -s)
	@$(MAKE) -s -f $(THIS_FILE) IMAGE=$(IMAGE_NAME):$(IMAGE_TAG) DBG=$(DBG) IMAGE_PROXY=$(IMAGE_PROXY) PUSH_REPO=$(PUSH_REPO) $(IMAGE_PROXY)
	@$(MAKE) -s -f $(THIS_FILE) IMAGE=$(IMAGE_NAME):$(IMAGE_TAG) DBG=$(DBG) IMAGE_PROXY=$(IMAGE_PROXY) PUSH_REPO=$(PUSH_REPO) $(DOCKER)

{{NAME}}-server: {{NAME}}-server.C $(PB_GENERATED_CC) $(PB_GENERATED_H) $(SERVER_XTRA_H)
	${CXX} $(SERVER_CFLAGS) $(CFLAGS) -o $@  $< $(PB_GENERATED_CC) $(SERVER_XTRA_C) $(SERVER_LFLAGS)

{{NAME}}-client: {{NAME}}-client.C $(PB_GENERATED_CC) $(PB_GENERATED_H) 
	${CXX} $(GRPC_INCS) $(CFLAGS) -o $@  $<  $(PB_GENERATED_CC) $(GRPC_LIBS)

clean:
	rm -f $(IMAGE_PROXY) {{NAME}}-server {{NAME}}-client 

all: {{NAME}}-server {{NAME}}-client 

client: {{NAME}}-client

server: {{NAME}}-server 

deploy: {{NAME}}-server {{NAME}}-client
ifneq ($(DBG), yes)
	strip $^
endif
	mkdir -p ~/{{NAME}}/docs
	mkdir -p ~/{{NAME}}/www
	cp $^ ~/{{NAME}}
	cp $(wildcard *.pem) ~/{{NAME}}
	cp $(wildcard docs/*.proto) $(wildcard docs/*.svg) $(wildcard docs/*.flow) ~/{{NAME}}/docs
ifeq ($(REST), yes)
	cp $(wildcard www/*.html) $(wildcard www/*.css) $(wildcard www/*.js) ~/{{NAME}}/www
endif
