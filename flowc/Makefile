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
#   Base image label - Default base image for when --runtime is not used with flowc 
#BASE_IMAGE?=alpine-3.12 
-include makefile.local

BUILD_VERSION=0.15.6

DBG?=no
ifeq ($(DBG), yes) 
CCFLAGS?=-Og -g
STACK_TRACE?=1
BUILD_VERSION:=$(BUILD_VERSION)d
else
CCFLAGS?=-O3
STACK_TRACE?=0
endif


## One of the runtimes alpine-3.11, alpine-3.12, ubuntu-18.04, ubuntu-20.04, redhat-ubi7, or redhat-ubi8. Override in 'makefile.local'
BASE_IMAGE?=redhat-ubi8

INSTALL_PREFIX?=/usr/local/bin

.PHONY: clean tar image image-info install targets image-info-Darwin.ts image-info-Darwin image-info-Linux all-images flowc-image
.PHONY: alpine-3.12 ubuntu-18.04 ubuntu-20.04 redhat-ubi7 alpine-3.11 redhat-ubi8 alpine-3.13 
.PHONY: tools 

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

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
GRPC_LIBS?=$(shell pkg-config --libs-only-L protobuf) -lprotoc $(shell pkg-config --libs grpc++ protobuf)

TEMPLATE_FILES:=template.Dockerfile template.slim.Dockerfile template.Makefile template.client.C template.help \
	template.driver.sh template.docker-compose.yaml template.kubernetes.group.yaml template.kubernetes.yaml \
	template.server.C template.syntax template.index.html template.server.pem\
    	

TEMPLATE_FILES:=$(filter-out %~,$(wildcard template.*))

OBJS:=flowc.o flow-templates.o flow-compiler.o flow-gclient.o flow-gconf.o flow-ast.o flow-ggrpc.o flow-gserver.o flow-version.o flow-opcodes.o

## Minimal set of files to portably build flow-compiler. 
## Includes intermediate files, but will require only the gRPC libraries to build.
MINSRC_FILES:=Makefile flow-parser.c flow-parser.h $(wildcard *.H) $(OBJS:.o=.C) rr-keys.sh rr-get.sh flow-parser.y

%.o: %.C
	${CXX} -std=c++11 ${GRPC_INCS} $(CCFLAGS) -c -MMD -MF $*.dep -o $@ $< 

flowc: $(OBJS) 
	${CXX} -o $@ $(CCFLAGS) $^ ${GRPC_LIBS} 

flow-version.o:
	${CXX} -std=c++11 "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\""  "-DBASE_IMAGE=\"$(BASE_IMAGE)\"" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ flow-version.C

flow-compiler.o: flow-parser.c

flowc.o: flowc.C
	${CXX} -std=c++11 "-DSTACK_TRACE=$(STACK_TRACE)" ${GRPC_INCS} $(CCFLAGS) -c -MMD -MF $*.dep -o $@ $<

flow-parser.h: flow-parser.c
	touch -r $^ $@

flow-parser.c: flow-parser.y
	$(LEMON) $<

clean:
	rm -f flowc $(TOOLS) $(OBJS:.o=.dep) $(OBJS) flow-templates.C flow-compiler-src.tgz flow-base-image-info.json flow-compiler-minsrc.tgz 

