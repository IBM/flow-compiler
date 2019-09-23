#include <string>
#include <map>
char const *template_Dockerfile = R"TEMPLATE(FROM flowc AS base

USER worker
RUN mkdir -p /home/worker/{{NAME}} && mkdir -p /tmp/{{NAME}}
COPY --chown=worker:worker {{MAIN_FILE}} {P:PROTO_FILE{{{PROTO_FILE}} }P} /tmp/{{NAME}}/
RUN cd /tmp/{{NAME}} && flowc --client --server {{MAIN_FILE}} --name {{NAME}} && make -j2 -f {{NAME}}.mak deploy && cd /tmp && rm -fr {{NAME}}

WORKDIR /usr/local/lib
RUN tar -cf /home/worker/so.tar lib*.so lib*.so.* 
WORKDIR /home/worker
RUN tar -cf /home/worker/bin.tar {{NAME}}/*

FROM flow-runtime

USER root
COPY --chown=worker:worker --from=base /home/worker/*.tar /home/worker/
COPY --from=base /usr/local/bin/cosget.sh /usr/local/bin/artiget.sh /usr/local/bin/
WORKDIR /usr/local/lib
RUN tar -xvf /home/worker/so.tar && rm /home/worker/so.tar && ldconfig
WORKDIR /home/worker
RUN tar -xvf /home/worker/bin.tar && rm /home/worker/bin.tar && chown -R worker:worker {{NAME}}
USER worker
)TEMPLATE";
char const *template_Makefile = R"TEMPLATE(.PHONY: info image clean all image-info-Darwin image-info-Linux client server deploy

########################################################################
# {{NAME}}.mak 
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
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
	@echo "make -f $(THIS_FILE) IMAGE=orchestrator:0.5 image"
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

$(IMAGE_PROXY): {{MAIN_FILE}} {P:PROTO_FILE{{{PROTO_FILE}} }P}
	-docker rmi -f $(IMAGE) 2>&1 > /dev/null
	docker build --force-rm -t $(IMAGE) -f $(DOCKERFILE) .
	-docker rmi $(IMAGE_REPO):latest 2>&1 > /dev/null
	docker tag $(IMAGE) $(IMAGE_REPO):latest
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
	${CXX} -std=c++11 $(GRPC_INCS) -O3 -o $@  $<  $(PB_GENERATED_CC) $(GRPC_LIBS)

{{NAME}}-client: {{NAME}}-client.C $(PB_GENERATED_CC) $(PB_GENERATED_H) 
	${CXX} -std=c++11 $(GRPC_INCS) -O3 -o $@  $<  $(PB_GENERATED_CC) $(GRPC_LIBS)

clean:
	rm -f $(IMAGE_PROXY) {{NAME}}-server {{NAME}}-client 

all: {{NAME}}-server {{NAME}}-client 

client: {{NAME}}-client

server: {{NAME}}-server 

deploy: {{NAME}}-server {{NAME}}-client
	strip $^
	mkdir -p ~/{{NAME}}
	cp $^ ~/{{NAME}}
	cp $(wildcard *.proto) ~/{{NAME}}
	cp $(wildcard *.flow)  ~/{{NAME}}
)TEMPLATE";
char const *template_client_C = R"TEMPLATE(/************************************************************************************************************
 *
 * {{NAME}}-client.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <chrono>
#include <getopt.h>
#include <grpc++/grpc++.h>
#include <google/protobuf/util/json_util.h>

static std::string message_to_json(google::protobuf::Message const &message, bool pretty=true) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = pretty;
    options.always_print_primitive_fields = false;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    return json_reply;
}
{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}

bool ignore_json_errors = false;
bool ignore_grpc_errors = false;
bool time_calls = false;
bool use_blocking_calls = false;
bool enable_trace = false;
bool show_help = false;
int call_timeout = 0;
std::string trace_filename;

struct option long_opts[] {
    { "help",                 no_argument, nullptr, 'h' },
    { "blocking-calls",       no_argument, nullptr, 'b' },
    { "ignore-grpc-errors",   no_argument, nullptr, 'g' },
    { "ignore-json-errors",   no_argument, nullptr, 'j' },
    { "time-calls",           no_argument, nullptr, 'c' },
    { "timeout",              required_argument, nullptr, 'T' },
    { "trace",                required_argument, nullptr, 't' },
    { nullptr,                0,                 nullptr,  0 }
};

static bool parse_command_line(int &argc, char **&argv) {
    int ch;
    while((ch = getopt_long(argc, argv, "hbgjT:t:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'b': use_blocking_calls = true; break;
            case 'g': ignore_grpc_errors = true; break;
            case 'j': ignore_json_errors = true; break;
            case 'c': time_calls = true; break;
            case 'h': show_help = true; break;
            case 't': trace_filename = optarg; break;
            case 'T': 
                call_timeout = std::atoi(optarg); 
            break;
            default:
                return false;
        }
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return true;
}

enum rpc_method_id {
{I:METHOD_FULL_UPPERID{    {{METHOD_FULL_UPPERID}},
}I}
    NONE
};
static std::map<std::string, rpc_method_id> rpc_methods = {
{I:METHOD_FULL_UPPERID{    {"{{METHOD_FULL_NAME}}", {{METHOD_FULL_UPPERID}}},
}I}
};
struct arg_field_descriptor { 
    int next; 
    char type; 
    char const *name; 
};
int help(std::string const &name, arg_field_descriptor const *d, int p = 0, int indent = 0) {
    if(indent == 0) std::cerr << "gRPC method " << name << " {\n";
    indent += 4;
    while(d[p].type) {
        std::cerr << std::string(indent, ' ') << d[p].name << ": ";
        switch(tolower(d[p].type)) {
            case 'm': 
                std::cerr << "{\n";
                indent += 4;
                break;
            case 'n':
                std::cerr << " NUMBER\n";
                break;
            case 'e':
                std::cerr << " ENUM\n";
                break;
            case 's':
                std::cerr << " STRING\n";
                break;
            default:
                break;
        }
        if(d[p].next == 0) {
            indent -= 4;
            std::cerr << std::string(indent, ' ') << "}\n";
        }
        ++p;
    }
    std::cerr << "}\n";
    return p;
}
/*
        bool Trace_call = Get_metadata_value(Client_metadata, "trace-call");
        bool Time_call = Get_metadata_value(Client_metadata, "time-call");
        bool Async_call = !Get_metadata_value(Client_metadata, "blocking-calls", !Async_Flag);
        */
{I:METHOD_FULL_UPPERID{/****** {{METHOD_FULL_NAME}}
 */
static int file_{{METHOD_FULL_ID}}(std::string const &label, std::istream &ins, std::unique_ptr<{{SERVICE_NAME}}::Stub> const &stub) {
    {{SERVICE_INPUT_TYPE}} input;
    {{SERVICE_OUTPUT_TYPE}} output;
    std::string input_line;
    unsigned line_count = 0;
    while(std::getline(ins, input_line)) {
        ++line_count;
        auto b = input_line.find_first_not_of("\t\r\a\b\v\f ");
        if(b == std::string::npos || input_line[b] == '#') {
            // Empty or comment line in the input. Reflect it as is in the output.
            std::cout << input_line << "\n";
            continue;
        }
        input.Clear(); 
        auto conv_status = google::protobuf::util::JsonStringToMessage(input_line, &input);
        if(!conv_status.ok()) {
            std::cerr << label << "(" << line_count << "): " << conv_status.ToString() << std::endl;
            if(!ignore_json_errors) 
                return 1;
            // Ouput a new line to keep the input and the output files in sync
            std::cout << "\n";
        }
        output.Clear();
        grpc::ClientContext context;
        if(use_blocking_calls) context.AddMetadata("blocking-call", "1");
        if(time_calls) context.AddMetadata("time-call", "1");
        //if(trace_calls) context.AddMetadata("trace-call", "1");
        if(call_timeout > 0) {
            auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(call_timeout);
            context.set_deadline(deadline);
        }
        grpc::Status status = stub->{{METHOD_NAME}}(&context, input, &output);
        if(!status.ok()) {
            std::cerr << label << "(" << line_count << ") from " << context.peer() << ": " << status.error_code() << ": " << status.error_message() << "\n" << status.error_details() << std::endl;
            if(!ignore_grpc_errors)
                return 1;
            // Otput a comment line with the error message to keep the input and output file in sync
            std::cout << "# " << status.error_code() << ": " << status.error_message() << "\n";
        }
        std::cout << message_to_json(output, false)  << "\n";
    }
    return 0;
}
static arg_field_descriptor const {{METHOD_FULL_ID}}_di_{{SERVICE_INPUT_ID}}[] { 
    {{SERVICE_INPUT_DESC}}, 
    { 0, '\0', "" }
};
}I}
static rpc_method_id get_rpc_method_id(std::string const &name) {
    rpc_method_id id = NONE;
    std::vector<std::string> matches;
    for(auto const &rme: rpc_methods) 
        if(rme.first == name || (rme.first.length() > name.length() + 1 && rme.first.substr(rme.first.length()-name.length()-1) == std::string(".")+name)) {
            matches.push_back(rme.first); 
            id = rme.second;
        }
    switch(matches.size()) {
        case 0:
            std::cerr << "Unknown method \"" << name << "\"\n";
        case 1: 
            return id;
        default:
            std::cerr << "Ambiguous method name \"" << name << "\", matches ";
            for(unsigned u = 0; u+2 < matches.size(); ++u) std::cerr << "\"" << name << "\", ";
            std::cerr << "\"" << matches[matches.size()-2] << "\" and \"" << matches[matches.size()-1] << "\"\n";
            return NONE;
    }
}
int main(int argc, char *argv[]) {
    bool show_method_help = argc == 3 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
    if(!show_method_help && (!parse_command_line(argc, argv) || argc != 4 || show_help)) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] <PORT|ENDPOINT> <[SERVICE.]RPC> <JSON-INPUT-FILE>\n";
        std::cerr << "    or " << argv[0] << " --help [[SERVIVCE.]RPC]\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "      --blocking-calls        Disable asynchronous calls in the aggregator\n";
        std::cerr << "      --ignore-grpc-errors    Keep going when grpc errors are encountered\n";
        std::cerr << "      --ignore-json-errors    Keep going even if json conversion fails for a request\n";
        std::cerr << "      --time-calls            Retrieve timing information for each call\n";
        std::cerr << "      --timeout MILLISECONDS  Limit each call to the given amount of time\n";
        std::cerr << "      --trace FILE, -t FILE   Enable the trace flag and output the trace to FILE\n";
        std::cerr << "      --help                  Display this screen and exit\n";
        std::cerr << "\n";
        std::cerr << "gRPC Methods:\n";
        for(auto mid: rpc_methods) 
            std::cerr << "\t" << mid.first << "\n";
        std::cerr << "\n";
        return show_help? 1: 0;
    }
    rpc_method_id mid = get_rpc_method_id(argv[2]);
    if(mid == NONE) return 1;
    if(show_method_help) {
        switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: 
                help("{{METHOD_FULL_NAME}}", {{METHOD_FULL_ID}}_di_{{SERVICE_INPUT_ID}});
                break;
}I}
            case NONE:
                break;
        }
        return 0;
    }
    std::string endpoint(strchr(argv[1], ':') == nullptr? std::string("localhost:")+argv[1]: std::string(argv[1]));
    std::shared_ptr<grpc::Channel> channel(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
    std::istream *in = nullptr;
    std::ifstream infs;
    std::string input_label;
    if(strcmp("-", argv[3]) == 0) {
        in = &std::cin;
        input_label = "<stdin>";
    } else {
        infs.open(argv[3]);
        if(!infs.is_open()) {
            std::cerr << "Could not open file: " << argv[3] << "\n";
            return 1;
        }
        in = &infs;
        input_label = argv[3];
    }
    int rc = 1;
    switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: {
            std::cerr << "method: {{METHOD_FULL_NAME}}\n";
            std::unique_ptr<{{SERVICE_NAME}}::Stub> client_stub({{SERVICE_NAME}}::NewStub(channel));
            rc = file_{{METHOD_FULL_ID}}(input_label, *in, client_stub);
        } break;
}I}
        case NONE:
            break;
    }
    return rc;
}

)TEMPLATE";
char const *template_docker_compose_sh = R"TEMPLATE(#!/bin/bash

