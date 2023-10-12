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
#BASE_IMAGE?=alpine-3.15
#
# 	Force gRPC or CivetWeb to a certain version instead of using the default in the Dockerfile.
#GRPC_VERSION?=1.49.1
#CIVETWEB_VERSION?=1.15

-include makefile.local

BUILD_VERSION?=0.22.0

ifndef BUILD_ID

GIT_TS:=$(shell git show -s --format=%cD)
GIT_MODIFIED_COUNT:=$(strip $(shell git status -suno | wc -l))
ifeq ($(GIT_MODIFIED_COUNT), 0)
	GIT_MODIFIED_COUNT:=
else
	GIT_MODIFIED_COUNT:=m$(GIT_MODIFIED_COUNT)
endif
GIT_GUID:=$(shell git show -s --format=%h)
GIT_TAG:=$(shell git tag --contains $(GIT_GUID) | tail -1)
GIT_BRANCH:=$(shell git branch --show-current 2> /dev/null || git branch | grep -E '^\*' | sed -E 's/^[*] *//')
## if not on the master branch, prepend the branch name
ifneq ($(GIT_BRANCH), master)
	GIT_GUID:=$(GIT_BRANCH)-$(GIT_GUID)
endif
## if this checkout is tagged set the build version to that
ifeq ($(GIT_TAG), )
	BUILD_ID?=$(strip $(GIT_TS) $(GIT_GUID)$(GIT_MODIFIED_COUNT))
else
	BUILD_VERSION:=$(GIT_TAG)
	BUILD_ID?=$(strip $(GIT_TS)$(GIT_MODIFIED_COUNT))
endif

endif # BUILD_ID

## write the current version and build id into a file,
## in order to have a timestamp associated with the build status
BUILD_STAMP:=$(shell cat build.id 2> /dev/null)
ifneq ($(BUILD_STAMP), $(BUILD_VERSION)+$(BUILD_ID))
	BUILD_STAMP:=$(shell echo $(BUILD_VERSION)+$(BUILD_ID) > build.id; cat build.id)
endif

DBG?=no
ifeq ($(DBG), yes) 
	CCFLAGS?=-O0 -g
	STACK_TRACE?=1
	BUILD_ID:=$(BUILD_ID) dbg
else
	CCFLAGS?=-O3
	STACK_TRACE?=0
endif

## one of the runtimes alpine-3.x, ubuntu-x.04, redhat-ubix. Override in 'makefile.local'
BASE_IMAGE?=redhat-ubi8

INSTALL_PREFIX?=/usr/local/bin

ifneq ($(GRPC_VERSION), )
	DOCKER_ARG_GRPC_VERSION:=--build-arg "GRPC_VERSION=$(GRPC_VERSION)"
endif

ifneq ($(CIVETWEB_VERSION), )
	DOCKER_ARG_CIVETWEB_VERSION:=--build-arg "CIVETWEB_VERSION=$(CIVETWEB_VERSION)"
endif

# Find a suitable tar - needed for building the image
HOST_OS:=$(shell uname -s)
ifeq ($(TAR), )
	ifeq ($(HOST_OS), Darwin)
		ifeq ($(shell which gnutar), )
			ifeq ($(shell which gtar), )
				$(warning GNU tar is needed to build the Docker image)
				$(warning Either install it with 'brew install gnu-tar',)
				$(warning or set TAR to a GNU tar compatible executable in 'makefile.local'.)
			else
				TAR:=gtar
			endif
		else
			TAR:=gnutar
		endif
	else
		TAR:=tar
	endif
endif

# lemon is needed only if changes are made to the parser
LEMON?=lemon

IMAGE_TAG?=$(BUILD_VERSION).$(BASE_IMAGE)
IMAGE?=flowc:$(IMAGE_TAG)

RUNTIME_IMAGE?=flow-runtime:$(IMAGE_TAG)
IMAGE_NAME:=$(shell echo "$(IMAGE)" | cut -d ':' -f1)
IMAGE_TAG:=$(shell  echo "$(IMAGE)" | cut -d ':' -f2)

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
ifeq ($(HOST_OS), Darwin)
	GRPC_LIBS?=$(shell pkg-config --libs-only-L protobuf) -lprotoc $(shell pkg-config --libs grpc++ protobuf) -framework CoreFoundation
