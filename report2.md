# Report 2: Thread-Safe Malloc

- Student Name: Yuchen Jiang
- Student NetID: yj210

## 1. General Implementation

### 1.1 How to add lock

We should use the following code to create a lock:

```C
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
```

For `lock_version`, we build upon the foundation of the previously implemented `bf_malloc` by adding locks before and after the `malloc` and `free` functions, treating their function bodies as the critical section. The rest of the code is allowed to run concurrently.

```C
void * ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  mem_blk * res = bf_malloc(size);
  pthread_mutex_unlock(&lock);
  return res;
}

void ts_free_lock(void * ptr){
  pthread_mutex_lock(&lock);
  simple_free(ptr);
  pthread_mutex_unlock(&lock);
}
```

For the nolock version, we only add locks before and after the call to the `sbrk` function, while no locks are added to other functions to ensure concurrency. To guarantee the correctness of memory operations, we create an independent `free list` for each process. Therefore, there will be no interference between processes.

```C
// free list for every thread
__thread mem_blk * global_head_origin = NULL;
```

### 1.2 Test Results

During the testing phase after completing the design, I conducted 50 rounds of `thread_test`, `thread_test_malloc_free`, `thread_test_measurement`, and `thread_test_malloc_free_change_thread` tests for each solution to mitigate the randomness of the output results in concurrent programs. In the 50 rounds of testing, both solutions passed all the tests.

Pass rate:
 
**NOLOCK_VERSION and LOCK_VERSION**

 * thread_test 50/50
 * thread_test_malloc_free 50/50
 * thread_test_malloc_free_change_thread 50/50
 * thread_test_measurement 50/50

## 2. Result & Analysis

Based on the 50 round tests, I conducted with the following results:
        
|  | LOCK | NO_LOCK |
| --- | --- | --- |
| Average Time | 0.989s | 0.237s |
| Average Size | 43924KB | 43631KB |

Since the no lock version only locks during the call to `sbrk`, its processing speed is significantly higher than that of the lock version. Additionally, because all threads in the lock version share a single free list, this may lead to the best fit traversal process requiring more time to find a suitable position.