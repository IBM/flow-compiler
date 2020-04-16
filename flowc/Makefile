#######################################################################
#	Customizable variables (use makefile.local, environment, or command line)
#
#   CXX compile flags for gRPC
#GRPC_INCS=-pthread -I/usr/local/include
#	
#	Link otptions for gRPC
#GRPC_LIBS=-L/usr/local/lib -lprotoc -lgrpc++ -lprotobuf -pthread 
#
#	Destination directory for the executable file 
#INSTALL_PREFIX?=/usr/local/bin
#
#   Base image label - set of docker files with the base images for flowc and for the application
#BASE_IMAGE?=alpine-edge   
-include makefile.local

BUILD_VERSION=0.11.0

## One of the runtimes alpine-3.11, alpine-edge, ubuntu-18.04, or redhat-ubi7. Override 'makefile.local'
BASE_IMAGE?=redhat-ubi7

INSTALL_PREFIX?=/usr/local/bin

.PHONY: clean tar image image-info image-info-Linux image-info-Darwin install targets 
.PHONY: alpine-edge ubuntu-18.04 redhat-ubi7 alpine-3.11

# Find a suitable tar - needed for building the image
HOST_OS:=$(shell uname -s)
ifeq ($(HOST_OS), Darwin)
ifeq ($(shell which gnutar), )
ifeq ($(shell which gtar), )
$(warning GNU tar is needed to build the Docker image)
$(warning Either install it with 'sudo port install gnutar' or with 'brew install gnu-tar',)
$(warning or set TAR to a GNU tar compatible executable in 'makefile.local'.)
else
TAR?=gtar
endif
else
TAR?=gnutar
endif
else
TAR?=tar
endif

# lemon is needed only if changes are made to the parser
LEMON?=lemon

STACK_TRACE?=1

IMAGE_TAG?=$(BUILD_VERSION).$(BASE_IMAGE)
IMAGE?=flowc:$(IMAGE_TAG)

RUNTIME_IMAGE?=flow-runtime:$(IMAGE_TAG)
IMAGE_NAME:=$(shell echo "$(IMAGE)" | cut -d ':' -f1)
IMAGE_TAG:=$(shell  echo "$(IMAGE)" | cut -d ':' -f2)


ifeq ($(BUILD_ID), )
GIT_TS:=$(shell git show -s --format=%cD)
GIT_MODIFIED_COUNT:=$(strip $(shell git status -suno | wc -l))
GIT_GUID:=$(shell  git show -s --format=%h-%ct)
ifneq ($(GIT_MODIFIED_COUNT), 0)
GIT_GUID:=$(GIT_GUID)-m$(GIT_MODIFIED_COUNT)
endif
endif

## Create a build id out of the git GUID.
BUILD_ID?=$(GIT_TS) $(GIT_GUID)

#ifeq ($(shell pkg-config --exists grpc++ protobuf && echo yes || echo no), no)
#$(error gRPC libraries are needed, set GRPC_LIBS and GRPC_INCS in 'makefile.local')
#endif

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
GRPC_LIBS?=$(shell pkg-config --libs-only-L protobuf) -lprotoc $(shell pkg-config --libs grpc++ protobuf)

TEMPLATE_FILES:=template.Dockerfile template.Makefile template.client.C template.help \
	template.docker-compose.sh template.docker-compose.yaml \
	template.kubernetes.group.yaml template.kubernetes.sh template.kubernetes.yaml \
	template.server.C template.syntax template.index.html \
    	
BASE_RUNTIME_TEMPLATE=$(BASE_IMAGE)/template.runtime.Dockerfile 

OBJS:=flowc.o flow-templates.o flow-compiler.o flow-gclient.o flow-gconf.o flow-ast.o flow-ggrpc.o flow-gserver.o flow-version.o flow-opcodes.o

## Minimal set of files to portably build flow-compiler. 
## Includes intermediate files, but will require only the gRPC libraries to build.
MINSRC_FILES:=Makefile flow-parser.c flow-parser.h $(wildcard *.H) $(OBJS:.o=.C) $(TEMPLATE_FILES) rr-keys.sh rr-get.sh flow-parser.y

%.o: %.C
	${CXX} -std=c++11 ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ $< 

flowc: $(OBJS) 
	${CXX} -o $@ -O3 $^ ${GRPC_LIBS} 

flow-version.o:
	${CXX} -std=c++11 "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\""  "-DBASE_IMAGE=\"$(BASE_IMAGE)\"" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ flow-version.C

flow-compiler.o: flow-parser.c

flowc.o: flowc.C
	${CXX} -std=c++11 "-DSTACK_TRACE=$(STACK_TRACE)" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ $<

flow-parser.h: flow-parser.c
	touch -r $^ $@