RUNTIMES?=$(patsubst runtimes/%/template.runtime.Dockerfile,%,$(wildcard runtimes/*/template.runtime.Dockerfile))
RUNTIME_TEMPLATES?=$(addsuffix /template.runtime.Dockerfile,$(addprefix runtimes/,$(RUNTIMES)))

flow-templates.C: $(TEMPLATE_FILES) rr-keys.sh rr-get.sh $(RUNTIME_TEMPLATES)
	echo "#include <string>" > $@
	echo "#include <map>" >> $@
	for FILE in $^; do echo "char const *$$FILE = R\"TEMPLATE(" | tr -- '-./' '___' | tr -d $$'\n'; cat "$$FILE"; echo ')TEMPLATE";'; echo "unsigned long const len_$$FILE = " | tr -- '-./' '___' | tr -d $$'\n'; cat "$$FILE" | wc -c; echo ";";  done >> $@
	echo "std::map<std::string, char const *> template_runtime_Dockerfile = {" >> $@
	for RUNTL in $(RUNTIMES); do echo "{\"$$RUNTL\", " | tr -d $$'\n' ; echo "runtimes/$$RUNTL/template.runtime.Dockerfile" | tr -- '-./' '___' | tr -d $$'\n'; echo "},"; done >> $@
	echo "};" >> $@

flow-compiler-src.tgz: Makefile flow-parser.c flow-parser.h $(wildcard *.[Hy]) $(TEMPLATE_FILES) flow-compiler.C
	$(TAR) -czf $@ $^ --transform 's,^,flow-compiler/,'

tar: flow-compiler-src.tgz 

flow-compiler-minsrc.tgz: $(MINSRC_FILES)
	$(TAR) -czf $@ --transform 's,^,flow-compiler/,' $^

ifeq ($(DBG), yes) 
install: flowc
	mkdir -p $(INSTALL_PREFIX)
	ln -s $(shell pwd)/$^ $(INSTALL_PREFIX)/$^
else
install: flowc
	mkdir -p $(INSTALL_PREFIX)
	strip $^
	cp $^ $(INSTALL_PREFIX)
endif

#################### Docker image builder
flow-base-image-info.json: runtimes/$(BASE_IMAGE)/template.runtime.Dockerfile runtimes/$(BASE_IMAGE)/base.Dockerfile flowc.Dockerfile $(MINSRC_FILES)
	-docker rmi -f $(IMAGE)  > /dev/null 2>&1
	$(TAR) -czf flow-compiler-minsrc.tgz  --transform 's,^,flow-compiler/,' $(MINSRC_FILES)
	cat runtimes/$(BASE_IMAGE)/template.runtime.Dockerfile runtimes/$(BASE_IMAGE)/base.Dockerfile flowc.Dockerfile | docker build --pull --force-rm --build-arg "STACK_TRACE=$(STACK_TRACE)" --build-arg "RUNTIME=$(RUNTIMES)" --build-arg "DEBUG_FLOWC=$(DBG)" --build-arg "BUILD_ID=$(BUILD_ID)" --build-arg "BUILD_VERSION=$(BUILD_VERSION)" --build-arg "BASE_IMAGE=$(BASE_IMAGE)" -t $(IMAGE) -f - .
	rm -f flow-compiler-minsrc.tgz
	-docker image inspect $(IMAGE) > /dev/null 2>&1 && docker image inspect $(IMAGE) > $@

####### Create a temprorary file with the docker image timestamp
image-info-Darwin.ts: 
	@docker image inspect $(IMAGE) > /dev/null 2>&1 \
		&& /bin/date -j -f '%Y%m%d%H%M%S%Z' `docker image inspect --format '{{.Created}}' $(IMAGE) | sed -E 's/([:T-]|[.][0-9]+)//g' | sed 's/Z/GMT/'` +'%Y%m%d%H%M.%S' > image-info-Darwin.ts \
	|| echo 202011121509.36 > image-info-Darwin.ts

image-info-Darwin: image-info-Darwin.ts
	@rm -f flow-base-image-info.json
	@docker image inspect $(IMAGE) > /dev/null 2>&1 && docker image inspect $(IMAGE) > flow-base-image-info.json \
		&& touch -t `cat image-info-Darwin.ts` flow-base-image-info.json \
	|| true
	@rm -f image-info-Darwin.ts


image-info-Linux:
	@rm -f flow-base-image-info.json
	@-docker image inspect $(IMAGE) > /dev/null 2>&1 \
		&& docker image inspect $(IMAGE) > flow-base-image-info.json && touch --date=`docker image inspect --format '{{.Created}}' $(IMAGE)` flow-base-image-info.json \
	|| true

image-info: image-info-$(HOST_OS)

flowc-image: image-info-$(HOST_OS)
	@$(MAKE) -s flow-base-image-info.json
	@echo Image $(IMAGE)  is up to date

image:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$(BASE_IMAGE)" "BASE_IMAGE=$(BASE_IMAGE)" flowc-image

alpine-3.13:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" "STACK_TRACE=0" flowc-image

alpine-3.12:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" "STACK_TRACE=0" flowc-image

alpine-3.11:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" "STACK_TRACE=0" flowc-image

ubuntu-18.04:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

ubuntu-20.04:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

redhat-ubi7:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

redhat-ubi8:
	@rm -f flow-templates.C
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

all-images: $(RUNTIMES)

recli: recli.C
	$(CXX) -std=c++11 -O3 -o recli recli.C $(shell pkg-config --libs libcurl)

vex: vex.C vex.H stru1.H
	$(CXX) -std=c++11 -O3 -o vex vex.C

TOOLS=recli vex

tools: $(TOOLS)

#### For information about lemon: http://www.hwaci.com/sw/lemon/
lemon.c:
	wget 'https://www.sqlite.org/src/tarball/sqlite.tar.gz?r=release' -q -O - | $(TAR) --strip-components=2 -xzv sqlite/tool/lemon.c sqlite/tool/lempar.c

lemon: lemon.c
	$(CC) -O3 -o $@ $<
	@echo "To use the lemon just built, add LEMON:=./lemon to makefile.local"

parser: flow-parser.h

-include $(OBJS:.o=.dep)

targets:
	@echo "flowc, image, recli, evex, tools, lemon, tar, clean, install, all-images"
