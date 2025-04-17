# Version History

[TOC]: #

- [Version 2.2](#version-22)
- [Version 2.1](#version-21)
- [Version 2.0.1](#version-201)
- [Version 2.0](#version-20)
- [Version 1.1](#version-11)
- [Version 1.0](#version-10)


## Version 2.2

* Change: rename `TinySwitcher` `Context` to `AsyncContext`
* Remove: `AsyncTask::isCurrentTask()` useless method. If
  `::isInAsyncContext()` is true then async yield can be performed.

## Version 2.1

* Change: implement C based `ByteQueue` to be used by all C++ classes
  for their queueing. The queue data buffer is at the end of the
  structure, eliminating need for indirection to access the elements.

## Version 2.0.1

* Change: simplify test for being in an async context by testing if have
  non-null current context in TinySwitcher. No need to test if it is of
  the current task. If there is an async context then can
  yieldContext().

## Version 2.0

* Add: `AsyncTask` type which can use yielding methods from within its
  loop to release the CPU to other tasks or the main loop. Execution of
  the code will continue after the call to the yielding method, when the
  scheduler resumes the task.

## Version 1.1

* Fix: after time slice is exceeded, advance to next task instead of
  trying to run the same task next time. It may just run out the clock
  every time, not letting any other tasks run.

## Version 1.0

* Initial Release

