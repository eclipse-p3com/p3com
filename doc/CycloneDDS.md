# Integration to Eclipse Cyclone DDS

Eclipse Cyclone DDS implements optional shared memory communication based on
iceoryx. More information is available on the [Cyclone DDS documentation
website](https://cyclonedds.io/docs/cyclonedds/latest/shared_memory.html)

To run Cyclone DDS shared memory communication over the p3com gateway with full
performance benefits, you need to apply a patch to Cyclone DDS source code
which disables using network for user data. This is necessary, because Cyclone
DDS will otherwise use the usual network path to reach subscribers on remote
devices, it doesnt understand that iceoryx will deliver data to them. The patch
file is [cyclone_dds.patch][./cyclone_dds.patch).

After you have applied the patch, you need to re-build the Cyclone DDS library
`libddsc` with the standard build procedure.

## Run example

Now, we can try to run the `ShmThroughtput` example, which tests the throughput
of Cyclone DDS over shared memory, but now it will be also be running over p3com:

1) Copy the binaries `ShmThroughputSubscriber`, `ShmThroughputPublisher` and
`libddsc.so.0` to the target devices.
2) Set the `LD_LIBRARY_PATH` variable to point to the directory with `libddsc.so.0`.
3) According to the instructions
[here](https://cyclonedds.io/docs/cyclonedds/latest/shared_memory.html#developer-hints),
create the XML file `cyclonedds.xml` with the content:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<CycloneDDS xmlns="https://cdds.io/config"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="https://cdds.io/config https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/iceoryx/etc/cyclonedds.xsd">
    <Domain id="any">
        <SharedMemory>
            <Enable>true</Enable>
            <LogLevel>fatal</LogLevel>
        </SharedMemory>
    </Domain>
</CycloneDDS>
```
4) Export the environment variable `CYCLONEDDS_URI` to point to the XML file:
`export CYCLONEDDS_URI=<path>/cyclonedds.xml`
5) Run the shared memory demo, perhaps with 1 MB messages:
`./ShmThroughputPublisher 1048576 0 1 0 "Topic"` on one device and
`./ShmThroughputSubscriber 0 0 "Topic" 1048576` on another device. Note that
you might get a lot of output warnings in the form `Mempool [m_chunkSize =
16552, numberOfChunks = 100, used_chunks = 100 ] has no more space left`. That
is because the demo is trying to publish samples as fast as possible, which
will exhaust the iceoryx memory pool and some messages are thus discarded.

All limitations that apply to to shared memory usage in Cyclone DDS over
iceoryx also apply for p3com. In particular, the user should be aware of the
strict requirements on the QoS policies and data types that are needed to
enable shared memory transfers. These are described
[here](https://cyclonedds.io/docs/cyclonedds/latest/shared_memory.html#limitations).

### Limitations and future work

It would be very nice to enable a "hybrid" p3com acceleration of Cyclone DDS,
where DDS topics would have a QoS configuration property setting whether their
data sharing should be accelerated with p3com or rather the default networking
mechanism should be used. This would be useful to leverage the high throughput
of p3com on large-payload topics, while keeping the low latency of the
networking stack for small-payload topics.