##################################################################################
# {{NAME}}-dc.sh
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
{O:VOLUME_NAME_VAR{export flow_{{VOLUME_NAME_VAR}}={{VOLUME_LOCAL}}
}O}
docker_COMPOSE_PROJECT_NAME={{NAME}}
export trace_ENABLED=0
export provision_ENABLED=1
{R:REST_NODE_NAME{
# protorest mount path (original path: {{PROTO_FILES_PATH}})
case "$0" in
    /*) 
        export local_PROTO_FILES_PATH="$(dirname "$0")"
        ;;
    *)
        export local_PROTO_FILES_PATH="$PWD/$(dirname "$0")"
        ;;
esac
if [ -d "$local_PROTO_FILES_PATH/htdocs" ]
then
    export {{NAME_UPPERID}}_HTDOCS="$local_PROTO_FILES_PATH/htdocs"
fi
export rest_PORT=${{{NAME_UPPERID}}_REST_PORT-{{REST_NODE_PORT}}}
export gui_PORT=${{{NAME_UPPERID}}_REST_PORT-{{GUI_NODE_PORT}}}
export app_PORT=${{{NAME_UPPERID}}_REST_PORT-{{CUSTOM_GUI_NODE_PORT}}}
{P:PROTO_FILE{if [ ! -f "$local_PROTO_FILES_PATH/{{PROTO_FILE}}" ]
then
    echo "proto file \"$local_PROTO_FILES_PATH/{{PROTO_FILE}}\" is missing, cannot continue"
    exit 1
fi
}P}
}R}
export grpc_PORT=${{{NAME_UPPERID}}_GRPC_PORT-{{MAIN_PORT}}}
# default to running in the foreground
fg_OR_bg=
export export_PORTS="#"
args=()
while [ $# -gt 0 ]
do
case "$1" in
{A:VOLUME_OPTION{
    {{VOLUME_OPTION}})
    export {{VOLUME_NAME_VAR}}="$2"
    shift
    shift
    ;;
}A}
    --htdocs)
    if [ "${2:0:1}" == "/" ] 
    then
        export {{NAME_UPPERID}}_HTDOCS="$2"
    else 
        export {{NAME_UPPERID}}_HTDOCS="$PWD/${2#./}"
    fi
    shift
    shift
    ;;
    --app-port)
    export app_PORT="$2"
    shift
    shift
    ;;
    --grpc-port)
    export grpc_PORT="$2"
    shift
    shift
    ;;
    --gui-port)
    export gui_PORT="$2"
    shift
    shift
    ;;
    --rest-port)
    export rest_PORT="$2"
    shift
    shift
    ;;
    -s|--skip-provision)
    export provision_ENABLED=0
    shift
    ;;
    -t|--enable-trace)
    export trace_ENABLED=1
    shift
    ;;
    -p|--export-ports)
    export export_PORTS=
    shift
    ;;
    -b)
    fg_OR_bg=-d
    shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

docker_COMPOSE_YAML=$(cat <<"ENDOFYAML"
{{DOCKER_COMPOSE_YAML}}
ENDOFYAML
)

{O:VOLUME_NAME_VAR{export {{VOLUME_NAME_VAR}}="${{{VOLUME_NAME_VAR}}-$flow_{{VOLUME_NAME_VAR}}}"
}O}
[ {O:VOLUME_NAME_VAR{-z "${{VOLUME_NAME_VAR}}" -o }O} 1 -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
export enable_custom_app="#"
if [ ! -z "${{NAME_UPPERID}}_HTDOCS" ]
then
    if [ ! -d "${{NAME_UPPERID}}_HTDOCS" ]
    then
        echo "${{NAME_UPPERID}}_HTDOCS: must point to a valid directory"
        have_ALL_VOLUME_DIRECTORIES=0
    else
        export enable_custom_app=
    fi
fi

if [ $# -eq 0 -o "$1" == "up" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 -o "$1" == "provision" -a $have_ALL_VOLUME_DIRECTORIES -eq 0  -o "$1" == "config" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 -o "$1" != "up" -a "$1" != "down" -a "$1" != "config" -a "$1" != "logs" -a "$1" != "provision" ]
then
echo "{{NAME}}-dc.sh generated from {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $0 <up|config|provision> [-b] [-t] [--xxx-port PORT] {R:REST_NODE_NAME{--htdocs DIRECTORY }R}{O:VOLUME_OPTION{{{VOLUME_OPTION}} DIRECTORY  }O}"
echo "   or $0 <down|logs>"
echo ""
echo "    -b  run docker compose in the background"
echo ""
echo "    -p, --export-ports  export the gRPC ports for all nodes"
echo ""
echo "    -s, --skip-provision  skip checking for new data files at startup"
echo ""
echo "    -t, --enable-trace  enable orchestrator call tracing"
echo ""
echo   "    --grpc-port PORT  (or set {{NAME_UPPERID}}_GRPC_PORT)"
echo   "        Override GRPC port (default is $grpc_PORT)"
echo ""
{R:REST_NODE_NAME{
echo   "    --rest-port PORT  (or set {{NAME_UPPERID}}_REST_PORT)"
echo   "        Override REST API port (default is $rest_PORT)"
echo ""
echo   "    --gui-port PORT  (or set {{NAME_UPPERID}}_GUI_PORT)"
echo   "        Override REST API port (default is $gui_PORT)"
echo ""
echo   "    --app-port PORT  (or set {{NAME_UPPERID}}_APP_PORT)"
echo   "        Override REST API port (default is $app_PORT)"
echo ""
echo   "    --htdocs DIRECTORY  (or set {{NAME_UPPERID}}_HTDOCS)"
echo   "        Directory with custom application files (default is \"${{NAME_UPPERID}}_HTDOCS\")"
echo ""
}R}
{O:VOLUME_OPTION{
echo   "    {{VOLUME_OPTION:}} DIRECTORY  (or set {{VOLUME_NAME_VAR}})"
echo   "        Override path to be mounted for {{VOLUME_NAME}} (default is $flow_{{VOLUME_NAME_VAR}})"
printf "        "{{VOLUME_HELP}}"\n"
echo ""
}O}
{A:HAVE_ARTIFACTORY{
{{HAVE_ARTIFACTORY}}
echo "Note: To access the Artifactory $0 looks for the environment variable API_KEY."
echo "If not found, it looks for a file named .api-key in the current directory and then in the home directory."
echo ""
}A}
exit 1
fi
{A:HAVE_ARTIFACTORY{
{{HAVE_ARTIFACTORY}}
{O:VOLUME_OPTION{artifactory_{{VOLUME_NAME_VAR}}="{{VOLUME_ARTIFACTORY}}"   
}O}
}A}
case "$1" in
    provision)
        rc=0
{A:HAVE_ARTIFACTORY{
        {{HAVE_ARTIFACTORY}}
{O:VOLUME_OPTION{
        if [ ! -z "$artifactory_{{VOLUME_NAME_VAR}}" ]
        then
            "$(dirname "$0")/{{NAME}}-ag.sh" -o "${{VOLUME_NAME_VAR}}" --untar "$artifactory_{{VOLUME_NAME_VAR}}"
            rc=$?
        fi
        [ $rc -eq 0 ] || exit $rc
}O}
}A}
        exit $rc
        ;;
    config)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" config
        exit $?
        ;;
    down)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" down -v
        exit $?
        ;;
    logs)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" logs -f
        exit $?
        ;;
    up)
        rc=0
{A:HAVE_ARTIFACTORY{
        if [ $provision_ENABLED -ne 0 ]{{HAVE_ARTIFACTORY}}
        then
{O:VOLUME_OPTION{
            if [ ! -z "$artifactory_{{VOLUME_NAME_VAR}}" ]
            then
                "$(dirname "$0")/{{NAME}}-ag.sh" -o "${{VOLUME_NAME_VAR}}" --untar "$artifactory_{{VOLUME_NAME_VAR}}"
                rc=$?
            fi
            [ $rc -eq 0 ] || exit $rc
}O}
        fi
}A}
        if [ $rc -eq 0 ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" up $fg_OR_bg
            rc=$?
        fi
        ;;
esac
if [ $rc -eq 0 -a "$fg_OR_bg" != "" ]
then
    echo "{{NAME}} gRPC service listening on port $grpc_PORT"
    {R:REST_NODE_NAME{
    echo "{{NAME}} REST service listening on port $rest_PORT"
    echo "{{NAME}} GUI available on port $gui_PORT"
    if [ -z "$enable_custom_app" ]
    then
        echo "{{NAME}} App available on port $app_PORT"
    fi
}R}
fi
exit $rc

)TEMPLATE";
char const *template_docker_compose_yaml = R"TEMPLATE(version: '3'
services:
  {{NAME}}:
    environment: [ "{{NAME_UPPERID}}_DEBUG=1", "{{NAME_UPPERID}}_TRACE=$trace_ENABLED", "{{NAME_UPPERID}}_ASYNC=1", {{ORCHESTRATOR_ENVIRONMENT}} ]
    image: "{{PUSH_REPO:}}{{IMAGE}}"
    ports: ["$grpc_PORT:{{MAIN_PORT}}"]
    command: ["/home/worker/{{NAME}}/{{NAME}}-server", "{{MAIN_PORT}}"]
{N:NODE_NAME{
{{EXTERN_NODE}}  {{NODE_NAME}}:
{{EXTERN_NODE}}    image: "{{NODE_IMAGE}}"
{{EXTERN_NODE}}$export_PORTS    ports: ["{{NODE_PORT}}:{{IMAGE_PORT}}"]
{{EXTERN_NODE}}    {{NODE_ENVIRONMENT:}}
{{EXTERN_NODE}}    {{NODE_MOUNTS:}}
}N}
{R:REST_NODE_NAME{
  {{REST_NODE_NAME}}:
    image: "{{REST_NODE_IMAGE}}"
    ports: 
    - $rest_PORT:{{REST_IMAGE_PORT}}/tcp
    - $gui_PORT:{{GUI_IMAGE_PORT}}/tcp 
$enable_custom_app    - $app_PORT:{{CUSTOM_GUI_IMAGE_PORT}}/tcp
    volumes: 
    - $local_PROTO_FILES_PATH:/home/worker/pr/input:ro
$enable_custom_app    - ${{NAME_UPPERID}}_HTDOCS:/home/worker/pr/htdocs:ro
    command: ["/home/worker/pr/bin/runpr3.sh", "--name", "{{NAME_UPPER}}", "--description", {{MAIN_DESCRIPTION}}, "--hostport", "{{NAME}}:{{MAIN_PORT}}"{E:REST_ENTRY{, "--service", "{{REST_ENTRY}}"}E}]
}R}
{V:HAVE_VOLUMES{
volumes:{{HAVE_VOLUMES}}
}V}
{W:VOLUME_NAME{
{{VOLUME_COMMENT:}}
    {{VOLUME_NAME}}:
        driver_opts:
            type: none
            o: bind
            device: ${{{VOLUME_NAME_VAR}}}
}W}
)TEMPLATE";
char const *template_flow_logger_proto = R"TEMPLATE(syntax = "proto3";

message FlowCallInfo {
    string callId=1;
};
message FlowLogMessage {
    string message=1;
};

service FlowService {
    rpc log(FlowCallInfo) returns(stream FlowLogMessage) {}
}
)TEMPLATE";
char const *template_help = R"TEMPLATE(flowc - compiler for gRPC microservice aggregation

USAGE  
       flowc [options] inputfile.flow 

DESCRIPION

       flowc compiles a control flow graph representation of an applicatiom where all the nodes are 
       "gRPC" calls. The compilation is done in several stages depending on the target type selected:
      
       "Protocol Buffers" 
              protoc, the protocol buffer compiler is invoked to compile the message types used in 
              the "gRPC" calls. At this stage "C++" source files may be generated if the selected 
              targets require them.

       "C++" code generation
              "C++" code is generated for the aggregator service and for an optional client. Along
              with the "C++" source code, a "makefile" and a "dockerfile" are also generated.

       Deployment tool generation
              Deployment configuration files and tools are generated for "Docker Compose" and for
              "Kubernetes"
OPTIONS

       --base-port=PORT, -P PORT
              Change the port used for the aggregator "gRPC" service. For "Docker Compose" port 
              numbers starting from this value are allocated for the each container. 

       --build-image, -i
              Generate code for the "gRPC" aggregator and invoke docker to build the application 
              image. See --image and --image-tag for related options.

       --build-client, -c
              Generate code for a client that can be used in debugging and testing the application,
              and invoke the "C++" compiler to build it

       --build-server, -s
              Generate code for the aggregator "gRPC" server and invoke the "C++" compiler to build it

       --client
              Generate code for a client that can be used in debugging and testing the application

       --color=YES/NO
              Force or disable ANSI coloring regardless of wheter output is to a terminal or not.
              By default coloring is enabled only when output is to a terminal.

       --default-client-timeout=TIME
              Timeout for "gRPC" calls made to any of the nodes. The default unit is seconds. This value is
              used unless a different timeout is specified for the node.

       --default-client-calls=NUMBER
              Number of concurent calls that can be ia synchronously made to a "gRPC" service. This value is 
              used when replicas are not set for a given node.

       --default-entry-timeout=TIME
              Default timeout for all the "gRPC" entries (servce methods.) If processing of any method exeeds
              this timeout the method will reply with "DEADLINE_EXCEEDED". The timeout is used when no value
              was set for a given entry.

       --docker-compose, -d
              Generate a "Docker Compose" driver tool for managing the application

       --dockerfile, -D
              Generate a "dockerfile" for the aggregator image

       --entry=NAME, -e NAME
              Generate code for this entry only. Can be repeated. By default code is genrated for all entries
              define in the flow.

       --grpc-files, -g
              Generate ".cc" and ".h" files for both "gRPC" services and "Protocol Buffers" messages.
              See --protobuf-file for generating code for messages only.

       --help, -h
              Print this screen and exit

       --help-syntax
              Display a page with a description of the flow syntax

       --image=IMAGE:TAG
              Set the image and tag for the aggregator image. The default image name is the 
              aggregator name and the default tag is "1". See --image-tag for changing only the tag.

       --image-pull-secret=NAME
              Add NAME to the "imagePullSecrets" section of the "kubernetes" deployments

       --image-tag=TAG
              Change the image tag only for the docker image. This option is ignored when --image 
              is set.

       --input-label=NAME
              Change the name of the input special node. The default is "input".

       --proto-path=PATH, -I PATH, --proto_path=PATH
              Specify the directory in which to search for the main file and for imports. May be specified
              multiple times. Directories will be searched in order. If not given, the current working 
              directory is used.

       --kubernetes, -k
              Generate configuration tool for "Kubernetes" deployment

       --makefile, -m
              Generate a "makefile" that can be used to make the client, the server and the docker image

       --name=IDENTIFIER, -n IDENTIFIER
              Identifier to base the output file names on. Defaults to the filename stripped of 
              the ".flow" suffix.

       --output-directory=DIRECTORY, -o DIRECTORY
              Write all the output files in DIRECTORY

       --push-repository=REMOTE-REPO-PREFIX, -p REMOTE-REPO-PREFIX
              Push the aggregator image to the specified remote repository. If configuration files are
              generated they will reference the remote repository rather than the local one.

       --protobuf-files
              Generate ".cc" and ".h" files for the "Protocol Buffers" messages using protoc. 
              See --grpc-file for generating code for both messages and services.

       --print-ast
              Print the "Abstract Syntax Tree" generated by the flow parser

       --print-pseudocode
              Print the intermediate code generated by the flow compiler

       --print-graph=ENTRY
              Print the flow graph in "dot" format. The "dot" utility can be used to render the graph.
              For example, to generate a grph in ".svg" format, pipe the output through "dot -Tsvg".

       --python-client
              Generate "Python" code for a client that can be used in debugging and testing the application

       --rest-image=IMAGE[:TAG] -R IMAGE[:TAG]
              Add a "REST" gateway to the configuration. Set to an empty string to disable "REST".

       --rest-port=PORT, -r PORT
              Make "REST" available on "PORT". Must not conflict with the aggregator "gRPC" port.
              On "Kubernetes" this port must not conflict with any of the ports used in the main pod.

       --server
              Generate code for the "gRPC" aggregator

       --single-pod
              Ignore all pod labels and generate a single pod deployment with all the nodes.
              This is a "Kubernetes" specific option.

       --trace 
              Very verbose compiler trace

       --version 
              Display the version and exit
)TEMPLATE";
char const *template_kubernetes_group_yaml = R"TEMPLATE(---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{NAME}}-{{G_NODE_GROUP}}
  labels:
    app: {{NAME}}-{{G_NODE_GROUP}}
    flow-group: {{NAME}}
spec:
  replicas: ${replicas_{{NAME_UPPERID}}_{{G_NODE_GROUP_UPPER}}}
  selector:
    matchLabels:
      app: {{NAME}}-{{G_NODE_GROUP}}
  template:
    metadata:
      labels:
        app: {{NAME}}-{{G_NODE_GROUP}}
        flow-group: {{NAME}}
    spec:
{X:HAVE_IMAGE_PULL_SECRETS{
      imagePullSecrets:{{HAVE_IMAGE_PULL_SECRETS}}}X}
{S:IMAGE_PULL_SECRET{
      - name: "{{IMAGE_PULL_SECRET}}"
}S}
{X:HAVE_INIT_CONTAINERS{
      initContainers:{{HAVE_INIT_CONTAINERS}}}X}
{I:G_INIT_CONTAINER{
      - {{G_INIT_CONTAINER}}
}I}
{W:VOLUME_NAME{
${{{VOLUME_UPPERID}}_ART}      - command: ["/bin/sh", "-c", "/usr/local/bin/artiget.sh '${{{VOLUME_UPPERID}}}' --api-key-file /etc/artifactory/api-key -u -o /to-1"]
${{{VOLUME_UPPERID}}_COS}      - command: ["/bin/sh", "-c", "/usr/local/bin/cosget.sh '${{{VOLUME_UPPERID}}}' -d /etc/cos-access -u -o /to-1"]
${{{VOLUME_UPPERID}}_PVC}      - command: ["/bin/sh", "-c", "rsync -vrtlog /from-1/. /to-1"] 
        image:  "{{PUSH_REPO:}}{{IMAGE}}" 
        securityContext: {privileged: true, runAsUser: 0} 
        name: volumes-init-{{VOLUME_OPTION}}
        volumeMounts:
${{{VOLUME_UPPERID}}_PVC}        - name: {{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_PVC}          mountPath: "/from-1"
${{{VOLUME_UPPERID}}_PVC}          readOnly: true
${{{VOLUME_UPPERID}}_COS}        - name: cos-access-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_COS}          mountPath: "/etc/cos-access"
${{{VOLUME_UPPERID}}_COS}          readOnly: true
${{{VOLUME_UPPERID}}_ART}        - name: api-key-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_ART}          mountPath: "/etc/artifactory"
${{{VOLUME_UPPERID}}_ART}          readOnly: true

        - name: scratch-{{VOLUME_OPTION}}
          mountPath: "/to-1"
          readOnly: false
}W}
      containers:
{N:G_NODE_NAME{
      - name: {{G_NODE_OPTION}}
        image: "{{G_NODE_IMAGE}}"
        {{G_NODE_ENVIRONMENT}}
        {{G_NODE_MOUNTS}}
        {{G_NODE_LIMITS}}
}N}
{V:HAVE_VOLUMES{
      volumes:{{HAVE_VOLUMES}}
}V}
{W:VOLUME_NAME{
${{{VOLUME_UPPERID}}_PVC}        - name: {{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_PVC}          persistentVolumeClaim:
${{{VOLUME_UPPERID}}_PVC}            claimName: ${{{VOLUME_UPPERID}}}
${{{VOLUME_UPPERID}}_COS}        - name: cos-access-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_COS}          secret:
${{{VOLUME_UPPERID}}_COS}            secretName: ${{{VOLUME_UPPERID}}_SECRET_NAME}
${{{VOLUME_UPPERID}}_ART}        - name: api-key-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_ART}          secret:
${{{VOLUME_UPPERID}}_ART}            secretName: ${{{VOLUME_UPPERID}}_SECRET_NAME}
        - name: scratch-{{VOLUME_OPTION}}
          emptyDir: {}
}W}
---
apiVersion: v1
kind: Service
metadata:
  name: {{NAME}}-{{G_NODE_GROUP}}
  labels:
    app: {{NAME}}-{{G_NODE_GROUP}}
    flow-group: {{NAME}}
spec:
  ports:
{N:G_NODE_NAME{
  - port: {{G_IMAGE_PORT}}
    protocol: TCP
    name: {{G_NODE_OPTION}}-port
}N}
  selector:
    app: {{NAME}}-{{G_NODE_GROUP}}
)TEMPLATE";
char const *template_kubernetes_sh = R"TEMPLATE(#!/bin/bash

##################################################################################
# {{NAME}}-kube.sh
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
{A:VOLUME_UPPERID{flow_{{VOLUME_UPPERID}}="${{{VOLUME_UPPERID}}-{{VOLUME_COS}}}"
if [ -z "$flow_{{VOLUME_UPPERID}}" ]
then
    flow_{{VOLUME_UPPERID}}="{{VOLUME_PVC}}"
fi
export {{VOLUME_UPPERID}}="$flow_{{VOLUME_UPPERID}}"
flow_{{VOLUME_UPPERID}}_SECRET_NAME=${{{VOLUME_UPPERID}}_SECRET_NAME-{{VOLUME_SECRET}}}
export {{VOLUME_UPPERID}}_SECRET_NAME=$flow_{{VOLUME_UPPERID}}_SECRET_NAME
}A}
cur_KUBECTL=${KUBECTL-kubectl}
kube_PROJECT_NAME={{NAME}}
export replicas_{{NAME_UPPERID}}=${{{NAME_UPPERID}}_REPLICAS-1}
{G:GROUP_UPPER{export replicas_{{NAME_UPPERID}}_{{GROUP_UPPERID}}=${{{NAME_UPPERID}}_{{GROUP_UPPERID}}_REPLICAS-1}
}G}
kubernetes_YAML=$(cat <<"ENDOFYAML"
{{KUBERNETES_YAML}}
ENDOFYAML
)
args=()
while [ $# -gt 0 ]
do
case "$1" in
{A:VOLUME_OPTION{
    {{VOLUME_OPTION}})
    export {{VOLUME_UPPERID}}="$2"
    shift
    shift
    ;;
    {{VOLUME_OPTION}}-secret-name)
    export {{VOLUME_UPPERID}}_SECRET_NAME="$2"
    shift
    shift
    ;;
}A}
    --htdocs)
    export {{NAME_UPPERID}}_HTDOCS="$2"
    shift
    shift
    ;;
    --htdocs-secret-name)
    export {{NAME_UPPERID}}_HTDOCS_SECRET_NAME="$2"
    shift
    shift
    ;;
    --{{NAME}}-replicas)
    export replicas_{{NAME_UPPERID}}="$2"
    shift
    shift
    ;;
{G:GROUP_UPPER{
    --{{GROUP}}-replicas)
    export replicas_{{NAME_UPPERID}}_{{GROUP_UPPERID}}="$2"
    shift
    shift
    ;;
}G}
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

{A:VOLUME_UPPERID{
case "$(echo "${{VOLUME_UPPERID}}" | tr '[:upper:]' '[:lower:]')" in
    s3://*|http://|https://)
        export {{VOLUME_UPPERID}}_PVC="#"
        export {{VOLUME_UPPERID}}_COS=
        ;;
    *)
        export {{VOLUME_UPPERID}}_PVC=
        export {{VOLUME_UPPERID}}_COS="#"
        ;;
esac
}A}

case "$(echo "${{NAME_UPPERID}}_HTDOCS" | tr '[:upper:]' '[:lower:]')" in
    s3://*|http://|https://)
        export htdocs_PVC="#"
        export htdocs_COS=
        ;;
    *)
        export htdocs_PVC=
        export htdocs_COS="#"
        ;;
esac

have_ALL_VOLUME_CLAIMS=1

export enable_htdocs="#"
if [ ! -z "${{NAME_UPPERID}}_HTDOCS" ]
then
    export enable_htdocs=""
    if [ -z "$htdocs_COS" -a -z "${{NAME_UPPERID}}_HTDOCS_SECRET_NAME" ]
    then 
        have_ALL_VOLUME_CLAIMS=0
        echo "Please set the secret name for accessing ${{NAME_UPPERID}}_HTDOCS"
    fi
fi


{O:VOLUME_UPPERID{
if [ -z "${{VOLUME_UPPERID}}" ] 
then
    have_ALL_VOLUME_CLAIMS=0
    echo "Please set information for {{VOLUME_OPTION}}"
else
    if [ -z "${{VOLUME_UPPERID}}_PVC" -a -z "${{VOLUME_UPPERID}}_SECRET_NAME" ]
    then
        have_ALL_VOLUME_CLAIMS=0
        echo "Please set the secret name for accessing ${{VOLUME_UPPERID}}"
    fi
fi
}O}

if [ $# -eq 0 -o "$1" == "deploy" -a $have_ALL_VOLUME_CLAIMS -eq 0 -o "$1" == "config" -a $have_ALL_VOLUME_CLAIMS -eq 0 -o "$1" != "deploy" -a "$1" != "delete" -a "$1" != "config" -a "$1" != "show" ]
then
echo "$0 generated from {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $0 <deploy|config> [OPTIONS]"
# {O:VOLUME_OPTION{{{VOLUME_OPTION}} <VOLUME-CLAIM-NAME> }O} --{{NAME}}-replicas <NUM> {G:GROUP_UPPER{--{{GROUP}}-replicas <NUM> }G}
echo "   or $0 <show|delete>"
echo ""
echo "OPTIONS"
{O:VOLUME_OPTION{
    echo   "    {{VOLUME_OPTION:}} <VOLUME-CLAIM-LABEL|CLOUD-OBJECT-STORE-URL>  (or set \${{VOLUME_UPPERID}})"
    echo   "        Default is \"${{VOLUME_UPPERID}}\""
    printf "        "{{VOLUME_HELP}}"\n"
    echo ""
    echo   "    {{VOLUME_OPTION:}}-secret-name <SECRET-NAME>  (or set \${{VOLUME_UPPERID}}_SECRET_NAME)"
    printf "        Secret name for COS access for {{VOLUME_OPTION}}, default is \"${{VOLUME_UPPERID}}_SECRET_NAME\"\n"
    echo ""
}O}
    echo   "    --htdocs <VOLUME-CLAIM-LABEL|CLOUD-OBJECT-STORE-URL>  (or set \${{NAME_UPPERID}}_HTDOCS)"
    echo   "        Default is \"${{NAME_UPPERID}}_HTDOCS\""
    echo   "        Volume or COS URL for tarball with custom application files"
    echo ""
    echo   "    --htdocs-secret-name <SECRET-NAME>  (or set \${{NAME_UPPERID}}_HTDOCS_SECRET_NAME)"
    printf "        Secret name for COS access for the custom application files, default is \"${{NAME_UPPERID}}_HTDOCS_SECRET_NAME\"\n"
    echo ""
    echo   "    --{{NAME}}-replicas <NUMBER>  (or set \${{NAME_UPPERID}}_REPLICAS)"
    echo   "        Number of replicas for the main pod [{{MAIN_GROUP_NODES}}] default is $replicas_{{NAME_UPPERID}}"
    echo ""

{G:GROUP_UPPER{
    echo   "    --{{GROUP}}-replicas <NUMBER>  (or set \${{NAME_UPPERID}}_{{GROUP_UPPERID}}_REPLICAS)"
    echo   "        Number or replicas for pod \"{{GROUP}}\" [{{GROUP_NODES}}] default is $replicas_{{NAME_UPPERID}}_{{GROUP_UPPERID}}"
    echo ""
}G}
echo "This script uses '$(which $cur_KUBECTL)'. Set \$KUBECTL to override."
echo ""
echo "Note: kubectl port-forward service/{{NAME}} --address 0.0.0.0 8443:8081"
echo ""
echo "Note: secret template for COS access:"
echo ""
echo "apiVersion: v1"
echo "kind: Secret"
echo "metadata:"
echo "  name: cos-access"
echo "data:"
echo "  SECRET_ACCESS_KEY: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
echo "  ACCESS_KEY_ID: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
echo ""
exit 1
fi
if [ "$1" == "config" ]
then
echo "$kubernetes_YAML" | envsubst | sed '/^ *$/d'
exit 0
fi
if [ "$1" == "deploy" ]
then
echo "$kubernetes_YAML" | envsubst | $cur_KUBECTL apply -f -
exit $?
fi
if [ "$1" == "show" ]
then
$cur_KUBECTL get po -l "flow-group={{NAME}}" &&\
$cur_KUBECTL get service -l "flow-group={{NAME}}"
exit $?
fi
if [ "$1" == "delete" ]
then
    $cur_KUBECTL delete service {{NAME}}
    {S:GROUP{$cur_KUBECTL delete service {{NAME}}-{{GROUP}}
}S}
    $cur_KUBECTL delete deploy {{NAME}}-{{MAIN_POD}}
    {G:GROUP{$cur_KUBECTL delete deploy {{NAME}}-{{GROUP}}
}G}
    {R:REST_NODE_NAME{$cur_KUBECTL delete configmap {{NAME}}-protos}R}
## ignore errors
exit 0
fi
)TEMPLATE";
char const *template_kubernetes_yaml = R"TEMPLATE({R:REST_NODE_NAME{
apiVersion: v1
data:
{P:PROTO_FILE{
  {{PROTO_FILE}}: {{PROTO_FILE_YAMLSTR}}
}P}
{E:REST_ENTRY{
  {{REST_ENTRY}}.svg: {{ENTRY_SVG_YAMLSTR}}
}E}
kind: ConfigMap
metadata:
  name: {{NAME}}-protos
---
}R}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{NAME}}-{{MAIN_POD}}
  labels:
    app: {{NAME}}
    flow-group: {{NAME}}
spec:
  replicas: ${replicas_{{NAME_UPPERID}}}
  selector:
    matchLabels:
      app: {{NAME}}
  template:
    metadata:
      labels:
        app: {{NAME}}
        flow-group: {{NAME}}
    spec:
{X:HAVE_IMAGE_PULL_SECRETS{
      imagePullSecrets:{{HAVE_IMAGE_PULL_SECRETS}}}X}
{S:IMAGE_PULL_SECRET{
      - name: "{{IMAGE_PULL_SECRET}}"
}S}
      initContainers:
{I:G_INIT_CONTAINER{
      - {{G_INIT_CONTAINER}}
}I}
{W:VOLUME_NAME{
${{{VOLUME_UPPERID}}_ART}      - command: ["/bin/sh", "-c", "/usr/local/bin/artiget.sh '${{{VOLUME_UPPERID}}}' --api-key-file /etc/artifactory/api-key -u -o /to-1"]
${{{VOLUME_UPPERID}}_COS}      - command: ["/bin/sh", "-c", "/usr/local/bin/cosget.sh '${{{VOLUME_UPPERID}}}' -d /etc/cos-access -u -o /to-1"]
${{{VOLUME_UPPERID}}_PVC}      - command: ["/bin/sh", "-c", "rsync -vrtlog /from-1/. /to-1"] 
        image:  "{{PUSH_REPO:}}{{IMAGE}}" 
        securityContext: {privileged: true, runAsUser: 0} 
        name: volumes-init-{{VOLUME_OPTION}}
        volumeMounts:
${{{VOLUME_UPPERID}}_PVC}        - name: {{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_PVC}          mountPath: "/from-1"
${{{VOLUME_UPPERID}}_PVC}          readOnly: true
${{{VOLUME_UPPERID}}_COS}        - name: cos-access-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_COS}          mountPath: "/etc/cos-access"
${{{VOLUME_UPPERID}}_COS}          readOnly: true
${{{VOLUME_UPPERID}}_ART}        - name: api-key-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_ART}          mountPath: "/etc/artifactory"
${{{VOLUME_UPPERID}}_ART}          readOnly: true

        - name: scratch-{{VOLUME_OPTION}}
          mountPath: "/to-1"
          readOnly: false
}W}
$enable_htdocs$htdocs_ART      - command: ["/bin/sh", "-c", "/usr/local/bin/artiget.sh '${{{NAME_UPPERID}}_HTDOCS}' --api-key-file /etc/artifactory/api-key -u -o /to-1"]
$enable_htdocs$htdocs_COS      - command: ["/bin/sh", "-c", "/usr/local/bin/cosget.sh '${{{NAME_UPPERID}}_HTDOCS}' -d /etc/cos-access -u -o /to-1"]
$enable_htdocs$htdocs_PVC      - command: ["/bin/sh", "-c", "rsync -vrtlog /from-1/. /to-1"] 
$enable_htdocs        image:  "{{PUSH_REPO:}}{{IMAGE}}" 
$enable_htdocs        securityContext: {privileged: true, runAsUser: 0} 
$enable_htdocs        name: volumes-init-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs        volumeMounts:
$enable_htdocs$htdocs_PVC        - name: {{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_PVC          mountPath: "/from-1"
$enable_htdocs$htdocs_PVC          readOnly: true
$enable_htdocs$htdocs_COS        - name: cos-access-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_COS          mountPath: "/etc/cos-access"
$enable_htdocs$htdocs_COS          readOnly: true
$enable_htdocs$htdocs_ART        - name: api-key-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_ART          mountPath: "/etc/artifactory"
$enable_htdocs$htdocs_ART          readOnly: true
$enable_htdocs        - name: scratch-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs          mountPath: "/to-1"
$enable_htdocs          readOnly: false
      containers:
      - name: {{NAME}}
        image: "{{PUSH_REPO:}}{{IMAGE}}"
        command: ["/home/worker/{{NAME}}/{{NAME}}-server"]
        args: ["{{MAIN_PORT}}"]
        ports:
          - name: grpc
            containerPort: {{MAIN_PORT}}
{E:HAVE_NODES{
        env:{{HAVE_NODES}}
          - name: {{NAME_UPPERID}}_TRACE
            value: "0"
          - name: {{NAME_UPPERID}}_DEBUG
            value: "1"
          - name: {{NAME_UPPERID}}_ASYNC
            value: "1"
{K:ORCHESTRATOR_ENVIRONMENT_KEY{
          - name: {{ORCHESTRATOR_ENVIRONMENT_KEY}}
            value: "{{ORCHESTRATOR_ENVIRONMENT_VALUE}}"
}K}}E}
{R:REST_NODE_NAME{
      - name: {{REST_NODE_NAME}}
        image: "{{REST_NODE_IMAGE}}"
        ports: 
          - name: rest
            containerPort: {{REST_IMAGE_PORT}}
        command: ["/home/worker/pr/bin/runpr3.sh"]
        args: ["--name", "{{NAME_UPPER}}", "--description", {{MAIN_DESCRIPTION}}, "--hostport", "localhost:{{MAIN_PORT}}"{E:REST_ENTRY{, "--service", "{{REST_ENTRY}}"}E}]
        volumeMounts:
        - name: {{REST_VOLUME_NAME}}
          mountPath: "/home/worker/pr/input"
          readOnly: true
$enable_htdocs        - name: {{HTDOCS_VOLUME_NAME}}
$enable_htdocs          mountPath: "/home/worker/pr/htdocs"
$enable_htdocs          readOnly: true
}R}
{N:G_NODE_NAME{
      - name: {{G_NODE_OPTION}}
        image: "{{G_NODE_IMAGE}}"
        {{G_NODE_ENVIRONMENT}}
        {{G_NODE_MOUNTS}}
        {{G_NODE_LIMITS}}
}N}
{V:HAVE_VOLUMES_OR_REST{
      volumes:{{HAVE_VOLUMES_OR_REST}}
}V}
{R:REST_NODE_NAME{
        - name: {{REST_VOLUME_NAME}}
          configMap:
            name: {{NAME}}-protos
$enable_htdocs$htdocs_PVC        - name: {{HTDOCS_VOLUME_NAME}}    
$enable_htdocs$htdocs_PVC          persistentVolumeClaim:
$enable_htdocs$htdocs_PVC            claimName: ${{{NAME_UPPERID}}_HTDOCS}
$enable_htdocs$htdocs_COS        - name: cos-access-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_COS          secret:
$enable_htdocs$htdocs_COS            secretName: ${{{NAME_UPPERID}}_HTDOCS_SECRET_NAME}
$enable_htdocs$htdocs_ART        - name: api-key-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_ART          secret:
$enable_htdocs$htdocs_ART            secretName: ${{{NAME_UPPERID}}_HTDOCS_SECRET_NAME}
$enable_htdocs$htdocs_COS        - name: scratch-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_COS          emptyDir: {}
$enable_htdocs$htdocs_ART        - name: scratch-{{HTDOCS_VOLUME_NAME}}
$enable_htdocs$htdocs_ART          emptyDir: {}
}R}
{W:VOLUME_NAME{
${{{VOLUME_UPPERID}}_PVC}        - name: {{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_PVC}          persistentVolumeClaim:
${{{VOLUME_UPPERID}}_PVC}            claimName: ${{{VOLUME_UPPERID}}}
${{{VOLUME_UPPERID}}_COS}        - name: cos-access-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_COS}          secret:
${{{VOLUME_UPPERID}}_COS}            secretName: ${{{VOLUME_UPPERID}}_SECRET_NAME}
${{{VOLUME_UPPERID}}_ART}        - name: api-key-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_ART}          secret:
${{{VOLUME_UPPERID}}_ART}            secretName: ${{{VOLUME_UPPERID}}_SECRET_NAME}
${{{VOLUME_UPPERID}}_COS}        - name: scratch-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_COS}          emptyDir: {}
${{{VOLUME_UPPERID}}_ART}        - name: scratch-{{VOLUME_OPTION}}
${{{VOLUME_UPPERID}}_ART}          emptyDir: {}
}W}
---
apiVersion: v1
kind: Service
metadata:
  name: {{NAME}}
  labels:
    flow-group: {{NAME}}
    app: {{NAME}}
spec:
  ports:
  - port: {{MAIN_PORT}}
    protocol: TCP
    name: grpc
{R:REST_NODE_NAME{
  - port:  {{REST_IMAGE_PORT}}
    protocol: TCP
    name: http
}R}
  selector:
    app: {{NAME}}
)TEMPLATE";
char const *template_runtime_Dockerfile = R"TEMPLATE(FROM ubuntu:18.04 AS flow-runtime

RUN apt-get -q -y update && DEBIAN_FRONTEND=noninteractive apt-get -q -y install locales lsb-release \
    curl jq bc ssh vim unzip \
    && apt-get clean

RUN locale-gen en_US.UTF-8 && /usr/sbin/update-locale LANG=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN useradd -u 1001 -m worker
RUN chown -R worker  /home/worker
USER worker
)TEMPLATE";
char const *template_server_C = R"TEMPLATE(/************************************************************************************************************
 *
 * {{NAME}}-server.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <grpc++/grpc++.h>
#include <grpc++/health_check_service_interface.h>
#include <google/protobuf/util/json_util.h>
template <class S>
static inline bool stringtobool(S s, bool default_value=false) {
    if(s.empty()) return default_value;
    std::string so(s.length(), ' ');
    std::transform(s.begin(), s.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(so.c_str()) != 0;
}
static inline bool strtobool(char const *s, bool default_value=false) {
    if(s == nullptr || *s == '\0') return default_value;
    std::string so(s);
    std::transform(so.begin(), so.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(s) != 0;
}
static std::string Message_to_json(google::protobuf::Message const &message, int max_length=1024) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    if(json_reply.length() > max_length) 
        return json_reply.substr(0, (max_length-5)/2) + " ... " + json_reply.substr(json_reply.length()-(max_length-5)/2);
    
    return json_reply;
}
inline std::ostream &operator << (std::ostream &out, std::chrono::steady_clock::duration time_diff) {
    auto td = double(time_diff.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
    char const *unit = "s";
    if(td < 1.0) { td *= 1000; unit = "ms"; }
    if(td < 1.0) { td *= 1000; unit = "us"; }
    if(td < 1.0) { td *= 1000; unit = "ns"; }
    return out << td << unit;
}
class sfmt {
    std::ostringstream os;
public:
    inline operator std::string() {
        return os.str();
    }
    template <class T> sfmt &operator <<(T v) {
        os << v; return *this;
    }
    sfmt &message(std::string const &message, long cid, int call, google::protobuf::Message const *msgp=nullptr) {
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char cstr[128];
        std::strftime(cstr, sizeof(cstr), "%a %F %T %Z (%z)", timeinfo);
        os << cstr;
        os << " [" << cid;
        if(call >= 0) os << ": " << call;
        os << "] " << message;
        if(msgp != nullptr) os << "\n" <<  Message_to_json(*msgp);
        os << "\n";
        return *this;
    }
};
#define MAX_LOG_WAIT_SECONDS 600
// 60*60*24*7
#define MAX_LOG_LIFE_SECONDS 1200 
struct Logbuffer_info {
    std::string call_id;
    std::string call_info;
    long expires;
    std::vector<std::string> buffer;
    std::mutex mtx;
    std::condition_variable cv;
    bool finished;

    Logbuffer_info(std::string const &cid, std::chrono::time_point<std::chrono::steady_clock> st): 
        call_id(cid), 
        expires(std::chrono::time_point_cast<std::chrono::seconds>(st+std::chrono::duration<int>(MAX_LOG_LIFE_SECONDS)).time_since_epoch().count()), 
        finished(false) {

    }
};
std::mutex global_display_mutex;

{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}

template <class C>
static bool Get_metadata_value(C mm, std::string const &key, bool default_value=false) { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return stringtobool(vp->second, default_value);
}

class Flow_logger_service final: public FlowService::Service {
    std::mutex buffer_store_mutex;
    std::map<std::string, std::shared_ptr<Logbuffer_info>> buffers;
public:
    ::grpc::Status log(::grpc::ServerContext *context, const ::FlowCallInfo *request, ::grpc::ServerWriter<::FlowLogMessage> *writer) override {
        std::string call_id(request->callid());
        std::shared_ptr<Logbuffer_info> bufinfop = Get_buffer(call_id);

        if(!bufinfop) 
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, sfmt() << "Invalid or expired call id: \"" << call_id << "\"");
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^begin streaming log for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
        
        unsigned curpos = 0;
        bool finished = false;
        { 
            std::unique_lock<std::mutex> lck(bufinfop->mtx);
            while(!finished) {
                finished = bufinfop->finished;
                while(curpos < bufinfop->buffer.size()) {
                    ::FlowLogMessage msg;
                    msg.set_message(bufinfop->buffer[curpos++]);
                    writer->Write(msg);
                }
                if(finished) break;
                /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^waiting to stream for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
                int wait_count = MAX_LOG_WAIT_SECONDS/2;
                std::cv_status status;
                do {
                    if(wait_count-- == 0) 
                        return ::grpc::Status(::grpc::StatusCode::CANCELLED, sfmt() << "Wait for log message excedeed " << MAX_LOG_WAIT_SECONDS << " seconds");
                    if(context->IsCancelled()) 
                        return ::grpc::Status(::grpc::StatusCode::CANCELLED, "Call exceeded deadline or was cancelled by the client");

                    status = bufinfop->cv.wait_until(lck, std::chrono::system_clock::now()+std::chrono::seconds(2));
                } while(status == std::cv_status::timeout);
            }
        }
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^finished streaming for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
        // garbage collect the log queues
        long now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count(); 
        {
            std::unique_lock<std::mutex> lck(buffer_store_mutex);
            auto p = buffers.begin();
            while(p != buffers.end()) {
                auto c = p; ++p; 
                if(c->second->expires > now) continue;
                buffers.erase(c);
            }
        }
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^done for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }*/
        return ::grpc::Status::OK;
    }
    std::shared_ptr<Logbuffer_info> New_call(std::string const &call_id, std::chrono::time_point<std::chrono::steady_clock> started) {
        std::unique_lock<std::mutex> lck(buffer_store_mutex);
        auto f = buffers.emplace(call_id, std::make_shared<Logbuffer_info>(call_id, started));
        auto r = f.first->second;
        return r;
    }
    std::shared_ptr<Logbuffer_info> Get_buffer(std::string const &call_id) {
        std::unique_lock<std::mutex> lck(buffer_store_mutex);
        auto f = buffers.find(call_id);
        if(f != buffers.end()) return f->second;
        return nullptr;
    }
};

