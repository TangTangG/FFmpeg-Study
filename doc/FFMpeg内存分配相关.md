## 内存分配相关

### 关于size_t

size _t 这个类型在FFmpeg中多次出现，简单解释一下其作用。size _t是为了增强程序的可移植性而定义的。**不同系统上，定义size_t可能不一样**。它实际上就是unsigned int。

### 为什么要内存对齐

FFmpeg内存分配方面多次涉及到“内存对齐”（memory alignment）的概念。
这方面内容在IBM的网站上有一篇文章，讲的挺通俗易懂的，在此简单转述一下。
程序员通常认为内存就是一个字节数组，每次可以一个一个字节存取内存。例如在C语言中使用char *指代“一块内存”，Java中使用byte[]指代一块内存。如下所示。

![img](./20141116213746581)

但那实际上计算机处理器却不是这样认为的。处理器相对比较“懒惰”，它会以2字节，4字节，8字节，16字节甚至32字节来存取内存。例如下图显示了以4字节为单位读写内存的处理器“看待”上述内存的方式。

![img](./20141116213757782)

上述的存取单位的大小称之为内存存取粒度。
下面看一个实例，分别从地址0，和地址1读取4个字节到寄存器。
从程序员的角度来看，读取方式如下图所示。

![img](./20141116213810231)

而2字节存取粒度的处理器的读取方式如下图所示。

![img](./20141116213947169)

可以看出2字节存取粒度的处理器从地址0读取4个字节一共读取2次；从地址1读取4个字节一共读取了3次。由于每次读取的开销是固定的，因此从地址1读取4字节的效率有所下降。
4字节存取粒度的处理器的读取方式如下图所示。

![img](./20141116213956545)

可以看出4字节存取粒度的处理器从地址0读取4个字节一共读取1次；从地址1读取4个字节一共读取了2次。从地址1读取的开销比从地址0读取多了一倍。由此可见内存不对齐对CPU的性能是有影响的。

### av_malloc()

av_malloc()是FFmpeg中最常见的内存分配函数。

如果不考虑一大堆宏定义（即类似CONFIG_MEMALIGN_HACK这类的宏都采用默认值0），av_malloc()的代码可以简化成如下形式。

```c
void *av_malloc(size_t size)
{
    void *ptr = NULL;
    /* let's disallow possibly ambiguous cases */
    if (size > (max_alloc_size - 32))
        return NULL;
    ptr = malloc(size);
    if(!ptr && !size) {
        size = 1;
        ptr= av_malloc(1);
    }
    return ptr;

}
```

可以看出，此时的av_malloc()就是简单的封装了系统函数malloc()，并做了一些错误检查工作。

*注：malloc -> memory allocation，中文叫动态内存分配，用于申请一块连续的指定大小的内存块区域*

### av_realloc()

av_realloc()用于对申请的内存的大小进行调整。它去除一大堆宏定义，之后简化如下：

```c
void *av_realloc(void *ptr, size_t size)
{
    /* let's disallow possibly ambiguous cases */
    if (size > (max_alloc_size - 32))
        return NULL;
    return realloc(ptr, size + !size);

}
```

### av_mallocz()

av_mallocz()可以理解为av_malloc()+zero

```C
void *av_mallocz(size_t size)
{
    void *ptr = av_malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}
```

可以看出在分配了size内存之后，又调用memset()将分配的内存中的内容全部设置为指定的值，这里是0。

### av_calloc()

av_calloc()则是简单封装了av_mallocz()，定义如下所示

```C
void *av_calloc(size_t nmemb, size_t size)
{
    if (size <= 0 || nmemb >= INT_MAX / size)
        return NULL;
    return av_mallocz(nmemb * size);
}
```

### av_free()

av_free()用于释放申请的内存。它的定义如下
```C
void av_free(void *ptr)
{
    free(ptr);
}
```

### av_freep()

av_freep()简单封装了av_free()。并且在释放内存之后将目标指针设置为NULL。

```c
void av_freep(void *arg)
{
    void **ptr = (void **)arg;
    av_free(*ptr);
    *ptr = NULL;

}
```

