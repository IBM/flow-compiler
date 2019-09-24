# Three Nodes Example

A toy application that combine three services that run in separate docker containers. 
The application is held together with **Docker Compose**.

# Build

First step is to create a **Docker** image with the gRPC services for the nodes. For simplicity the services are all implemented in the same **Python** script, so only one **Docker** image will need to be created.

```
make nodes-image
```

The second step is to build the flow image.


