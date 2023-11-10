# Azure RTOS OPC UA Samples 

This repo contains sample projects of open62541 running on Azure RTOS.

## Getting Started

[GitHub Codespaces](https://github.com/features/codespaces) is the preferred way to building and run these sample if you have your GitHub account enabled for this feature. Otherwise, you can still use it with the [local dev container](https://code.visualstudio.com/docs/remote/containers) or set up the toolchain by your own.


## Building the project

1. Fork this GitHub repo to your own GitHub account. Codespaces requires read/write access which is why a fork is necessary.

1. From the forked repo page, create a new codespace. Wait until the codespace is set up.

1. The VSCode web opens and you can see the smaples in the Explorer view under src. 

1. Depress Ctrl+Shift+P to activate the command palette pop-up, then select Tasks: Run Task and then select Build Azure RTOS OPC UA Project.

1. After the building is completed, depress Ctrl+Shift+P and select Tasks: Run Task again. Select Build open62541 Project.

## Running the project

Follow the steps below to run the examples.

### OPC UA Server on Azure RTOS

This example runs a simple OPC UA server on Azure RTOS. The OPC UA client for Linux will connect to the server using TCP and read the variable holding the server current time.

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select server to run the server example.
    The terminal window should output the log that the server is listening on the network.

1. Open a new terminal by selecting the "+" in the terminal window. And then run the following command:

    ```
    ./libs/open62541/build/bin/examples/client_connect opc.tcp://192.168.201.5:4840/
    ```

    The terminal should display the server date read by the client. The follwoing is a sample output:

    ```
    ...
    [2023-08-15 01:34:21.896 (UTC+0000)] info/client        Client Status: ChannelState: Open, SessionState: Created, ConnectStatus: Good
    [2023-08-15 01:34:21.897 (UTC+0000)] info/client        Client Status: ChannelState: Open, SessionState: Activated, ConnectStatus: Good
    [2023-08-15 01:34:21.897 (UTC+0000)] info/userland      Connected!
    [2023-08-15 01:34:21.897 (UTC+0000)] info/userland      The server date is: 15-08-2023 01:34:20.970
    [2023-08-15 01:34:21.899 (UTC+0000)] info/client        Client Status: ChannelState: Closed, SessionState: Closed, ConnectStatus: Good
    ```

### OPC UA Client on Azure RTOS

This example runs a simple OPC UA client on Azure RTOS and it will connect to the open62541 server for Linux to read the current time from it.

1. Run the following command in the terminal window to start the server:

    ```
    ./libs/open62541/build/bin/examples/server_mainloop
    ```

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select client to run the client example on Azure RTOS.

    The terminal will output the server date. The following is a sample output:

    ```
    ...
    [2023-08-15 01:38:23.850 (UTC+0000)] info/client        Selected UserTokenPolicy open62541-anonymous-policy with UserTokenType Anonymous and SecurityPolicy http://opcfoundation.org/UA/SecurityPolicy#None
    [2023-08-15 01:38:23.900 (UTC+0000)] info/client        Client Status: ChannelState: Open, SessionState: Created, ConnectStatus: Good
    [2023-08-15 01:38:23.950 (UTC+0000)] info/client        Client Status: ChannelState: Open, SessionState: Activated, ConnectStatus: Good
    [2023-08-15 01:38:24.000 (UTC+0000)] info/userland      date is: 15-08-2023 01:38:24.962
    [2023-08-15 01:38:24.050 (UTC+0000)] info/client        Client Status: ChannelState: Closed, SessionState: Closed, ConnectStatus: Good
    ```

### OPC UA PubSub on Azure RTOS

This example runs a PubSub publisher on Azure RTOS to publish over UDP multicast and a subscriber to read the published message.

1. Depress Ctrl+Shift+P to activate the command palette, then select Tasks: Run Task and then select Run Azure RTOS OPC UA Project. Select pubsub_publish to run the pubsub_publish example.

1. Open a new terminal by selecting the "+" in the terminal window. And then run the following command:

    ```
    sudo ./build/pubsub_subscribe
    ```

    This will run the subscriber on Azure RTOS and the following is a sample output:

    ```
    ...
    [2023-08-15 02:02:49.510 (UTC+0000)] info/userland      PubSub channel requested
    [2023-08-15 02:02:49.610 (UTC+0000)] info/userland      Message length: 39
    [2023-08-15 02:02:49.610 (UTC+0000)] info/userland      Message content: [DateTime]     Received date: 2023-08-15 Received time: 02:02:50
    [2023-08-15 02:02:49.680 (UTC+0000)] info/userland      Message length: 39
    [2023-08-15 02:02:49.680 (UTC+0000)] info/userland      Message content: [DateTime]     Received date: 2023-08-15 Received time: 02:02:50
    ...
    ```

## Supported Build Options

The following build options are currently supported when building with Azure RTOS:

- UA_ENABLE_PUBSUB
- UA_ENABLE_DISCOVERY
- UA_MULTITHREADING
- UA_ENABLE_SUBSCRIPTIONS
- UA_ENABLE_SUBSCRIPTIONS_EVENTS
- UA_ENABLE_DA
- UA_ENABLE_METHODCALLS

## Resources

- [Azure RTOS ThreadX](https://github.com/azure-rtos/threadx)
- [Azure RTOS NetX Duo](https://github.com/azure-rtos/netxduo)
- [open62541](https://github.com/open62541/open62541)