else
	GRPC_LIBS?=$(shell pkg-config --libs-only-L protobuf) -lprotoc $(shell pkg-config --libs grpc++ protobuf)
endif

RUNTIMES:=alpine-3.18 redhat-ubi8 ubuntu-22.04 debian-bullseye ubuntu-20.04
RUNTIMES?=$(patsubst runtimes/%/template.runtime.Dockerfile,%,$(wildcard runtimes/*/template.runtime.Dockerfile))
RUNTIME_TEMPLATES?=$(addsuffix /template.runtime.Dockerfile,$(addprefix runtimes/,$(RUNTIMES)))

.PHONY: $(RUNTIMES)
.PHONY: clean tar image image-info install targets image-info-Darwin.ts image-info-Darwin image-info-Linux all-images flowc-image 
.PHONY: tools 

TEMPLATE_FILES:=$(filter-out %~,$(wildcard template.*)) flowc.help

OBJS:=filu.o flow-ast.o flow-compiler.o flow-opcodes.o flow-pgcc.o flow-templates.o flow-funcs.o flowc.o \
	  gc-client.o gc-conf.o gc-grpc.o gc-server.o grpc-helpers.o gvars.o print-graph.o print-pseu.o \
	  strtrs.o tree-accessors.o veximpl.o version.o ansi-escapes.o

## minimal set of files to portably build flow-compiler,
## includes intermediate files, but will require only the gRPC libraries to build
MINSRC_FILES:=Makefile flow-parser.c flow-parser.h $(wildcard *.H) flow-templates.H $(OBJS:.o=.C) rr-keys.sh rr-get.sh flow-parser.y flowc.help

%.o: %.C
	@echo ${CXX} -std=c++17 $(CCFLAGS) -c $<
	@${CXX} -std=c++17 "-DSTACK_TRACE=$(STACK_TRACE)" ${GRPC_INCS} $(CCFLAGS) -c -MMD -MF $*.dep -o $@ $<

%.C: %.hlp
	@echo Writing $@ from $^
	@printf "#include <map>\n#include <string>\n#include <sstream>\n#include \"filu.H\"\nnamespace templates {\n" > $@
	@for FILE in $^; do ID=$$(echo "$$FILE" | sed -E 's/^template.//' | tr -- '-./' '___'); echo "static unsigned char const $${ID}_z[] = {"; cat "$$FILE" | gzip -c | xxd -i; echo '};'; printf "std::string $${ID}() {return filu::gunzip($${ID}_z, %d);}\n" $$(cat "$$FILE" | gzip -c | wc -c); done >> $@
	@echo "}" >> $@

flowc: $(OBJS) 
	${CXX} -o $@ $(CCFLAGS) $^ ${GRPC_LIBS} -lz 
	@echo "====================================================================="
	@./$@ --version

version.o: version.C build.id
	@echo ${CXX} -std=c++17 "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\"" "-DBASE_IMAGE=\"$(BASE_IMAGE)\"" -O3 -c $<
	@${CXX} -std=c++17 "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\"" "-DBASE_IMAGE=\"$(BASE_IMAGE)\"" ${GRPC_INCS} -O3 -c -MMD -MF $*.dep -o $@ $<

flow-compiler.o: flow-parser.c

gc-conf.o: flow-templates.H

flow-parser.h: flow-parser.c
	@echo $@ > /dev/null

flow-parser.c: flow-parser.y
	$(LEMON) $<

flow-scanner.C: flow-scanner.lex
	 flex++ -Cem -o $@  $<

clean:
	rm -f flowc $(TOOLS) cdmp $(OBJS:.o=.dep) $(OBJS) flow-templates.C flow-templates.H flow-compiler-src.tgz flow-base-image-info.json flow-compiler-minsrc.tgz 

flow-templates.C: $(TEMPLATE_FILES) rr-keys.sh rr-get.sh $(RUNTIME_TEMPLATES)
	@echo Writing $@ $(patsubst %.C,%.H,$@)
	@echo Adding $^ to $@
	@printf "#include <map>\n#include <string>\n#include <sstream>\n#include \"filu.H\"\nnamespace templates {\n" > $@
	@for FILE in $^; do ID=$$(echo "$$FILE" | sed -E 's/^template.//' | tr -- '-./' '___'); echo "static unsigned char const $${ID}_z[] = {"; cat "$$FILE" | gzip -c | xxd -i; echo '};'; printf "std::string $${ID}() {return filu::gunzip($${ID}_z, %d);}\n" $$(cat "$$FILE" | gzip -c | wc -c); done >> $@
	@echo "std::map<std::string, std::string (*)()> ztemplate_runtime_Dockerfile = {" >> $@
	@for RUNTL in $(RUNTIMES); do echo "{\"$$RUNTL\", " | tr -d $$'\n' ; echo "runtimes/$$RUNTL/template.runtime.Dockerfile" | tr -- '-./' '___' | tr -d $$'\n'; echo "},"; done >> $@
	@echo "};" >> $@
	@echo "}" >> $@
	@printf "#ifndef H_%s\n#define H_%s\n#include <map>\n#include <string>\nnamespace templates {\n" $$(echo $(patsubst %.C,%.H,$@) | tr -- '-./' '___') $$(echo $(patsubst %.C,%.H,$@) | tr -- '-./' '___') > $(patsubst %.C,%.H,$@)
	@for FILE in $^; do ID=$$(echo "$$FILE" | sed -E 's/^template.//' | tr -- '-./' '___'); printf "std::string $${ID}();\n"; done >> $(patsubst %.C,%.H,$@)
	@echo "extern std::map<std::string, std::string (*)()> ztemplate_runtime_Dockerfile;" >> $(patsubst %.C,%.H,$@)
	@printf "}\n#endif" >> $(patsubst %.C,%.H,$@)

flow-templates.H: flow-templates.C
	@echo $@

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
	cat runtimes/$(BASE_IMAGE)/template.runtime.Dockerfile runtimes/$(BASE_IMAGE)/base.Dockerfile flowc.Dockerfile | docker build --force-rm \
		--build-arg "STACK_TRACE=$(STACK_TRACE)" \
		--build-arg "RUNTIME=$(RUNTIMES)" \
		--build-arg "DEBUG_FLOWC=$(DBG)" \
		--build-arg "BUILD_ID=$(BUILD_ID)" \
		--build-arg "BUILD_VERSION=$(BUILD_VERSION)" \
		--build-arg "BASE_IMAGE=$(BASE_IMAGE)" \
		$(DOCKER_ARG_GRPC_VERSION) \
		$(DOCKER_ARG_CIVETWEB_VERSION) \
		-t $(IMAGE) -f - .
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
	@echo "--------------- $(IMAGE) -------------------------"
	@docker run --rm $(IMAGE) --version
	@echo "--------------------------------------------------------------------"

image:
	@rm -f flow-templates.[CH]
	@$(MAKE) "RUNTIMES=$(BASE_IMAGE)" "BASE_IMAGE=$(BASE_IMAGE)" flowc-image

alpine-3.18:
	@rm -f flow-templates.[CH]
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" "STACK_TRACE=0" flowc-image

ubuntu-20.04:
	@rm -f flow-templates.[CH] 
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

ubuntu-22.04:
	@rm -f flow-templates.[CH]
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

debian-bullseye:
	@rm -f flow-templates.[CH]
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

redhat-ubi8:
	@rm -f flow-templates.[CH]
	@$(MAKE) "RUNTIMES=$@" "BASE_IMAGE=$@" flowc-image

all-images: $(RUNTIMES)

recli: recli.C
	$(CXX) -std=c++17 $(CCFLAGS) "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\"" -o recli recli.C $(shell pkg-config --libs libcurl)
	@echo "====================================================================="
	@./$@ --version

vex: vex.C vex.H stru1.H veximpl.C strtrs.C vex-help.C filu.C ansi-escapes.C
	$(CXX) -std=c++17 $(CCFLAGS) "-DBUILD_ID=\"$(BUILD_ID)\"" "-DBUILD_VERSION=\"$(BUILD_VERSION)\"" -o vex $(filter-out %.H,$^) -lz
	@echo "====================================================================="
	@./$@ --version

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