flow-parser.c: flow-parser.y
	$(LEMON) $<

clean:
	rm -f flowc $(OBJS:.o=.dep) $(OBJS) flow-templates.C flow-compiler-src.tgz flow-base-image-info.json flow-compiler-minsrc.tgz 

RUNTIME_TEMPLATES=$(wildcard */template.runtime.Dockerfile)

flow-templates.C: $(TEMPLATE_FILES) rr-keys.sh rr-get.sh $(RUNTIME_TEMPLATES)
	echo "#include <string>" > $@
	echo "#include <map>" >> $@
	for FILE in $^; do echo "char const *$$FILE = R\"TEMPLATE(" | tr -- '-./' '___' | tr -d $$'\n'; cat "$$FILE"; echo ')TEMPLATE";'; done >> $@
	echo "std::map<std::string, char const *> template_runtime_Dockerfile = {" >> $@
	for RUNTL in $(RUNTIME_TEMPLATES:/template.runtime.Dockerfile=); do echo "{\"$$RUNTL\", " | tr -d $$'\n' ; echo "$$RUNTL/template.runtime.Dockerfile" | tr -- '-./' '___' | tr -d $$'\n'; echo "},"; done >> $@
	echo "};" >> $@

flow-compiler-src.tgz: Makefile flow-parser.c flow-parser.h $(wildcard *.[Hy]) $(TEMPLATE_FILES) flow-compiler.C
	$(TAR) -czf $@ $^ --transform 's,^,flow-compiler/,'

tar: flow-compiler-src.tgz 

flow-compiler-minsrc.tgz: $(MINSRC_FILES)
	$(TAR) -czf $@ --transform 's,^,flow-compiler/,' $^

install: flowc
	mkdir -p $(INSTALL_PREFIX)
	strip $^
	cp $^ $(INSTALL_PREFIX)

#################### Docker image builder
flow-base-image-info.json: $(BASE_IMAGE)/template.runtime.Dockerfile $(BASE_IMAGE)/base.Dockerfile flowc.Dockerfile $(MINSRC_FILES)
	-docker rmi -f $(IMAGE) 2>&1 > /dev/null
	$(TAR) -czf flow-compiler-minsrc.tgz  --transform 's,^,flow-compiler/,' $(MINSRC_FILES)
	cat $(BASE_IMAGE)/template.runtime.Dockerfile $(BASE_IMAGE)/base.Dockerfile flowc.Dockerfile | docker build --force-rm  --build-arg "BUILD_ID=$(BUILD_ID)" --build-arg "BUILD_VERSION=$(BUILD_VERSION)" --build-arg "BASE_IMAGE=$(BASE_IMAGE)" -t $(IMAGE) -f - .
	rm -f flow-compiler-minsrc.tgz
	-docker image inspect $(IMAGE) 2>&1 > /dev/null && docker image inspect $(IMAGE) > $@

image-info-Darwin:
	@rm -f flow-base-image-info.json
	@-docker image inspect $(IMAGE) 2>&1 > /dev/null && docker image inspect $(IMAGE) > flow-base-image-info.json && touch -t $(shell /bin/date -j -f '%Y%m%d%H%M%S%Z' `docker image inspect --format '{{.Created}}' $(IMAGE) | sed -E 's/([:T-]|[.][0-9]+)//g' | sed 's/Z/GMT/'` +'%Y%m%d%H%M.%S') flow-base-image-info.json

image-info-Linux:
	@rm -f flow-base-image-info.json
	@-docker image inspect $(IMAGE) 2>&1 > /dev/null && docker image inspect $(IMAGE) > flow-base-image-info.json && touch --date=`docker image inspect --format '{{.Created}}' $(IMAGE)` flow-base-image-info.json

image-info: image-info-$(HOST_OS)

image: image-info
	@$(MAKE) -s flow-base-image-info.json

alpine-edge:
	@$(MAKE) "BASE_IMAGE=$@" image

alpine-3.11:
	@$(MAKE) "BASE_IMAGE=$@" image

ubuntu-18.04:
	@$(MAKE) "BASE_IMAGE=$@" image

redhat-ubi7:
	@$(MAKE) "BASE_IMAGE=$@" image


#### For information about lemon: http://www.hwaci.com/sw/lemon/
lemon.c:
	wget 'https://www.sqlite.org/src/tarball/sqlite.tar.gz?r=release' -q -O - | $(TAR) --strip-components=2 -xzv sqlite/tool/lemon.c sqlite/tool/lempar.c

lemon: lemon.c
	$(CC) -O3 -o $@ $<
	@echo "To use the lemon just built, add LEMON:=./lemon to makefile.local"

parser: flow-parser.h

-include $(OBJS:.o=.dep)

targets:
	@echo "flowc, image, lemon, tar, clean, install"
