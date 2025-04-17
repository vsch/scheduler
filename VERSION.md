# Version History

[TOC]: #

- [Version 2.0.1](#version-201)
- [Version 2.0](#version-20)
- [Version 1.1](#version-11)
- [Version 1.0](#version-10)


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