bool Flow_logger_service_enabled;
static Flow_logger_service Flow_logger;
static void Print_message(std::shared_ptr<Logbuffer_info> lbip,  std::string const &message, bool finish=false) {
    if(Flow_logger_service_enabled && lbip) {
        std::unique_lock<std::mutex> lck(lbip->mtx);
        lbip->buffer.push_back(message);
        if(finish) lbip->finished = true;
        lbip->cv.notify_all();
    }
    global_display_mutex.lock();
    std::cerr << message << std::flush;
    global_display_mutex.unlock();
}
static std::string Grpc_error(long cid, int call, char const *message, grpc::Status &status, grpc::ClientContext &context, google::protobuf::Message const *reqp=nullptr, google::protobuf::Message const *repp=nullptr) {
    sfmt out;
    out.message(sfmt() << (message == nullptr? "": message) << "error " << status.error_code(), cid, call, reqp);
    out << "context info: " << context.debug_error_string() << "\n";
    if(repp != nullptr) std::cerr << "the response was: " << Message_to_json(*repp) << "\n";
    return out;
}
#define TRACECMF(c, n, text, messagep, finish) if((Trace_Flag || Trace_call) && (c)) { Print_message(LOGBIP, sfmt().message(text, CID, n, messagep), finish); } 
#define TRACECM(c, n, text, messagep) TRACECMF((c), (n), (text), (messagep), false) 
#define TRACE(c, text, messagep) TRACECM((c), -1, (text), (messagep)) 
#define TRACEA(text, messagep) TRACECM(true, -1, (text), (messagep))
#define TRACEAF(text, messagep) TRACECMF(true, -1, (text), (messagep), true)
#define TRACEC(n, text) TRACECM(true, (n), (text), nullptr)
#define ERROR(text) {Print_message(LOGBIP, sfmt().message((text), CID, -1, nullptr), true); } 
#define GRPC_ERROR(n, text, status, context, reqp, repp) Print_message(LOGBIP, Grpc_error(CID, (n), (text), (status), (context), (reqp), (repp)), true);
#define PTIME2(method, stage, stage_name, cet, td, calls) {\
    if(Time_call) Time_info << method << "\t" << stage << "\t" << stage_name << "\t" << cet << "\t" << td << "\t" << calls << "\n"; \
    if(Debug_Flag||Trace_Flag||Trace_call) Print_message(LOGBIP, sfmt().message(\
        sfmt() << method << " stage " << stage << " (" << stage_name << ") started after " << cet << " and took " << td << " for " << calls << " call(s)", \
        CID, -1, nullptr)); }

