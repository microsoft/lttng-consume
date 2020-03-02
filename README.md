[![Build Status](https://dev.azure.com/ms/lttng-consume/_apis/build/status/microsoft.lttng-consume?branchName=master)](https://dev.azure.com/ms/lttng-consume/_build/latest?definitionId=149&branchName=master)

# lttng-consume

lttng-consume is a helper library that allows realtime consumption of LTTNG traces through the [JsonBuilder](https://github.com/Microsoft/JsonBuilder) interface.

## Examples

### Consuming LTTNG events via lttng-consume

```cpp
void RunConsumerTraceLogging(LttngConsume::LttngConsumer& consumer)
{
    consumer.StartConsuming([](JsonBuilder&& jsonBuilder) {
        JsonRenderer renderer;
        renderer.Pretty(true);

        std::string_view jsonString = renderer.Render(jsonBuilder);
        std::cout << jsonString << std::endl;
    });
}

void RunSample()
{
    constexpr auto hostName = "<host name goes here>";
    constexpr auto sessionName = "<lttng session name goes here>";

    std::string connectionString = "net://localhost/host/";
    connectionString += hostName;
    connectionString += "/";
    connectionString += sessionName;

    std::chrono::milliseconds pollInterval(50);

    LttngConsume::LttngConsumer consumer{ connectionString,
                                          pollInterval };

    std::thread consumptionThread{ RunConsumerTraceLogging,
                                   std::ref(consumer) };

    // Emit events here, wait, etc.

    consumer.StopConsuming();
    consumptionThread.join();
}
```

## Dependencies

This project carries on the following libraries:

1) [JsonBuilder](https://github.com/Microsoft/JsonBuilder)
2) uuid
3) babeltrace2 (Tested against 2.0.0 release)

Number 1 can be gotten at the URL above and Number 2 is available via apt:

```bash
sudo apt install uuid-dev
```

For #3, there is no package release, so it needs to be built from source.  There are
large breaking changes across different pre-release versions here.  lttng-consume currently
build against the v2.0.0-pre5 tag:

```bash
git clone -b v2.0.0-pre5 https://github.com/efficios/babeltrace/
sudo apt install autoconf autoheader libtool flex bison libglib2.0-dev libpopt-dev
./bootstrap
./configure --disable-debug-info --disable-man-pages --prefix=~/install/babeltrace
make
make install
```

## Integration

lttng-consume builds as a static or shared library and requires C++11. The project creates a CMake compatible 'lttng-consume' target which you can use for linking against the library.

1. Add this project as a subdirectory in your project, either as a git submodule or copying the code directly.
2. Add that directory to your top-level CMakeLists.txt with 'add_subdirectory'. This will make the target 'lttng-consume' available.  
3. Add the 'lttng-consume' target to the target_link_libraries of any target that will use lttng-consume.

## Reporting Security Issues

Security issues and bugs should be reported privately, via email, to the
Microsoft Security Response Center (MSRC) at <[secure@microsoft.com](mailto:secure@microsoft.com)>.
You should receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. Further information, including the
[MSRC PGP](https://technet.microsoft.com/en-us/security/dn606155) key, can be found in the
[Security TechCenter](https://technet.microsoft.com/en-us/security/default).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Contributing

Want to contribute? The team encourages community feedback and contributions. Please follow our [contributing guidelines](CONTRIBUTING.md).

We also welcome [issues submitted on GitHub](https://github.com/Microsoft/lttng-consume/issues).

## Project Status

This project is currently in active development.

## Contact

The easiest way to contact us is via the [Issues](https://github.com/microsoft/lttng-consume/issues) page.
