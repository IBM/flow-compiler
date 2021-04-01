# Building a Flow Application

## Introduction

The Flow Compiler offers a solution for quickly building a **gRPC** enabled service by glueing together a number of other **gRPC** enabled microservices.

The input to **flowc** consists of:

* The **gRPC** definitions of all the microservices to be glued together
* The **gRPC** definition of the final aggregated service
* The definition of a graph that repesents how the information flows from/to the component microservices.

The output is:

* **C++** source code for the aggregator 
* `Makefile` and `Dockerfile` to build the binary and/or container
* **HTML** forms that can be used to interact with the aggregated service
* A deployment script for the containerized application that can be used with **Docker Compose**, **Docker Swarm**, and **Kubernetes**

## The Interface

The aggregated service is defined as a **gRPC** service that uses **Protocol Buffer** *version 3.0*. 
> Details about the language definitions are [here](https://developers.google.com/protocol-buffers/).



**flowc** will generate **C++** code for the application. Optionally a **Docker** image containing the ready to run application can be generated.

**flowc** can also generate a deployment tool that can be used with **Kubernetes**, **Docker Compose** or **Docker Swarm**, if the application is dockerized.  

More information about **Protocol Buffers** can be found here: [https://developers.google.com/protocol-buffers/](https://developers.google.com/protocol-buffers/), and 
about **gRPC**, here: [https://grpc.io](https://grpc.io).

Glossary of terms used here:

* **application method**
gRPC service method to be implemented
* **node response, or result message**
gRPC message specified as output for the gRPC service method assciated with the node
* **node input, or request message**
gRPC message specified as input for the gRPC service method assciated with the node
* **node method**
gRPC method associated with the node

## Defining the Application Interface

The first step of building the applicaton is defining its **gRPC** interface. This is done using the **Protocol Buffers Verson 3** language (see details [here](https://developers.google.com/protocol-buffers/docs/reference/proto3-spec).) Note that there are some restrictions in how the accepted gRPC definitions.

As an example we will build a tokenizer application that combines the output from two other services: one that splits a string around white-space and 
one that identifies if a string is all white space or all punctuation and returns a corresponding label.

First, the **.proto** definition of the interface for the main application, the tokenizer service. This is the content of the file [*main.proto*](example/two-node/main.proto) from the *two-node* example:
```proto
syntax = "proto3";

service Tokenizer {
    rpc tokenize(TokenizeRequest) returns(TokenizeResponse) {}
};

message TokenizeRequest {
    string sentence=1;
    bool preserve_white_space=2;
};

message TokenizeResponse {
    string original_sentence=1;
    message Token {
        string word=1;
        int32 begin=2;
        int32 end=3;
        string label=4;
    }   
    repeated Token tokens=3;
};          
```
Note that we start by declaring the syntax to be *proto3*. This is a requirement and must always be the first line in the file.

In this example the service has only one method, but there is no restriction on the number of methods that can be declared.
However, all methods must be part of the same service and must have the same input message type.

Next the definitions of the two services used, [*splitter.proto*](example/two-node/splitter.proto):
```proto
syntax = "proto3";

service SplitterService {
    rpc split(SentenceRequest) returns(TokensResponse) {}
};

message SplitterRequest {
    // the sentence to split
    string sentence=1;
    // whether to create tokens from the white space
    bool preserve_white_space=2;
};

message TokensResponse {
    message Token {
        // start position in the sentence, zero based
        int32 begin=1,
        // end position in the sentence, zero based
        int32 end=2,
        string token=3,
    }
    repeated Token tokens=1;
};
```
and [*labeler.proto*](example/two-node/labeler.proto):
```proto
syntax = "proto3";

service LabelerService {
    rpc label(WordRequest) returns(LabelResponse) {}
};

message WordRequest {
    string word=1;
};

message LabelResponse {
    /* either word, white-space, or punctuation */
    string label=2;
};
```

## Defining the Flow

The relations between the fields of the request and response messages for each service are described using a graph. 
Each node in the graph represents a **gRPC** service method. As a consequence each node has two associated message types, one for input and one for output. 

Before we can define how the message fields relate to each other we have to acquire the definitions of all the messages and services involved.

First, the application service definition:
```proto 
import "main.proto";
```
Next the **.proto** files for the two services needed:
```proto
import "labeler.proto";
import "splitter.proto";
```

The next step is to define how the response message fields are filled for the main method *tokenize*. 

The keyword **entry** is used to define a set or properties associated with an application service method. 
One of the properties is the mapping bewteen the response message fields and the response message fields from microservices, using the keyword **return.**
Since each node in the graph has only one associated response message, every field can be referred to, by specifying the node name and the name of the field. 
The special node label **input** refers to the request message for this entry.

The first field, *original_sentence* is filled directlty from the input. The fields of the repeated field *tokens* are filled from responses from the nodes
*splitter* and *labeler*.

```proto
entry Tokenizer.tokenize {
    return (
        original_sentence: input@ sentence,
        tokens: ( 
            word:  splitter@tokens.token,
            begin: splitter@tokens.begin,
            end:   splitter@tokens.end,
            label: labeler@label
        )
    );
}
```

For every node we can define properties with the keyword **node.** The only property always required is **output**. It specifies both the service method used, and how the request message fields are populated:

```proto
node splitter {
    output SplitterService.split(sentence: input@sentence, preserve_white_space: input@preserve_white_space); 
}
node labeler {
    output LabelerService.label(word: splitter@ tokens.token);
}
```

Note that the request message fields for *splitter* can all be filled with fields from the application input, while the *labeler* field requires the output from *splitter.*

## Restrictions

* Streaming is currently not supported for either the node or the final application methods.
* Protocol buffer maps are not supported in any of the messages.

## Compiling

**flowc** compiles the input *.flow* and *.proto* files into C++ code. Optionally the C++ compiler can be invoked to obtain a binary file.

```bash
# Generate C++ code for the server
flowc example/two-node/tokenizer.flow --server

# Generate C++ code for the server and invoke the default C++ compiler
flowc example/two-node/tokenizer.flow --build-server
```

If **docker** is available, the server can be built into a ready to run image:

```bash
# Build docker image with the application server inside
flowc example/two-node/tokenizer.flow --build-image
```

## Kubernetes Notes

Build a deployment tool for **Kubernetes**
```bash
flowc --kubernetes example/two-node/tokenizer.flow
```

## Docker Compose Notes
Build a **Docker Compose** driver tool
```bash
flowc --docker-compose example/two-node/tokenizer.flow
```