class {{NAME_ID}}_service final: public {{CPP_SERVER_BASE}}::Service {
public:
    bool Debug_Flag = false;
    bool Trace_Flag = false;
    bool Async_Flag;
    std::atomic<long> Call_Counter;

{I:CLI_NODE_NAME{
    /* {{CLI_NODE_NAME}} line {{CLI_NODE_LINE}}
     */
    int const {{CLI_NODE_ID}}_maxcc = {{CLI_NODE_MAX_CONCURRENT_CALLS}};
    std::unique_ptr<{{CLI_SERVICE_NAME}}::Stub> {{CLI_NODE_ID}}_stub[{{CLI_NODE_MAX_CONCURRENT_CALLS}}];
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_ID}}_prep(long CID, int call_number, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call, std::shared_ptr<Logbuffer_info> LOGBIP) {
        TRACECM(true, call_number, "{{CLI_NODE_NAME}} prepare request: ", A_inp);
        auto result = {{CLI_NODE_ID}}_stub[call_number % {{CLI_NODE_ID}}_maxcc]->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
        return result;
    }

    ::grpc::Status {{CLI_NODE_ID}}_call(long CID, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call, std::shared_ptr<Logbuffer_info> LOGBIP) {
        ::grpc::ClientContext L_context;
        /**
         * Some notes on error codes (from https://grpc.io/grpc/cpp/classgrpc_1_1_status.html):
         * UNAUTHENTICATED - The request does not have valid authentication credentials for the operation.
         * PERMISSION_DENIED - Must not be used for rejections caused by exhausting some resource (use RESOURCE_EXHAUSTED instead for those errors). 
         *                     Must not be used if the caller can not be identified (use UNAUTHENTICATED instead for those errors).
         * INVALID_ARGUMENT - Client specified an invalid argument. 
         * FAILED_PRECONDITION - Operation was rejected because the system is not in a state required for the operation's execution.
         */
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds({{CLI_NODE_TIMEOUT:120000}});
        L_context.set_deadline(deadline);
        ::grpc::Status L_status = {{CLI_NODE_ID}}_stub[0]->{{CLI_METHOD_NAME}}(&L_context, *A_inp, A_outp);
        if(!L_status.ok()) {
            GRPC_ERROR(-1, "{{CLI_NODE_NAME}}", L_status, L_context, A_inp, nullptr);
        } else {
            TRACE(true, "{{CLI_NODE_NAME}} request: ", A_inp);
            TRACE(true, "{{CLI_NODE_NAME}} reply: ", A_outp);
        }
        return L_status;
    }}I}
    // Constructor
    {{NAME_ID}}_service(bool async_a{I:CLI_NODE_ID{, std::string const &{{CLI_NODE_ID}}_endpoint}I}): Async_Flag(async_a) {
        Call_Counter.store(1);   
        
        {I:CLI_NODE_ID{for(int i = 0; i < {{CLI_NODE_ID}}_maxcc; ++i) {
            std::shared_ptr<::grpc::Channel> {{CLI_NODE_ID}}_channel(::grpc::CreateChannel({{CLI_NODE_ID}}_endpoint, ::grpc::InsecureChannelCredentials()));
            {{CLI_NODE_ID}}_stub[i] = {{CLI_SERVICE_NAME}}::NewStub({{CLI_NODE_ID}}_channel);                    
        }
        }I}
    }
{I:ENTRY_CODE{{{ENTRY_CODE}}
}I}
};

