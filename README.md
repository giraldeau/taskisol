# taskisol
Task isolation experiments

## Compiling
That cannot be simpler:
```
make
```

To run taskisol, you need a kernel configured with `CONFIG_TASK_ISOLATION` from Chris Metcalf's dataplane branch

## Usage

```
taskisol [ mode ] [ event ]

mode: [ default | signal ]
      The default strict mode sends SIGKILL when the task enters
      the kernel or an interrupt disturbs the isolated task. With
      the signal mode, SIGUSR1 is called when the task exits
      userspace.

event: [ syscall | pagefault ]
      The syscall option calls write() after the task entered isolation state.
      The pagefault option forces a pagefault instead.
```

## Debug

To know why a task exits isolation, boot the kernel with `task_isolation_debug` parameter.

