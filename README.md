# Azure RTOS OPC UA Samples 

This repo contains sample projects of open62541 running on Azure RTOS.

## Getting Started

[GitHub Codespaces](https://github.com/features/codespaces) is the preferred way to building and run these sample if you have your GitHub account enabled for this feature. Otherwise, you can still use it with the [local dev container](https://code.visualstudio.com/docs/remote/containers) or set up the toolchain by your own.


## Building and running the project

1. Fork this GitHub repo to your own GitHub account. Codespaces requires read/write access which is why a fork is necessary.

1. From the forked repo page, create a new codespace. Wait until the codespace is set up.

1. The VSCode web opens and you can see the smaples in the Explorer view under src. 

1. Depress Ctrl+Shift+P to activate the command palette pop-up, then select Tasks: Run Task and then select Build Azure RTOS OPC UA Project.

1. After the building is completed, depress Ctrl+Shift+P and select Tasks: Run Task again. Select Build open62541 Project.

Following the following steps to run the samples.

### server

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select server to run the server sample.
    The terminal window should output the log that the server is listening on the network.

1. Open a new terminal by selecting the "+" in the terminal window. And then run the following command:

```
./libs/open62541/build/bin/examples/client_connect opc.tcp://192.168.201.5:4840/
```

    The terminal should display the server date read by the client.

### client

1. Run the following command in the terminal window:

```
./libs/open62541/build/bin/examples/server_mainloop
```

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select client to run the client sample.

    The terminal will output the server date.

### pubsub_publish

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select pubsub_publish to run the pubsub_publish sample.

1. Open a new terminal by selecting the "+" in the terminal window. And then run the following command:

```
./libs/open62541/build/bin/examples/tutorial_pubsub_subscribe
```

### pubsub_subscribe

1. Run the following command in the terminal window:

```
./libs/open62541/build/bin/examples/tutorial_pubsub_publish
```

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select client to run the client sample.

### discovery_server_regist

1. Run the following command in the terminal window:

```
./libs/open62541/build/bin/examples/discovery_server_lds
```
1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select discovery_server_regist.

### discovery_client_find_servers

Run the discovery_server_regist sample first and then open a new terminal by selecting the "+" in the terminal window. Run the following command:
```
sudo ./build/discovery_client_find_servers
```

The output should contain a list of registered servers found by the client.

## Resources

- [Azure RTOS ThreadX](https://github.com/azure-rtos/threadx)
- [Azure RTOS NetX Duo](https://github.com/azure-rtos/netxduo)
- [open62541](https://github.com/open62541/open62541)
