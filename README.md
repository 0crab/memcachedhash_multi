多线程多次使用“set”插入memcached底层哈希表测试

```
gcc -std=c99 main.c assoc.c jenkins_hash.c my_memcached.c -lpthread
```

set方法包含delete和insert。直接对整个item进行操作。并且在每次set前都要上锁。

---

***iclab server5*** （hash size 1亿）

单线程100w：

- 1: 0.455s
- 4：0.899s
- 8：1.425s
- 16：2.207s
- 32: 3.434s

单线程1000w：

- 1:3.209s

- 4：6.208s

- 8：8.268s

- 16:19.947s

- 32:33.394s

---

由于锁的存在，导致哈希表的多线程性能不佳。