int main(int argc, char *argv[]) {
    if(argc != 2) {
       std::cout << "Usage: " << argv[0] << " PORT\n\n";
       std::cout << "Set the ENDPOINT variable with the host:port for each node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_ENDPOINT for node {{CLI_NODE_NAME}} ({{GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}})\n";
       }I}
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_DEBUG=1 to enable debug mode\n";
       std::cout << "Set {{NAME_UPPERID}}_TRACE=1 to enable trace mode\n";
       std::cout << "Set {{NAME_UPPERID}}_ASYNC=0 to disable asynchronous client calls\n";
       std::cout << "Set {{NAME_UPPERID}}_LOGGER=0 to disable the logger service\n";
       std::cout << "\n";
       return 1;
    }
    // Use the default grpc health checking service
	grpc::EnableDefaultHealthCheckService(true);
    int error_count = 0;

    {I:CLI_NODE_NAME{char const *{{CLI_NODE_ID}}_ep = std::getenv("{{CLI_NODE_UPPERID}}_ENDPOINT");
    if({{CLI_NODE_ID}}_ep == nullptr) {
        std::cout << "Endpoint environment variable ({{CLI_NODE_UPPERID}}_ENDPOINT) not set for node {{CLI_NODE_NAME}}\n";
        ++error_count;
    }
    }I}
    if(error_count != 0) return 1;

    {I:CLI_NODE_ID{std::string {{CLI_NODE_ID}}_endpoint(strchr({{CLI_NODE_ID}}_ep, ':') == nullptr? (std::string("localhost:")+{{CLI_NODE_ID}}_ep): std::string({{CLI_NODE_ID}}_ep));
    }I}
    int listening_port;
    grpc::ServerBuilder builder;

    {{NAME_ID}}_service service(strtobool(std::getenv("{{NAME_UPPERID}}_ASYNC"), true){I:CLI_NODE_ID{, {{CLI_NODE_ID}}_endpoint}I});
    service.Debug_Flag = strtobool(std::getenv("{{NAME_UPPERID}}_DEBUG"));
    service.Trace_Flag = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE"));

    // Listen on the given address without any authentication mechanism.
    std::string server_address("[::]:"); server_address += argv[1];
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &listening_port);

    // Register services
    builder.RegisterService(&service);
    if((Flow_logger_service_enabled = strtobool(std::getenv("{{NAME_UPPERID}}_LOGGER"), true)))
        builder.RegisterService(&Flow_logger);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if(listening_port == 0) {
        std::cerr << "failed to start {{NAME}} gRPC service at " << listening_port << "\n";
        return 1;
    }
    std::cerr << "gRPC service {{NAME}} listening on port: " << listening_port 
        << ", debug: " << (service.Debug_Flag? "yes": "no")
        << ", trace: " << (service.Trace_Flag? "yes": "no")
        << ", asynchronous client calls: " << (service.Async_Flag? "yes": "no") 
        << ", logger service: " << (Flow_logger_service_enabled? "enabled": "disabled") 
        << std::endl;
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
    return 0;
}
)TEMPLATE";
char const *template_syntax = R"TEMPLATE("flowc" - compiler for "gRPC" microservice orchestration

