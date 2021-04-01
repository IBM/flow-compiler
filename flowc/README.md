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

- The **civetweb** embeddable web server, if you want to add a **REST** API or a web interface to you application. See details in the github project: [civetweb](https://github.com/civetweb/civetweb). Find [here](BUILD-civetweb.md) instructions on how to build the **civetweb** library.

Note that on **macOs** most of the above requirements can be installed with `brew` and that is the recommended method.

## Build the compiler

Compile `flowc` with default settings

```bash
cd flowc && make -j
```

To install `flowc` in the directory pointed by `BIN_DIR` use the following command:
```bash
cd flowc && make install INSTALL_PREFIX=$BIN_DIR
```
Note that the above command might need to be prefixed with `sudo` if you are attempting to install in a system directory.


If you plan to build and run applications directly in a container, you have to first build a docker image with the compiler in it (this requires access to the **Docker** daemon):

```bash 
cd flowc && make image
```

A number of variables can be set to customize the build. All of them can be given to `make` at the command line, or written in a file called `makefile.local` located in the same directory with the `Makefile`.

This is a list of variables that can be changed:

- **INSTALL_PREFIX** the location of the binary after make install, defaults to `/usr/local/bin`
- **BASE_IMAGE** the name of the default runtime for the containerized applications, defaults to `redhat-ubi8`. Other available runtimes are `alpine-3.13` and `ubuntu-20.04`.
- **GRPC_INCS** and **GRPC_LIBS** compile and link flags for the grpc libraries. The defaults are obtained with `pkg-config`  

# Continue To 

- How to define the application [flow](../docs/FLOW.md) and compile it into a service.
- How to build a **Docker** image with the flow [application].
