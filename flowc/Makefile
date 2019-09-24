#######################################################################
#	Customizable variables (use makefile.local, environment, or command line)
#
#   CXX compile flags for gRPC
#GRPC_INCS=-pthread -I/usr/local/include
#	
#	Link otptions for gRPC
#GRPC_LIBS=-L/usr/local/lib -lprotoc -lgrpc++ -lprotobuf -pthread 
#
#	Build ID string (defaults to the timestamp of the last modified file and the git hash)
#BUILD_ID=
#
#	Destination directory for the executable file 
#INSTALL_PREFIX?=/usr/local/bin

BUILD_VERSION=0.1.0

.PHONY: clean tar image image-info image-info-Linux image-info-Darwin install targets
-include makefile.local

# Find a suitable tar
#
HOST_OS:=$(shell uname -s)
ifeq ($(HOST_OS), Darwin)
ifeq ($(shell which gnutar), )
ifeq ($(shell which gtar), )
$(warning GNU tar is needed to build the Docker image)
$(warning Either install it with 'sudo port install gnutar' or with 'brew install gtar',)
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

# destination directory
INSTALL_PREFIX?=/usr/local/bin

BASE_IMAGE=flow-base:alpine-edge

STACK_TRACE?=1

BASE_IMAGE_TAG=$(shell echo "$(BASE_IMAGE)" | cut -d ':' -f2)
IMAGE_TAG?=$(BASE_IMAGE_TAG)-$(BUILD_VERSION)
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

BUILD_ID?=$(GIT_TS) $(GIT_GUID)

ifeq ($(shell pkg-config --exists grpc++ protobuf && echo yes || echo no), no)
$(error gRPC libraries are needed, set GRPC_LIBS and GRPC_INCS in 'makefile.local')
endif

GRPC_INCS ?= $(shell pkg-config --cflags grpc++ protobuf)
GRPC_LIBS ?= $(shell pkg-config --libs-only-L protobuf) -lprotoc  $(shell pkg-config --libs grpc++ protobuf)

TEMPLATE_FILES:=$(subst ~,,$(wildcard template.*))

OBJS:=flowc.o flow-templates.o flow-compiler.o flow-gclient.o flow-gconf.o flow-ast.o flow-ggrpc.o flow-gserver.o flow-version.o

## Minimal set of files to portably build flow-compiler. 
## Includes intermediate files, but will require only the gRPC libraries to build.
MINSRC_FILES:=Makefile flow-parser.c flow-parser.h $(wildcard *.H) $(OBJS:.o=.C) artiget.sh cosget.sh

%.o: %.C
	${CXX} -std=c++17 ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ $< 

flowc: $(OBJS) 
	${CXX} -o $@ -O3 $^ ${GRPC_LIBS} 

flow-version.o:
	${CXX} -std=c++17 "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\"" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ flow-version.C

flowc.o: flowc.C
	${CXX} -std=c++17 "-DSTACK_TRACE=$(STACK_TRACE)" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ $^

flow-parser.h: flow-parser.c
	touch -r $^ $@

flow-parser.c: flow-parser.Y
	$(LEMON) $<

clean:
	rm -f flowc $(OBJS:.o=.dep) $(OBJS) flow-templates.C flow-compiler-src.tgz flow-base-image-info.json flow-compiler-minsrc.tgz 

flow-templates.C: $(TEMPLATE_FILES) artiget.sh cosget.sh
	echo "#include <string>" > $@
	echo "#include <map>" >> $@
	for FILE in $^; do echo "char const *$$FILE = R\"TEMPLATE(" | tr -- '-.' '__' | tr -d $$'\n'; cat "$$FILE"; echo ')TEMPLATE";'; done >> $@
	echo "std::map<std::string, char const *> templates = {" >> $@
	for FILE in $^; do echo "{\"$$FILE\"" | sed 's/template.//' | tr -d $$'\n'; echo ", $$FILE}," | tr -- '-.' '__'; done >> $@
	echo "};" >> $@

flow-compiler-src.tgz: Makefile flow-parser.c flow-parser.h $(wildcard *.[HY]) $(TEMPLATE_FILES) flow-compiler.C
	$(TAR) -czf $@ $^ --transform 's,^,flow-compiler/,'

tar: flow-compiler-src.tgz 

flow-compiler-minsrc.tgz: $(MINSRC_FILES)
	$(TAR) -czf $@ --transform 's,^,flow-compiler/,' $^

install: flowc
	mkdir -p $(INSTALL_PREFIX)
	strip $^
	cp $^ $(INSTALL_PREFIX)

#################### Docker image builder
flow-base-image-info.json: template.runtime.Dockerfile base.Dockerfile flowc.Dockerfile $(MINSRC_FILES) cosget.sh artiget.sh
	-docker rmi -f $(IMAGE) 2> /dev/null
	$(TAR) -czf flow-compiler-minsrc.tgz  --transform 's,^,flow-compiler/,' $(MINSRC_FILES)
	cat template.runtime.Dockerfile base.Dockerfile flowc.Dockerfile | docker build --force-rm  --build-arg "BUILD_ID=$(BUILD_ID)" --build-arg "BUILD_VERSION=$(BUILD_VERSION)" -t $(IMAGE) -t $(IMAGE_NAME):latest -f - .
	rm -f flow-compiler-minsrc.tgz
	-docker image inspect $(IMAGE) > /dev/null && docker image inspect $(IMAGE) > $@

image-info-Darwin:
	@rm -f flow-base-image-info.json
	@-docker image inspect $(IMAGE) > /dev/null && docker image inspect $(IMAGE) > flow-base-image-info.json && touch -t $(shell /bin/date -j -f '%Y%m%d%H%M%S%Z' `docker image inspect --format '{{.Created}}' $(IMAGE) | sed -E 's/([:T-]|[.][0-9]+)//g' | sed 's/Z/GMT/'` +'%Y%m%d%H%M.%S') flow-base-image-info.json

image-info-Linux:
	@rm -f flow-base-image-info.json
	@-docker image inspect $(IMAGE) > /dev/null && docker image inspect $(IMAGE) > flow-base-image-info.json && touch --date=`docker image inspect --format '{{.Created}}' $(IMAGE)` flow-base-image-info.json

image-info: image-info-$(HOST_OS)

image: image-info
	@$(MAKE) -s flow-base-image-info.json

base-image:
	cat template.runtime.Dockerfile base.Dockerfile | docker build -t $(BASE_IMAGE) -t $(shell echo "$(BASE_IMAGE)" | cut -d ':' -f1):latest  -f - .

runtime-image:
	docker build -t $(RUNTIME_IMAGE) -t $(shell echo "$(RUNTIME_IMAGE)" | cut -d ':' -f1):latest -f template.runtime.Dockerfile .

#### For information about lemon: http://www.hwaci.com/sw/lemon/
lemon.c:
	wget 'https://www.sqlite.org/src/tarball/sqlite.tar.gz?r=release' -q -O - | $(TAR) --strip-components=2 -xzv sqlite/tool/lemon.c sqlite/tool/lempar.c

lemon: lemon.c
	$(CC) -O3 -o $@ $<
	@echo "To use the lemon just built, add LEMON:=./lemon to makefile.local"

parser: flow-parser.h

-include $(OBJS:.o=.dep)

targets:
	@echo "flowc, image, base-image, runtime-image, lemon, tar, clean, install"
