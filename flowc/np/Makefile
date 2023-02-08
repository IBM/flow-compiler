BUILD_VERSION?=0.21.2

DBG?=no
ifeq ($(DBG), yes) 
CCFLAGS?=-O0 -g -I..
else
CCFLAGS?=-O3 -I..
endif

.PHONY: clean tar image grut.help 
.PHONY: tools 

# Find a suitable tar - needed for building the image
ifeq ($(TAR), )
HOST_OS:=$(shell uname -s)
ifeq ($(HOST_OS), Darwin)
ifeq ($(shell which gnutar), )
ifeq ($(shell which gtar), )
$(warning GNU tar is needed to build the Docker image)
$(warning Either install it with 'brew install gnu-tar',)
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
endif

# lemon is needed if changes are made to the parser
LEMON?=lemon
# flex++ is needed if changes are made to the lex scanner
LEX?=flex++

IMAGE_TAG?=$(BUILD_VERSION).$(BASE_IMAGE)
IMAGE?=flowc:$(IMAGE_TAG)

RUNTIME_IMAGE?=flow-runtime:$(IMAGE_TAG)
IMAGE_NAME:=$(shell echo "$(IMAGE)" | cut -d ':' -f1)
IMAGE_TAG:=$(shell  echo "$(IMAGE)" | cut -d ':' -f2)

GRPC_INCS?=$(shell pkg-config --cflags grpc++ protobuf)
GRPC_LIBS?=$(shell pkg-config --libs-only-L protobuf) -lprotoc $(shell pkg-config --libs grpc++ protobuf)

OBJS:=gru.o grut.o grut.help.o filu.o ansi-escapes.o flowcomp.o fcast.o flow-scanner.o

%.help.C: %.help
	@echo "#include <string>" > $@
	@echo "#include <sstream>" >> $@
	@echo "#include \"filu.H\"" >> $@
	@echo "unsigned char const $(shell echo "$<" | tr -- '-./' '___')_z[] = {" >> $@
	@cat "$<" | gzip -c | xxd -i >> $@
	@echo '};' >> $@
	@echo "unsigned const $(shell echo "$<" | tr -- '-./' '___')_zlen = " >> $@
	@cat "$<" | gzip -c | wc -c >> $@
	@echo ";" >> $@
	@echo "std::string get_$(shell echo "$<" | tr -- '-./' '___')() { std::ostringstream buf; std::istringstream zbuf(std::string((char const *)$(shell echo "$<" | tr -- '-./' '___')_z, $(shell echo "$<" | tr -- '-./' '___')_zlen)); filu::gunzip(buf, zbuf, true); return buf.str();}"  >> $@
	@echo "created $@ from $<"

flowcomp.o: flow-parser.c

fcast.H: flow-parser.h

flow-parser.h: flow-parser.c
	touch -r $^ $@

flow-parser.c: flow-parser.y
	$(LEMON) $<

flow-scanner.C: flow-scanner.lex
	 flex++ -Cem -o $@  $<

%.o: %.C
	${CXX} -std=c++17 ${GRPC_INCS} $(CCFLAGS) -c -MMD -MF $*.dep -o $@ $<

grut: flow-parser.h $(OBJS)
	${CXX} -o $@ $(CCFLAGS) $(OBJS) ${GRPC_LIBS} -lz 
	@echo "====================================================================="
	@./$@ --version

scan: flow-parser.h scan.o flow-scanner.o ansi-escapes.o
	${CXX} -o $@ $(CCFLAGS) scan.o flow-scanner.o ansi-escapes.o

clean:
	rm -f grut $(OBJS:.o=.dep) $(OBJS) grut.help.C scan.o scan.dep

-include $(OBJS:.o=.dep)

