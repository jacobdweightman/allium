# Debugging Guide

Currently, the best way to debug Allium programs is using execution traces.
This works in both the compiler and the interpreter, and the traces have the
same structure since programs follow the same execution model in both.

The verbosity of trace messages is determined by the log level. This table
details the trace messages which are logged for each log level:

| Log Level | Name  | Messages                      |
| --------- | ----- | ----------------------------- |
| 0         | OFF   | None                          |
| 1         | QUIET | OFF + handled effects         |
| 2         | LOUD  | QUIET + attempted predicates  |
| 3         | MAX   | LOUD + attempted implications |

# Debugging in the compiler

Execution traces in the compiler are slightly more involved, since the trace
messages are created by generating additional logging instrumentation code in
the resulting binary. The tracing feature should not impact the performance of
"release" artifacts, and so logging instrumentation should only be included by
opting in.

To include logging instrumentation, compile with the `-g` option. Then, to set
the log level, set the `ALLIUM_LOG_LEVEL` environment variable to 0 (no logs),
1, 2, or 3 (all logs). As an example, consider `Hello.allium`:
```
pred main: IO {
    main <- do print("Hello world!");
}
```

Effects aren't currently supported in the compiler so this program won't
actually work, but they are necessary to demonstrate all tracing messages. Until
they are supported, the following is purely illustrative.
```
$ allium Hello.allium -g
$ ALLIUM_LOG_LEVEL=0 ./a.out
Hello world!

$ ALLIUM_LOG_LEVEL=1 ./a.out
handle effect: do IO.print
Hello world!

$ ALLIUM_LOG_LEVEL=2 ./a.out
prove: main()
handle effect: do IO.print
Hello world!

$ ALLIUM_LOG_LEVEL=3 ./a.out
prove: main()
  try implication: main <- do IO.print("Hello world!")
handle effect: do IO.print
Hello world!
```

## Debugging in the interpreter

It is possible to print out execution traces in the interpreter by passing
`allium` the `--log-level` parameter. The log level is specified as a number
between 0 (no logs) and 3 (all logs). By default, the log level is 0.

Reconsider `Hello.allium` from the previous section. Logging at various levels
will produce the following output:
```
$ allium Hello.allium -i --log-level=0
Hello world!

$ allium Hello.allium -i --log-level=1
handle effect: do 0.0
Hello world!

$ allium Hello.allium -i --log-level=2
prove: main()
handle effect: do 0.0
Hello world!

$ allium Hello.allium -i --log-level=3
prove: main()
  try implication: 0() <- do 0.0
handle effect: do 0.0
Hello world!
```

Currently, the interpreter discards the names of effects during lowering. In the
future, we will store these into a table for clearer log messages.