"flowc" shares the tokenizer with the "Protocol Buffers" compiler protoc. 

Strings, number literals (integer and floating point), and commnets are "C" style.
Identifiers must start with a letter, and can contain _ and digits. Aditionally,
boolean literals are availablle as "true" and "false".

The detailed specification of the "Protocol Buffers" version "3" can be found here:
    https://developers.google.com/protocol-buffers/docs/reference/proto3-spec

Details about "gRPC" connectivity can be found here:
    https://grpc.io/grpc/cpp/md_doc_connectivity-semantics-and-api.html

The flow specific statemets are: "node", "container", and "entry". The language reference
below is spcified using 'Extended Backus-Naur Form (EBNF)' with the following notation:

    '|'   for alternative productions
    '()'  grouping
    '[]'  zero or one time 
    '{}'  any number of times

Terminals are "emphasized".

A "node" definition is an abstraction of a gRPC service. 

"node" identifier "{"  
    "output" full_identifier "(" message_mapping ")" ";"
    '{'option_definition'}'
    '['environment']'
    '{'volume_mounts'}'
"}"

An "entry" is the implementation of one service method. Its name must match the service.method 
that it implements.

"entry" dotted_identifier "{"
    "return" "(" message_mapping ")" ";"
    '{'option_definition'}'
