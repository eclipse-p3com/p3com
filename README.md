# Eclipse p3com - portable pluggable publish/subscribe communication

## Introduction

**Eclipse p3com enables middleware libraries to leverage the performance of
platform-specific interfaces.** This is in contrast to ethernet-based
networking, which is the default data exchange mechanism in all established
implementations of middleware protocols such DDS, SOME/IP or MQTT.

Eclipse p3com is also an extension of the [Eclipse
iceoryx](https://github.com/eclipse-iceoryx/iceoryx) middleware that enables it
to work over PCI Express, UDP, TCP and other transport layers/protocols, apart
from the default inter-process-communication (IPC) that iceoryx supports. It
could also be said that p3com provides the infrastructure for writing iceoryx
gateways.

Eclipse p3com consists of:
* platform-agnostic infrastructure (e.g., discovery system, generic gateway endpoints)
* platform-specific transport layers (useful as is, or as examples for new transport layer for other platforms)

*Warning: Currently, the source code in this repository is the initial
contribution to the p3com project. It is in an experimental stage, probably
with many issues and bugs. It only contains the UDP and TCP transport layers,
mostly intended for debug purposes. In future contributions, the PCI Express
driver stack for the NXP BlueBox3 development platform will be contributed,
along with the PCIe transport layer implementation in p3com. This PCIe
transport layer will truly demonstrate the strength of p3com and the full
features available. Furthermore, useful examples and demos will also be
gradually contributed over time.*

## Features

### p3com gateway

Eclipse iceoryx has the concept of a **gateway**. In general, it is a daemon
process that forwards all local inter-process-communication (between user
applications using the iceoryx API) over a particular channel (such as an
ethernet bus) to a different device, where it is received by a corresponding
gateway process and published into its local iceoryx system. In other words, it
mirrors all the iceoryx traffic between multiple devices, thus enabling that
user publishers and subscribers can communicate even if they are running on
different devices (nodes). Gateway implementations can use the iceoryx
introspection mechanism to dynamically discover local user publishers and
subscribers.

Eclipse p3com is essentially a **iceoryx gateway implementation**. This gateway
contains a custom device- and service-discovery system and a modular
transport layer architecture supporting the following layers:
* PCI Express via an NXP-internal Linux PCIe driver stack
* UDP/IP via the ASIO networking library
* TCP/IP via the ASIO networking library

### Discovery system

Instances of the p3com gateway (running on each device) keep track of:
* Iceoryx publishers and subscribers in user applications on the local device
(node)
* Remote p3com gateway instances and their enabled transport types
* Iceoryx subscribers in user applications in the remote gateway instances'
devices (nodes)

The local discovery data is obtained via the iceoryx introspection
functionality.  Remote discovery of other running gateways and their associated
subscribers is achieved via a custom discovery protocol over the enabled
transport layer.

This discovery information is then used when a user publisher publishes a
sample.  This sample is received by a corresponding subscriber in the local
gateway and then it is forwarded (over the appropriate transport layer) to all
remote gateway instances which register at least one user subscriber with a
matching service description. Finally, these remote gateways will publish the
received sample with their own associated publisher.  This way, the sample gets
to all relevant user subscribers.

#### Forwarding mechanism

The discovery system also includes support for forwarding between different
transport layers.

Here is an example setup: Consider three devices A, B and C. Device A is
running the p3com gateway with only PCIe transport enabled, device B is running
the p3com gateway with both PCIe and UDP transports enabled and device C is
running the p3com gateway with only the UDP transport enabled. Furhermore, the
p3com gateway on device B is configured to forward the relevant service. In
this case, a publisher on device A can communicate with a matching subscriber
on device C (and vice versa), because the gateway on device B will forward all
traffic between the PCIe and UDP interfaces.

The configuration of p3com gateway on device C has to be done statically with
the gateway configuration file, as described below.

#### Gateway introspection API

The p3com gateway discovery mechanism has a certain necessary lag to register
a new user publisher and share the new discovery state to other devices after
each user publisher is created and offered. The p3com library therefore exposes
a pair of API functions `hasGwRegistered` and `waitForGwRegistration` which
make it possible to wait for the gateway registration, either synchronously or
asynchronously.

Futhermore, an additional API function `isGwRunning` is provided, which can be
used to query whether the p3com gateway daemon is currently running on the
device.

### Automatic transport switching on failure

The p3com project is developed in an automotive context, therefore we have
safety in mind. It is possible that a transport interface might fail during the
runtime of the gateway. In this case, the gateway application will not crash,
but simply disable the faulty transport layer and continue forwarding
(and receiving) the iceoryx traffic over the other enabled transport layers.

Note that this feature has only been tested for UDP transport failures, in
particular by switching off the used ethernet interface, for example with
`ifconfig eth0 down`. Issues with the PCIe transport are much more difficult to
recover from, and are currently not supported.

### PCIe DMA acceleration

The PCI Express transport layer has a special capability to use the DMA engine
embedded in the PCIe controller. If the sample size exceeds a certain treshold,
provisionally set to 4kB, the DMA controller is used to execute the copy of the
sample payload between the devices, instead of the usual PCIe BAR-window-based
messaging protocol.  Moreover, thanks to a sophisticated userspace buffer DMA
support in the BlueBox3 PCIe stack, the DMA engine directly operates on the
iceoryx memory pool buffers, avoiding any redundant copy. This technique can
achieve extremely high-throughput data-sharing performance.

### FreeRTOS support

TODO in future contributions.

## Integration to Cyclone DDS

Please refer to [CycloneDDS.md](./doc/CycloneDDS.md) for instructions how to
build and run Cyclone DDS with p3com.

## Build and install

### Dependencies

Eclipse p3com depends on Eclipse iceoryx v2.0.3. It is recommended to build it
from source and install it.

The UDP and TCP transport layers depend on the open-source ASIO library. You
can install it via these commands on Ubuntu:
```
cd /tmp && wget https://sourceforge.net/projects/asio/files/asio/1.24.0%20%28Stable%29/asio-1.24.0.tar.gz/download -O asio-1.24.0.tar.gz
cd /tmp && tar xf asio-1.24.0.tar.gz
cp -r /tmp/asio-1.24.0/include/asio* /usr/include
```

### CMake options

The p3com fork adds the following top-level CMake options:

* `PCIE_TRANSPORT`, enables the PCIe transport layer in the p3com gateway.
* `UDP_TRANSPORT`, enables the UDP transport layer in the p3com gateway.
* `TCP_TRANSPORT`, enables the TCP transport layer in the p3com gateway.

The p3com gateway application is built if at least one of the `PCIE_TRANSPORT`,
`UDP_TRANSPORT` and `TCP_TRANSPORT` options are enabled. Only one of
`UDP_TRANSPORT` and `TCP_TRANSPORT` can be enabled at the same time.

<!--- TODO: Add CMake build instructions, after refactors of CMakeLists.txt -->

## Usage

### Gateway command line options

Since the p3com gateway is just another iceoryx application, it requires the
RouDi daemon running in the background.

You can see the command line options of the p3com gateway by supplying the `-h`
flag:

```
p3com gateway application
usage: ./p3com-gw [options]
  options:
    -h, --help                Print help and exit
    -l, --log-level <LEVEL>   Set log level
    -p, --pcie                Enable PCIe transport
    -u, --udp                 Enable UDP transport
    -t, --tcp                 Enable TCP transport
    -c, --config-file <PATH>  Path to the gateway config file
```

If no transport options (i.e., `-p` and `-u`) are specified, all
available transport layers will be enabled. Also, note that only the transport
layers enabled during the build of the gateway binary (with CMake options) are
available for enabling.

### Usage as daemon

It is recommended to run the p3com gateway after boot as a daemon process on
all devices in the system. This way, all local iceoryx communication is
automatically forwarded between the nodes.

### Configuration file

The p3com gateway can load some configuration settings from a configuration
file in the TOML format. The default path of the configuration file is
`/etc/iceoryx/p3com.toml`, but it is also possible to set a custom path via the
`--config-file` option when running the gateway application.

There are currently two supported options in the configuration file. The first
one is `preferred-transport` which lets the user select the transport layer
which should be used when possible. This can be useful when multiple transport
layers should be enabled in the running p3com gateway for communication with
various device supporting various interfaces, but a single transport layer
should be preferred. By default, the PCIE transport layer is preferred over UDP
and TCP.

The second supported options is an array of tables `forwarded-service`, where
each table contains the keys `service`, `instance` and `event`. These are the
description of services to forward by the gateway.

You can find a sample of this file [here](./p3com.toml).

## Limitations

There are some limitations that should be kept in mind when using the p3com
gateway to achieve inter-device communication.

### The `hasSubscribers` method

The `iox::popo::BasePublisher::hasSubscribers` method might behave unexpectedly
in a system with the p3com gateway running. The gateway maintains a set of
internal publishers and subscribers for all active services on the system. So,
a user application publisher might appear to have matched subscribers, but
those are only the gateway-internal subscribers, instead of the expected
same-service subscribers in a remote user application.

### The UDP and TCP transport layers are not optimized

The p3com gateway support for the UDP and TCP transports is mostly experimental
and for debugging purposes. We are not aiming to optimize this feature, because
there exist many middleware solutions for networking communication, such as the
plenty DDS (e.g., Eclipse Cycloned DDS, FastDDS) or MQTT (e.g., Eclipse
Mosquitto) protocols implementations.

## Future work

### Support request-response model

With the Eclipse iceoryx 2.0.0 release, a new request-response API model was
added to complement the default publish-subscribe model. This is mostly about
the `Client` and `Server` types. Currently, p3com does not support forwarding
communication with this model, but it should be reasonably easy to implement
it.

### Improve configurability

It would be good to be able to configure further things in the p3com gateway,
in particular:

* Blocking behaviour of gateway-internal publishers and subscribers. This
concerns the `iox::popo::SubscriberTooSlowPolicy` and
`iox::popo::QueueFullPolicy` policies.
* Timeout durations. For example, there is a timeout specified when sending
data over the transport layers.
* PCIe DMA treshold. The PCIe transport has an empirically obtained treshold
number to be used for the decision whether to use the PCIe BAR windows for
message transfer or rather PCIe DMA engine. It could be useful if this can
configured by the user.
* Behaviour when gateway drops a sample. Most frequently, this concerns the
situation that the gateway received a sample over a transport layer, but it is
not able to publish it into the local iceoryx system, because the memory pool
is exhausted. It might try to wait, but this will block all receiving from that
transport layer. Alternatively, it can report this error in some way to the
user.

### Improve automatic transport switching on failure

There should be a mechanism to support the restoration of a failed transport
layer. For now, the failed transport is simply disabled forever and never used
again (until the gateway application is restarted).

### Add Github actions CI

As the title says.
