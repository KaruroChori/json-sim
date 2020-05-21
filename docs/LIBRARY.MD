This is the documentation for the *Stochastic Simulator Agnostic Configuration Interface* (SSAGI) and not the simulator itself.
For that please refer to the `README.md` stored in the directory of the application.

**SSAGI** is a specification to represent configuration files used by stochastic simulators. Its structure is general enough that it is possible to accommodate any reasonable batch process you may want to execute. While it is possible to write the implementation for you application from scratch, this library should provide most of it for free, and let you focus on the implementation of few glue-classes. The official library is implemented in C++ and you will need a C++20 compliant compiler. It is possible to apply some minimal changes to the code to make it compatible with C++17 or even earlier but there is no interest on my side on doing that. **SSAGI** is based on **JSON Multi File** a specification for JSON file with support for split origins, automatic licensing and lazy archival. **v1** is enough to cover the needs of **SSAGI** but any real world application would probably benefit by using **v2** once a compatible implementation is ready.

The SSAGI implementation here provided is based on two external libraries:

* [Nlohmann JSON]: https://github.com/nlohmann/json to add support of JSON to C++.
* [CURL for People]: https://github.com/whoshuu/cpr is used to provide url support in callbacks.

## Known limitations
Because of the limited support of C++20 by any major compiler some parts of the code could not be fully implemented, for example those which were based on *source_location* and the new *concepts*.
At the moment no **JMF/v2**  implementation has been developed to work integrated with **nlohmann**.
The library is in pre-beta stage as the **SSAGI** specifications are not definitive yet. Future releases may break compatibility with the configuration files of older ones.

## Features
* Task batches to execute multiple instances sharing the same initial conditions in a scalable way.
* Callback mechanisms to notify single steps, instance completion, batch completion or the full completion of all simulation tasks. 
  Callback can be anything, usually bash scripts or remote urls. 
  So you can automatically process and group data, synchronize github repositories, provide notification and share your results online.
* Automatic backup so to have a recovery point if something goes wrong. 
  Simulators are not often the best written pieces of software and random segmentation faults are always around the corner. 
  In this way it will be possible to easily recover you simulation.
* Based on *JSON Multi File*.
* Native support for multithreading. You will be able to run multiple jobs at the same time automatically.
* Customizable! Adding feature to be used by your specific application is extremely simple.
* Designed for stochastic simulators but can be also used for any deterministic one.

# Syntax & Usage

## Output
The configuration is a JSON compliant file (RFC Something) which can be enhanced by *JSON Multi File*. Tasks are organized in batches which all share the same initial and final conditions. For each instance the following elements will be generated:
* A folder `%workspace/tasks/%group-name/%number`.
* A `.err` file representing the console error.
* A `.out` file representing the console output.
* `status` the file of the last synchronized system state.
* `status.copy` the backup of `status`
* An optional `trace` file only if *save-trace* is set *true*.
* An optional backup copy `trace.copy` of `trace`.
* An optional `mstatus` the status of the model in case the class has the capabilities and *save-model* is set *true*.
* An optional backup copy `mstatus.copy` of `mstatus`.

# Integration in you application
Integrating your application with *SSAGI* is simple, you only have to provide the implementation of few glue classes to have the minimal interface the library expects. Most of them are optional and in some cases a default implementation is already provided.
* *tweaks* (optional, only used if you want the configuration to have some configuration information passed down to your simulator)
* *model*
  - *state*
  - *delta_state* (application based choice, in most cases this will actually be a typedef for state but in the most exotic cases)
  - *end_condition* (a basic version simply counting the number of steps simulated or time elapsed is provided)
  - *model_state* (application based choice, it depends on the algorithms you are using to perform the simulations)
* *callback* (optional, a basic callback interface is already provided)