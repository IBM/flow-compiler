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

- The **civetweb** embeddable web server, if you want to add a **REST** API or a web interface to you application. See details in the github project: [civetweb](https://github.com/civetweb/civetweb). Find instructions on how to build [here](BUILD-civetweb.md)

Note that on **macOs** most of the above requirements can be installed with `brew` and that is the recommended method.

## Building the compiler:

Compile `flowc` with default settings

```bash
cd flowc && make -j
```

To install `flowc` in the directory pointed by `BIN_DIR` use the following command:
```bash
cd flowc && make install INSTALL_PREFIX=$BIN_DIR
```
Note that the above command might need to be prefixed with `sudo` if you are attempting to install in a system directory.


To build a docker image with the **flow-compiler** in it (requires access to the **Docker** daemon):

```bash 
cd flowc && make image
```
Note that the docker image is needed if you plan to build and run applications directly in a container.


To customize the build create a file `makefile.local` with the your options in the directory with the `Makefile`.

