Demo application for a simulator interface. The real one needs proper implementations for the classes which are here introduced.

Simulator
=========

This is the actual simulator. It does not accept parameters on the command line but only an input redirection of a JSON object containing all the relevant information to initialize and run the simulation.

Usage:
```
echo "{}" | simulator
```
or:
```
cat file.json | simulator
```

There is no further input required. By default output is emitted according to the configuration JSON object. **cout** is still being used for debugging while **cerr** will display errors.