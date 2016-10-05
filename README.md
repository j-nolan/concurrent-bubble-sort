# Concurrent bubble sort
This is a C++ concurrent implementation of the bubble sort algorithm using the QT framework. Works with an arbitrary number of threads. Uses mutexes and semaphores to maintain integrity.

## Principle
For `N` threads, The list to sort is splitted into `N` segments, each sharing a value with its contiguous segments. Each thread sorts its segment, notifies threads responsible for contiguous segments and blocks on a semaphore. When a blocked thread is released, it checks whether the shared value has changed and, if so, performs a new sort. The list is sorted when all threads are blocked.

Concurrent programming lab at HEIG-VD - James Nolan and Mélanie Huck