"}"

"container" is used to define any auxiliary "non-gRPC" service that is needed by the system. 
The syntax is identical to the "node" except that no "output" statement is allowed.

dotted_identifier = identifier'{'"."identifier'}'
identifier = [A-Za-z] '{' [A-Za-z] '|' [0-9] '|' "_" '}'

Node Level Properties:

"image"           String with the "Docker" image name and tag

"port"            Integer. The listening port value for the gRPC service. 

"environment"     Name-value map with the envitonment variables to be set when the image is run. 
                The values must be strings. 

"pod"             Label to be used as a deployment name in the "Kubernetes" configuration. If this value 
                is missing the nodes image will be run togther with the orchestrator in the main pod.

"limits"          Name-value map with settings that will be used as resource limits in the "Kubernetes" 
                deployment. The values must be strings and will be copied verbatim into the yaml file.

"init"            Stanza to be added to the "initContainers" section of the "Kubernetes" deployment

"replicas"        Number of overlapped calls that can be made to this service

"timeout"         Timeout for calling this node. By default no timeout is set.
)TEMPLATE";
char const *artiget_sh = R"TEMPLATE(#!/bin/bash

art_get_UNTAR=0
art_get_OUTPUT=.
art_get_FORCE=0
art_get_HELP=0
art_API_KEYS=

function clsp() {
    echo "$1" | tr $'\n' ' ' | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/[[:space:]][[:space:]]*/ /g'
}

if [ -f ~/.api-key ]
then
    art_API_KEYS="$(cat ~/.api-key) $art_API_KEYS"
fi
if [ -f ~/.api_key ]
then
    art_API_KEYS="$(cat ~/.api_key) $art_API_KEYS"
fi
if [ -f .api-key ] 
then
    art_API_KEYS="$(cat .api-key) $art_API_KEYS"
fi
if [ -f .api_key ] 
then
    art_API_KEYS="$(cat .api_key) $art_API_KEYS"
