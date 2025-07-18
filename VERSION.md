# Version History

[TOC]: #

- [Version 3.0](#version-30)
- [Version 2.2](#version-22)
- [Version 2.1](#version-21)
- [Version 2.0](#version-20)
- [Version 1.1](#version-11)
- [Version 1.0](#version-10)


## Version 3.0

* Change: microsecond granularity resume functions, simplifies the
  scheduler loop execution.

## Version 2.2

* Upcoming: change scheduler 3.0 to use microseconds from now or from
  start of task invocation for task resume delay.
* Add: `Controller`, `TwiController` to allow shared I2C write requests
  and shared byte buffer with resource reservation.
* Add: modified version of `twiint.c` to handle requests, debug traces.
* Add: `Dac53401` module to provide primitive function and command
  definitions.
* Add: `XL9535` I2C I/O expander module to communicate with chip.

## Version 2.1

* Change: renamed classes and methods, `AsyncTask`, `Mutex`, `Signal`,
  added byte resourceReq and byte resourceReq stream to use from C interrupts or C
  code.

## Version 2.0

* Add: `AsyncTask` type which can use yielding methods from within
  its loop to makeAvailable the CPU to other tasks or the main loop. Execution
  of the code will continue after the call to the yielding method, when
  the scheduler resumes the task.

## Version 1.1

* Fix: after time slice is exceeded, advance to next task instead of
  trying to run the same task next time. It may just run out the clock
  every time, not letting any other tasks run.

## Version 1.0

* Initial Release

