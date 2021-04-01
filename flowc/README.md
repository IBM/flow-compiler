# Quick Start
The flow-compiler is written in **C++** and uses a variety of open source components. The generated code also has dependencies on open source libraries. 

## Prerequisites
The first step is to install the required components:

- **C++** compiler that supports the *c++11* standard
- **gRPC** libraries and the **gRPC** compiler with the **C++** plugin. Details for your platform are here: [grpc.io](https://grpc.io/docs/languages/cpp/quickstart)
- **dot**, from the **graphviz** toolkit. Details are here: [graphviz.org](http://www.graphviz.org)
- **Docker**, with installation instructions here: [docs.docker.com/install](https://docs.docker.com/install/) 
- **gnutar**, only for **macOs**
      
If you intend to run the final application natively, some other components might be needed:

- The **civetweb** embeddable web server, if you want to add a **REST** API or a web interface to you application. See details the github project: [civetweb](https://github.com/civetweb/civetweb)

## Building the compiler:

