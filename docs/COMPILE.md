# Compile a Flow Application


## Compiling

**flowc** compiles the input *.flow* and *.proto* files into C++ code. Optionally the C++ compiler can be invoked to obtain a binary file.

```bash
# Generate C++ code for the server
flowc example/two-node/tokenizer.flow --server

# Generate C++ code for the server and invoke the default C++ compiler
flowc example/two-node/tokenizer.flow --build-server
```

## Building the image

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