fi
art_API_KEYS="$API_KEY $art_API_KEYS"

curl_SILENT=
args=()
while [ $# -gt 0 ]
do
case "$1" in

    -o|--output)
    art_get_OUTPUT="${2-.}"
    shift; shift
    ;;

    -u|--untar)
    art_get_UNTAR=1
    shift
    ;;

    --api-key)
    art_API_KEYS="$2 $art_API_KEYS"
    shift; shift
    ;;

    --api-key-file)
    if [ -f "$2" ] 
    then
        art_API_KEYS="$(cat "$2") $art_API_KEYS"
    fi
    shift; shift
    ;;

    -f|--force)
    art_get_FORCE=1
    shift
    ;;

    -s|--silent)
    curl_SILENT=-s
    shift
    ;;

    --help)
    art_get_HELP=1
    shift
    ;;

    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

art_API_KEYS=$(clsp "$art_API_KEYS")

if [ $art_get_HELP -ne 0 -o -z "$1" ]
then 
    echo "Usage $0 RESOURCE-NAME"
    echo ""
    echo "options:"
    echo "  --help                              print this and exit"
    echo "  -o, --output <FILE|DIRECTORY>       destination for the downloaded file"
    echo "  -f, --force                         download the file even if a newer file exists"
    echo "  -s, --silent                        don't show download progress"
    echo "  -u, --untar                         untar the file at the destination, output must be a directory"
    echo "  --api-key <STRING>                  API key"
    echo "  --api-key-file <FILE>               file with the API key"
    echo ""
    echo "note: $0 looks for the environment variable API_KEY."
    echo "If not found, it looks for a file named .api-key in the current directory and then in the home directory."
    echo ""
    [ $art_get_HELP -ne 0 ] && exit 0
    exit 1
fi

chk_url="$(echo "$1" | tr '[:lower:]' '[:upper:]')"
if [ "${chk_url:0:7}" == "HTTP://" -o "${chk_url:0:8}" == "HTTPS://" ]
then
    art_get_URL="$1"
else
    if [ ! -z "$ARTIFACTORY_URL" ] 
    then
        art_get_URL="$ARTIFACTORY_URL/$1"
    else
        echo "Argument must be an HTTP URL"
        exit $1
    fi
fi

art_get_FILE="$(basename "$art_get_URL")"
case "$art_get_FILE" in
    *.tgz|*.tar.gz)
        tar_Z=-z
    ;;
    *.tar.bz2)
        tar_Z=-j
    ;;
    *.tar.xz)
        tar_Z=--xz
    ;;
    *)
        tar_Z=
    ;;
esac

art_get_HAVE_FILE=
download_API_KEY=

rc=1
for api_key in $art_API_KEYS 
do
    HEADERS=$(curl -f -s -I -H "X-JFrog-Art-Api:$api_key" "$art_get_URL")
    rc=$?
    [ $rc -ne 0 ] && continue
    download_API_KEY=$api_key
    break
done

if [ $rc -ne 0 ]
then
    echo "Can't get resource: $art_get_URL"
    exit $rc
fi

if [ $art_get_FORCE -eq 0 ]
then

LASTMOD="$(echo "$HEADERS" | grep -i 'last-modified' | cut -d ' ' -f 2- | cut -d ',' -f 2- | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"

if [ "$(uname -s)" == "Linux" ]
then
    LMTS=$(/bin/date -d "$LASTMOD" '+%Y%m%d%H%M.%S')
else
    LMTS=$(/bin/date -j -f '%d %b %Y %H:%M:%S %Z' "$LASTMOD" '+%Y%m%d%H%M.%S')
fi

[ -d "$art_get_OUTPUT" ] || mkdir -p "$art_get_OUTPUT"

touch -t "$LMTS" "$art_get_OUTPUT/tmp$$"
if [ $art_get_UNTAR -ne 0 ]
then
    art_get_HAVE_FILE="$(find "$art_get_OUTPUT" -newer "$art_get_OUTPUT/tmp$$" -name ".downloaded.$art_get_FILE")"
else
    art_get_HAVE_FILE="$(find "$art_get_OUTPUT" -newer "$art_get_OUTPUT/tmp$$" -name "$art_get_FILE")"
fi
rm -f "$art_get_OUTPUT/tmp$$"
fi

rc=0
if [ -z "$art_get_HAVE_FILE" ]
then
    [ -d "$art_get_OUTPUT" ] || mkdir -p "$art_get_OUTPUT"
    cd "$art_get_OUTPUT"
    echo "getting $art_get_URL"
    if [ $art_get_UNTAR -ne 0 ] 
    then
        curl $curl_SILENT -f -H "X-JFrog-Art-Api:$download_API_KEY" "$art_get_URL" --output - | tar -x $tar_Z 
        rc=$?
        [ $rc -eq 0 ] && touch .downloaded.$art_get_FILE
    else
        curl $curl_SILENT -f -O -H "X-JFrog-Art-Api:$download_API_KEY" "$art_get_URL"
        rc=$?
    fi
else
    echo "$art_get_HAVE_FILE is newer that the remote resource"
fi
exit $rc
)TEMPLATE";
char const *cosget_sh = R"TEMPLATE(#!/bin/bash

s3_get_UNTAR=0
s3_get_OUTPUT=.
ACCESS_KEY_ID="${S3_ACCESS_KEY_ID}"
SECRET_ACCESS_KEY="${S3_SECRET_ACCESS_KEY}"

args=()
while [ $# -gt 0 ]
do
case "$1" in

    -o|--output)
    s3_get_OUTPUT="${2-.}"
    shift; shift
    ;;

    -u|--untar)
    s3_get_UNTAR=1
    shift
    ;;

    --access-key-id)
    ACCESS_KEY_ID="$2"
    shift; shift
    ;;

    --secret-access-key)
    SECRET_ACCESS_KEY="$2"
    shift; shift
    ;;

    -a|--access)
    ACCESS_KEY_ID=${2%:*}
    SECRET_ACCESS_KEY=${2#*:}
    shift; shift
    ;;

    -d|--access-directory)
    ACCESS_KEY_ID="$(cat "$2/ACCESS_KEY_ID")"
    SECRET_ACCESS_KEY="$(cat "$2/SECRET_ACCESS_KEY")"
    shift; shift
    ;;

    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

URL="$1"
if [ -z "$URL" ]
then
    echo "usage $0 [options] <URL>"
    echo ""
    echo "options:"
    echo "  -o, --output <FILE|DIRECTORY>       destination for the downloaded file"
    echo "  -u, --untar                         untar the file at the destination, output must be a directory"
    echo "  --access-key-id <STRING>            key id"
    echo "  --secret-access-key <STRING>        access secret"
    echo "  -a, --access <KEY:SECRET>           access key and secret"
    echo "  -d, --access-directory <DIRECTORY>  directory with files ACCESS_KEY_ID and SECRET_ACCESS_KEY"
    echo ""
    echo "note: default access information can be set in S3_ACCESS_KEY_ID and S3_SECRET_ACCESS_KEY"
    echo ""
    exit 1
fi

URLHP=${URL#*//}
FILE=$(basename $URLHP)
OUTPUT_FILE="${s3_get_OUTPUT-$FILE}"
if [ "$s3_get_OUTPUT" != "-" -a -d "$s3_get_OUTPUT" -a $s3_get_UNTAR -eq 0 ]
then
    OUTPUT_FILE="${s3_get_OUTPUT}/${FILE}"
fi


if [ ! -d "$OUTPUT_FILE" -a $s3_get_UNTAR -ne 0 ]
then
    echo "Output must be a directory"
    exit 1
fi

HOST="${URLHP%%/*}"
RESOURCE="${URLHP#*/}"

CONTENT_TYPE="application/x-compressed-tar"
DATE_VALUE="$(date +'%a, %d %b %Y %H:%M:%S %z')" 
TO_SIGN="GET

$CONTENT_TYPE
$DATE_VALUE
/$RESOURCE" 

SIGNATURE=$(/bin/echo -n "$TO_SIGN" | openssl sha1 -hmac "$SECRET_ACCESS_KEY" -binary | base64)

if [ $s3_get_UNTAR -eq 0 ]
then
curl -fsS -H "Host: $HOST" \
  -H "Date: $DATE_VALUE" \
  -H "Content-Type: $CONTENT_TYPE" \
  -H "Authorization: AWS $ACCESS_KEY_ID:$SIGNATURE" \
  "https://$URLHP" --output "$OUTPUT_FILE"
RC=$?
else 
case "$RESOURCE" in
    *.tgz|*.tar.gz)
        TAR_Z=-z
    ;;
    *.tar.bz2)
        TAR_Z=-j
    ;;
    *.tar.xz)
        TAR_Z=--xz
    ;;
    *)
        TAR_Z=
    ;;
esac
curl -fsS -H "Host: $HOST" \
  -H "Date: $DATE_VALUE" \
  -H "Content-Type: $CONTENT_TYPE" \
  -H "Authorization: AWS $ACCESS_KEY_ID:$SIGNATURE" \
  "https://$URLHP" --output - | tar -C "$OUTPUT_FILE" $TAR_Z -xv 
RC=$?
fi

exit $RC
)TEMPLATE";
std::map<std::string, char const *> templates = {
{"Dockerfile", template_Dockerfile},
{"Makefile", template_Makefile},
{"client.C", template_client_C},
{"docker-compose.sh", template_docker_compose_sh},
{"docker-compose.yaml", template_docker_compose_yaml},
{"flow-logger.proto", template_flow_logger_proto},
{"help", template_help},
{"kubernetes.group.yaml", template_kubernetes_group_yaml},
{"kubernetes.sh", template_kubernetes_sh},
{"kubernetes.yaml", template_kubernetes_yaml},
{"runtime.Dockerfile", template_runtime_Dockerfile},
{"server.C", template_server_C},
{"syntax", template_syntax},
{"artiget.sh", artiget_sh},
{"cosget.sh", cosget_sh},
};
