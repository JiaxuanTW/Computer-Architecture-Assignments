# Set-associative Cache Simulator

## Arguments
```
Cache.exe trace_file cache_size block_size set_degree
```
* *trace_file*:  Relative path to the byte-address trace file.
* *cache_size*:  Size of the cache in KBytes.
* *block_size*:  Size of a cache block in Words (4 Bytes).
* *set_degree*:  Number of cache blocks in a set.

## Outputs
```
No    Status ByteAddr      BlockAddr     Index    Tag
------------------------------------------------------------
    1 [Miss] 0x011C8272 ->     291337 ->     9 ->       1138
    2 [Hit!] 0x011C8272 ->     291337 ->     9 ->       1138
    3 [Miss] 0x18F0B218 ->    6537928 ->   200 ->      25538
         .
         .
         .
 5003 [Hit!] 0x0DEF1C20 ->    3652720 ->   112 ->      14268

Total: 5003 / Hit: 3509 / Miss: 1494
MissRate: 0.298621

Process finished with exit code 0
```
