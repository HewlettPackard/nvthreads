# 1 "list_atomic.c"
# 1 "/mnt/ssd/terry/workspace/atlas/libs/libatomic_ops-7.4.0/tests//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "list_atomic.c"
# 1 "../src/atomic_ops.h" 1
# 26 "../src/atomic_ops.h"
# 1 "../src/atomic_ops/ao_version.h" 1
# 27 "../src/atomic_ops.h" 2



# 1 "/usr/include/assert.h" 1 3 4
# 35 "/usr/include/assert.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 374 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 1 3 4
# 385 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 386 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 375 "/usr/include/features.h" 2 3 4
# 398 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 399 "/usr/include/features.h" 2 3 4
# 36 "/usr/include/assert.h" 2 3 4
# 66 "/usr/include/assert.h" 3 4



extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));


extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));



# 31 "../src/atomic_ops.h" 2
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h" 1 3 4
# 147 "/usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h" 3 4
typedef long int ptrdiff_t;
# 212 "/usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 324 "/usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h" 3 4
typedef int wchar_t;
# 32 "../src/atomic_ops.h" 2
# 241 "../src/atomic_ops.h"
# 1 "../src/atomic_ops/sysdeps/gcc/x86.h" 1
# 24 "../src/atomic_ops/sysdeps/gcc/x86.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../all_aligned_atomic_load_store.h" 1
# 31 "../src/atomic_ops/sysdeps/gcc/../all_aligned_atomic_load_store.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_load.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_load.h"
static __inline size_t
AO_load(const volatile size_t *addr)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_load.h", 31, __PRETTY_FUNCTION__));



  return *(const size_t *)addr;
}
# 28 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/char_atomic_load.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/char_atomic_load.h"
static __inline unsigned char
AO_char_load(const volatile unsigned char *addr)
{





  return *(const unsigned char *)addr;
}
# 29 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_load.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_load.h"
static __inline unsigned short
AO_short_load(const volatile unsigned short *addr)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_load.h", 31, __PRETTY_FUNCTION__));



  return *(const unsigned short *)addr;
}
# 30 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_load.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_load.h"
static __inline unsigned
AO_int_load(const volatile unsigned *addr)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_load.h", 31, __PRETTY_FUNCTION__));



  return *(const unsigned *)addr;
}
# 30 "../src/atomic_ops/sysdeps/gcc/../all_atomic_only_load.h" 2
# 28 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 2

# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_store.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_store.h"
static __inline void
AO_store(volatile size_t *addr, size_t new_val)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/atomic_store.h", 31, __PRETTY_FUNCTION__));

  *(size_t *)addr = new_val;
}
# 30 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/char_atomic_store.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/char_atomic_store.h"
static __inline void
AO_char_store(volatile unsigned char *addr, unsigned char new_val)
{



  *(unsigned char *)addr = new_val;
}
# 31 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_store.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_store.h"
static __inline void
AO_short_store(volatile unsigned short *addr, unsigned short new_val)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/short_atomic_store.h", 31, __PRETTY_FUNCTION__));

  *(unsigned short *)addr = new_val;
}
# 32 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 2
# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_store.h" 1
# 27 "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_store.h"
static __inline void
AO_int_store(volatile unsigned *addr, unsigned new_val)
{

    ((((size_t)addr & (sizeof(*addr) - 1)) == 0) ? (void) (0) : __assert_fail ("((size_t)addr & (sizeof(*addr) - 1)) == 0", "../src/atomic_ops/sysdeps/gcc/../loadstore/int_atomic_store.h", 31, __PRETTY_FUNCTION__));

  *(unsigned *)addr = new_val;
}
# 32 "../src/atomic_ops/sysdeps/gcc/../all_atomic_load_store.h" 2
# 31 "../src/atomic_ops/sysdeps/gcc/../all_aligned_atomic_load_store.h" 2
# 25 "../src/atomic_ops/sysdeps/gcc/x86.h" 2

# 1 "../src/atomic_ops/sysdeps/gcc/../test_and_set_t_is_char.h" 1
# 30 "../src/atomic_ops/sysdeps/gcc/../test_and_set_t_is_char.h"
typedef enum {AO_BYTE_TS_clear = 0, AO_BYTE_TS_set = 0xff} AO_BYTE_TS_val;
# 27 "../src/atomic_ops/sysdeps/gcc/x86.h" 2







  static __inline void
  AO_nop_full(void)
  {
    __asm__ __volatile__("mfence" : : : "memory");
  }
# 52 "../src/atomic_ops/sysdeps/gcc/x86.h"
  static __inline size_t
  AO_fetch_and_add_full (volatile size_t *p, size_t incr)
  {
    size_t result;

    __asm__ __volatile__ ("lock; xadd %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr), "m" (*p)
                        : "memory");
    return result;
  }



static __inline unsigned char
AO_char_fetch_and_add_full (volatile unsigned char *p, unsigned char incr)
{
  unsigned char result;

  __asm__ __volatile__ ("lock; xaddb %0, %1" :
                        "=q" (result), "=m" (*p) : "0" (incr), "m" (*p)
                        : "memory");
  return result;
}


static __inline unsigned short
AO_short_fetch_and_add_full (volatile unsigned short *p, unsigned short incr)
{
  unsigned short result;

  __asm__ __volatile__ ("lock; xaddw %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr), "m" (*p)
                        : "memory");
  return result;
}




  static __inline void
  AO_and_full (volatile size_t *p, size_t value)
  {
    __asm__ __volatile__ ("lock; and %1, %0" :
                        "=m" (*p) : "r" (value), "m" (*p)
                        : "memory");
  }


  static __inline void
  AO_or_full (volatile size_t *p, size_t value)
  {
    __asm__ __volatile__ ("lock; or %1, %0" :
                        "=m" (*p) : "r" (value), "m" (*p)
                        : "memory");
  }


  static __inline void
  AO_xor_full (volatile size_t *p, size_t value)
  {
    __asm__ __volatile__ ("lock; xor %1, %0" :
                        "=m" (*p) : "r" (value), "m" (*p)
                        : "memory");
  }







static __inline AO_BYTE_TS_val
AO_test_and_set_full(volatile unsigned char *addr)
{
  unsigned char oldval;

  __asm__ __volatile__ ("xchgb %0, %1"
                        : "=q" (oldval), "=m" (*addr)
                        : "0" ((unsigned char)0xff), "m" (*addr)
                        : "memory");
  return (AO_BYTE_TS_val)oldval;
}




  static __inline int
  AO_compare_and_swap_full(volatile size_t *addr, size_t old, size_t new_val)
  {

      return (int)__sync_bool_compare_and_swap(addr, old, new_val
                                                                          );
# 155 "../src/atomic_ops/sysdeps/gcc/x86.h"
  }



static __inline size_t
AO_fetch_compare_and_swap_full(volatile size_t *addr, size_t old_val,
                               size_t new_val)
{

    return __sync_val_compare_and_swap(addr, old_val, new_val
                                                                  );
# 174 "../src/atomic_ops/sysdeps/gcc/x86.h"
}
# 276 "../src/atomic_ops/sysdeps/gcc/x86.h"
  static __inline unsigned int
  AO_int_fetch_and_add_full (volatile unsigned int *p, unsigned int incr)
  {
    unsigned int result;

    __asm__ __volatile__ ("lock; xaddl %0, %1"
                        : "=r" (result), "=m" (*p)
                        : "0" (incr), "m" (*p)
                        : "memory");
    return result;
  }
# 360 "../src/atomic_ops/sysdeps/gcc/x86.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../ordered_except_wr.h" 1
# 30 "../src/atomic_ops/sysdeps/gcc/../ordered_except_wr.h"
# 1 "../src/atomic_ops/sysdeps/gcc/../read_ordered.h" 1
# 30 "../src/atomic_ops/sysdeps/gcc/../read_ordered.h"
static __inline void
AO_nop_read(void)
{
  __asm__ __volatile__("" : : : "memory");
}


# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/ordered_loads_only.h" 1
# 37 "../src/atomic_ops/sysdeps/gcc/../read_ordered.h" 2
# 31 "../src/atomic_ops/sysdeps/gcc/../ordered_except_wr.h" 2

static __inline void
AO_nop_write(void)
{

  __asm__ __volatile__("" : : : "memory");


}


# 1 "../src/atomic_ops/sysdeps/gcc/../loadstore/ordered_stores_only.h" 1
# 42 "../src/atomic_ops/sysdeps/gcc/../ordered_except_wr.h" 2
# 360 "../src/atomic_ops/sysdeps/gcc/x86.h" 2
# 242 "../src/atomic_ops.h" 2
# 392 "../src/atomic_ops.h"
# 1 "../src/atomic_ops/generalize.h" 1
# 176 "../src/atomic_ops/generalize.h"
  static __inline void AO_nop(void) {}
# 303 "../src/atomic_ops/generalize.h"
# 1 "../src/atomic_ops/generalize-small.h" 1
# 295 "../src/atomic_ops/generalize-small.h"
  static __inline unsigned char
  AO_char_load_read(const volatile unsigned char *addr)
  {
    unsigned char result = AO_char_load(addr);

    AO_nop_read();
    return result;
  }
# 815 "../src/atomic_ops/generalize-small.h"
  static __inline unsigned short
  AO_short_load_read(const volatile unsigned short *addr)
  {
    unsigned short result = AO_short_load(addr);

    AO_nop_read();
    return result;
  }
# 1335 "../src/atomic_ops/generalize-small.h"
  static __inline unsigned
  AO_int_load_read(const volatile unsigned *addr)
  {
    unsigned result = AO_int_load(addr);

    AO_nop_read();
    return result;
  }
# 1855 "../src/atomic_ops/generalize-small.h"
  static __inline size_t
  AO_load_read(const volatile size_t *addr)
  {
    size_t result = AO_load(addr);

    AO_nop_read();
    return result;
  }
# 304 "../src/atomic_ops/generalize.h" 2

# 1 "../src/atomic_ops/generalize-arithm.h" 1
# 2689 "../src/atomic_ops/generalize-arithm.h"
  static __inline size_t
  AO_fetch_and_add_acquire(volatile size_t *addr, size_t incr)
  {
    size_t old;

    do
      {
        old = *(size_t *)addr;
      }
    while (__builtin_expect(!AO_compare_and_swap_full(addr, old, old + incr), 0)
                                                                          );
    return old;
  }





  static __inline size_t
  AO_fetch_and_add_release(volatile size_t *addr, size_t incr)
  {
    size_t old;

    do
      {
        old = *(size_t *)addr;
      }
    while (__builtin_expect(!AO_compare_and_swap_full(addr, old, old + incr), 0)
                                                                          );
    return old;
  }





  static __inline size_t
  AO_fetch_and_add(volatile size_t *addr, size_t incr)
  {
    size_t old;

    do
      {
        old = *(size_t *)addr;
      }
    while (__builtin_expect(!AO_compare_and_swap_full(addr, old, old + incr), 0)
                                                                  );
    return old;
  }
# 306 "../src/atomic_ops/generalize.h" 2
# 393 "../src/atomic_ops.h" 2
# 2 "list_atomic.c" 2
# 15 "list_atomic.c"
void list_atomic(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;



    (void)"AO_nop(): ";
    AO_nop();





    (void)"AO_load(&val):";
    AO_load(&val);




    (void)"AO_store(&val, newval):";
    AO_store(&val, newval);




    (void)"AO_fetch_and_add(&val, incr):";
    AO_fetch_and_add(&val, incr);




    (void)"AO_fetch_and_add1(&val):";
    AO_fetch_and_add(&val, 1);




    (void)"AO_fetch_and_sub1(&val):";
    AO_fetch_and_add(&val, (size_t)(-1));




    (void)"AO_and(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set(&ts):";
    AO_test_and_set_full(&ts);



}
# 132 "list_atomic.c"
void list_atomic_release(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;






    (void)"No AO_nop_release";






    (void)"No AO_load_release";


    (void)"AO_store_release(&val, newval):";
    (AO_nop_write(), AO_store(&val, newval));




    (void)"AO_fetch_and_add_release(&val, incr):";
    AO_fetch_and_add_release(&val, incr);




    (void)"AO_fetch_and_add1_release(&val):";
    AO_fetch_and_add_release(&val, 1);




    (void)"AO_fetch_and_sub1_release(&val):";
    AO_fetch_and_add_release(&val, (size_t)(-1));




    (void)"AO_and_release(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_release(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_release(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_release(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_release(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_release(&ts):";
    AO_test_and_set_full(&ts);



}
# 249 "list_atomic.c"
void list_atomic_acquire(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;






    (void)"No AO_nop_acquire";



    (void)"AO_load_acquire(&val):";
    AO_load_read(&val);







    (void)"No AO_store_acquire";


    (void)"AO_fetch_and_add_acquire(&val, incr):";
    AO_fetch_and_add_acquire(&val, incr);




    (void)"AO_fetch_and_add1_acquire(&val):";
    AO_fetch_and_add_acquire(&val, 1);




    (void)"AO_fetch_and_sub1_acquire(&val):";
    AO_fetch_and_add_acquire(&val, (size_t)(-1));




    (void)"AO_and_acquire(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_acquire(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_acquire(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_acquire(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_acquire(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_acquire(&ts):";
    AO_test_and_set_full(&ts);



}
# 366 "list_atomic.c"
void list_atomic_read(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;



    (void)"AO_nop_read(): ";
    AO_nop_read();





    (void)"AO_load_read(&val):";
    AO_load_read(&val);







    (void)"No AO_store_read";


    (void)"AO_fetch_and_add_read(&val, incr):";
    AO_fetch_and_add_full(&val, incr);




    (void)"AO_fetch_and_add1_read(&val):";
    AO_fetch_and_add_full(&val, 1);




    (void)"AO_fetch_and_sub1_read(&val):";
    AO_fetch_and_add_full(&val, (size_t)(-1));




    (void)"AO_and_read(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_read(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_read(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_read(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_read(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 483 "list_atomic.c"
void list_atomic_write(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;



    (void)"AO_nop_write(): ";
    AO_nop_write();
# 521 "list_atomic.c"
    (void)"No AO_load_write";


    (void)"AO_store_write(&val, newval):";
    (AO_nop_write(), AO_store(&val, newval));




    (void)"AO_fetch_and_add_write(&val, incr):";
    AO_fetch_and_add_full(&val, incr);




    (void)"AO_fetch_and_add1_write(&val):";
    AO_fetch_and_add_full(&val, 1);




    (void)"AO_fetch_and_sub1_write(&val):";
    AO_fetch_and_add_full(&val, (size_t)(-1));




    (void)"AO_and_write(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_write(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_write(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_write(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_write(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 600 "list_atomic.c"
void list_atomic_full(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;



    (void)"AO_nop_full(): ";
    AO_nop_full();





    (void)"AO_load_full(&val):";
    (AO_nop_full(), AO_load_read(&val));




    (void)"AO_store_full(&val, newval):";
    ((AO_nop_write(), AO_store(&val, newval)), AO_nop_full());




    (void)"AO_fetch_and_add_full(&val, incr):";
    AO_fetch_and_add_full(&val, incr);




    (void)"AO_fetch_and_add1_full(&val):";
    AO_fetch_and_add_full(&val, 1);




    (void)"AO_fetch_and_sub1_full(&val):";
    AO_fetch_and_add_full(&val, (size_t)(-1));




    (void)"AO_and_full(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_full(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_full(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_full(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_full(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_full(&ts):";
    AO_test_and_set_full(&ts);



}
# 717 "list_atomic.c"
void list_atomic_release_write(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;






    (void)"No AO_nop_release_write";






    (void)"No AO_load_release_write";


    (void)"AO_store_release_write(&val, newval):";
    (AO_nop_write(), AO_store(&val, newval));




    (void)"AO_fetch_and_add_release_write(&val, incr):";
    AO_fetch_and_add_full(&val, incr);




    (void)"AO_fetch_and_add1_release_write(&val):";
    AO_fetch_and_add_full(&val, 1);




    (void)"AO_fetch_and_sub1_release_write(&val):";
    AO_fetch_and_add_full(&val, (size_t)(-1));




    (void)"AO_and_release_write(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_release_write(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_release_write(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_release_write(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_release_write(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_release_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 834 "list_atomic.c"
void list_atomic_acquire_read(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;






    (void)"No AO_nop_acquire_read";



    (void)"AO_load_acquire_read(&val):";
    AO_load_read(&val);







    (void)"No AO_store_acquire_read";


    (void)"AO_fetch_and_add_acquire_read(&val, incr):";
    AO_fetch_and_add_full(&val, incr);




    (void)"AO_fetch_and_add1_acquire_read(&val):";
    AO_fetch_and_add_full(&val, 1);




    (void)"AO_fetch_and_sub1_acquire_read(&val):";
    AO_fetch_and_add_full(&val, (size_t)(-1));




    (void)"AO_and_acquire_read(&val, incr):";
    AO_and_full(&val, incr);




    (void)"AO_or_acquire_read(&val, incr):";
    AO_or_full(&val, incr);




    (void)"AO_xor_acquire_read(&val, incr):";
    AO_xor_full(&val, incr);




    (void)"AO_compare_and_swap_acquire_read(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_acquire_read(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 951 "list_atomic.c"
void list_atomic_dd_acquire_read(void)
{






    static volatile size_t val ;



    static size_t oldval ;




    static size_t newval ;


    unsigned char ts;



    static size_t incr ;






    (void)"No AO_nop_dd_acquire_read";



    (void)"AO_load_dd_acquire_read(&val):";
    AO_load(&val);







    (void)"No AO_store_dd_acquire_read";


    (void)"AO_fetch_and_add_dd_acquire_read(&val, incr):";
    AO_fetch_and_add(&val, incr);




    (void)"AO_fetch_and_add1_dd_acquire_read(&val):";
    AO_fetch_and_add(&val, 1);




    (void)"AO_fetch_and_sub1_dd_acquire_read(&val):";
    AO_fetch_and_add(&val, (size_t)(-1));







    (void)"No AO_and_dd_acquire_read";





    (void)"No AO_or_dd_acquire_read";





    (void)"No AO_xor_dd_acquire_read";


    (void)"AO_compare_and_swap_dd_acquire_read(&val, oldval, newval):";
    AO_compare_and_swap_full(&val, oldval, newval);






    (void)"AO_fetch_compare_and_swap_dd_acquire_read(&val, oldval, newval):";
    AO_fetch_compare_and_swap_full(&val, oldval, newval);





    (void)"AO_test_and_set_dd_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 1068 "list_atomic.c"
void char_list_atomic(void)
{






    static volatile unsigned char val ;
# 1085 "list_atomic.c"
    static unsigned char newval ;


    unsigned char ts;



    static unsigned char incr ;



    (void)"AO_nop(): ";
    AO_nop();





    (void)"AO_char_load(&val):";
    AO_char_load(&val);




    (void)"AO_char_store(&val, newval):";
    AO_char_store(&val, newval);




    (void)"AO_char_fetch_and_add(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and";





    (void)"No AO_char_or";





    (void)"No AO_char_xor";





    (void)"No AO_char_compare_and_swap";







    (void)"No AO_char_fetch_compare_and_swap";



    (void)"AO_test_and_set(&ts):";
    AO_test_and_set_full(&ts);



}
# 1185 "list_atomic.c"
void char_list_atomic_release(void)
{






    static volatile unsigned char val ;
# 1202 "list_atomic.c"
    static unsigned char newval ;


    unsigned char ts;



    static unsigned char incr ;






    (void)"No AO_nop_release";






    (void)"No AO_char_load_release";


    (void)"AO_char_store_release(&val, newval):";
    (AO_nop_write(), AO_char_store(&val, newval));




    (void)"AO_char_fetch_and_add_release(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_release(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_release(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_release";





    (void)"No AO_char_or_release";





    (void)"No AO_char_xor_release";





    (void)"No AO_char_compare_and_swap_release";







    (void)"No AO_char_fetch_compare_and_swap_release";



    (void)"AO_test_and_set_release(&ts):";
    AO_test_and_set_full(&ts);



}
# 1302 "list_atomic.c"
void char_list_atomic_acquire(void)
{






    static volatile unsigned char val ;
# 1322 "list_atomic.c"
    unsigned char ts;



    static unsigned char incr ;






    (void)"No AO_nop_acquire";



    (void)"AO_char_load_acquire(&val):";
    AO_char_load_read(&val);







    (void)"No AO_char_store_acquire";


    (void)"AO_char_fetch_and_add_acquire(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_acquire(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_acquire(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_acquire";





    (void)"No AO_char_or_acquire";





    (void)"No AO_char_xor_acquire";





    (void)"No AO_char_compare_and_swap_acquire";







    (void)"No AO_char_fetch_compare_and_swap_acquire";



    (void)"AO_test_and_set_acquire(&ts):";
    AO_test_and_set_full(&ts);



}
# 1419 "list_atomic.c"
void char_list_atomic_read(void)
{






    static volatile unsigned char val ;
# 1439 "list_atomic.c"
    unsigned char ts;



    static unsigned char incr ;



    (void)"AO_nop_read(): ";
    AO_nop_read();





    (void)"AO_char_load_read(&val):";
    AO_char_load_read(&val);







    (void)"No AO_char_store_read";


    (void)"AO_char_fetch_and_add_read(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_read(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_read(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_read";





    (void)"No AO_char_or_read";





    (void)"No AO_char_xor_read";





    (void)"No AO_char_compare_and_swap_read";







    (void)"No AO_char_fetch_compare_and_swap_read";



    (void)"AO_test_and_set_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 1536 "list_atomic.c"
void char_list_atomic_write(void)
{






    static volatile unsigned char val ;
# 1553 "list_atomic.c"
    static unsigned char newval ;


    unsigned char ts;



    static unsigned char incr ;



    (void)"AO_nop_write(): ";
    AO_nop_write();
# 1574 "list_atomic.c"
    (void)"No AO_char_load_write";


    (void)"AO_char_store_write(&val, newval):";
    (AO_nop_write(), AO_char_store(&val, newval));




    (void)"AO_char_fetch_and_add_write(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_write(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_write(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_write";





    (void)"No AO_char_or_write";





    (void)"No AO_char_xor_write";





    (void)"No AO_char_compare_and_swap_write";







    (void)"No AO_char_fetch_compare_and_swap_write";



    (void)"AO_test_and_set_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 1653 "list_atomic.c"
void char_list_atomic_full(void)
{






    static volatile unsigned char val ;
# 1670 "list_atomic.c"
    static unsigned char newval ;


    unsigned char ts;



    static unsigned char incr ;



    (void)"AO_nop_full(): ";
    AO_nop_full();





    (void)"AO_char_load_full(&val):";
    (AO_nop_full(), AO_char_load_read(&val));




    (void)"AO_char_store_full(&val, newval):";
    ((AO_nop_write(), AO_char_store(&val, newval)), AO_nop_full());




    (void)"AO_char_fetch_and_add_full(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_full(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_full(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_full";





    (void)"No AO_char_or_full";





    (void)"No AO_char_xor_full";





    (void)"No AO_char_compare_and_swap_full";







    (void)"No AO_char_fetch_compare_and_swap_full";



    (void)"AO_test_and_set_full(&ts):";
    AO_test_and_set_full(&ts);



}
# 1770 "list_atomic.c"
void char_list_atomic_release_write(void)
{






    static volatile unsigned char val ;
# 1787 "list_atomic.c"
    static unsigned char newval ;


    unsigned char ts;



    static unsigned char incr ;






    (void)"No AO_nop_release_write";






    (void)"No AO_char_load_release_write";


    (void)"AO_char_store_release_write(&val, newval):";
    (AO_nop_write(), AO_char_store(&val, newval));




    (void)"AO_char_fetch_and_add_release_write(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_release_write(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_release_write(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_release_write";





    (void)"No AO_char_or_release_write";





    (void)"No AO_char_xor_release_write";





    (void)"No AO_char_compare_and_swap_release_write";







    (void)"No AO_char_fetch_compare_and_swap_release_write";



    (void)"AO_test_and_set_release_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 1887 "list_atomic.c"
void char_list_atomic_acquire_read(void)
{






    static volatile unsigned char val ;
# 1907 "list_atomic.c"
    unsigned char ts;



    static unsigned char incr ;






    (void)"No AO_nop_acquire_read";



    (void)"AO_char_load_acquire_read(&val):";
    AO_char_load_read(&val);







    (void)"No AO_char_store_acquire_read";


    (void)"AO_char_fetch_and_add_acquire_read(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_acquire_read(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_acquire_read(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_acquire_read";





    (void)"No AO_char_or_acquire_read";





    (void)"No AO_char_xor_acquire_read";





    (void)"No AO_char_compare_and_swap_acquire_read";







    (void)"No AO_char_fetch_compare_and_swap_acquire_read";



    (void)"AO_test_and_set_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 2004 "list_atomic.c"
void char_list_atomic_dd_acquire_read(void)
{






    static volatile unsigned char val ;
# 2024 "list_atomic.c"
    unsigned char ts;



    static unsigned char incr ;






    (void)"No AO_nop_dd_acquire_read";



    (void)"AO_char_load_dd_acquire_read(&val):";
    AO_char_load(&val);







    (void)"No AO_char_store_dd_acquire_read";


    (void)"AO_char_fetch_and_add_dd_acquire_read(&val, incr):";
    AO_char_fetch_and_add_full(&val, incr);




    (void)"AO_char_fetch_and_add1_dd_acquire_read(&val):";
    AO_char_fetch_and_add_full(&val, 1);




    (void)"AO_char_fetch_and_sub1_dd_acquire_read(&val):";
    AO_char_fetch_and_add_full(&val, (unsigned char)(-1));







    (void)"No AO_char_and_dd_acquire_read";





    (void)"No AO_char_or_dd_acquire_read";





    (void)"No AO_char_xor_dd_acquire_read";





    (void)"No AO_char_compare_and_swap_dd_acquire_read";







    (void)"No AO_char_fetch_compare_and_swap_dd_acquire_read";



    (void)"AO_test_and_set_dd_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 2121 "list_atomic.c"
void short_list_atomic(void)
{






    static volatile unsigned short val ;
# 2138 "list_atomic.c"
    static unsigned short newval ;


    unsigned char ts;



    static unsigned short incr ;



    (void)"AO_nop(): ";
    AO_nop();





    (void)"AO_short_load(&val):";
    AO_short_load(&val);




    (void)"AO_short_store(&val, newval):";
    AO_short_store(&val, newval);




    (void)"AO_short_fetch_and_add(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and";





    (void)"No AO_short_or";





    (void)"No AO_short_xor";





    (void)"No AO_short_compare_and_swap";







    (void)"No AO_short_fetch_compare_and_swap";



    (void)"AO_test_and_set(&ts):";
    AO_test_and_set_full(&ts);



}
# 2238 "list_atomic.c"
void short_list_atomic_release(void)
{






    static volatile unsigned short val ;
# 2255 "list_atomic.c"
    static unsigned short newval ;


    unsigned char ts;



    static unsigned short incr ;






    (void)"No AO_nop_release";






    (void)"No AO_short_load_release";


    (void)"AO_short_store_release(&val, newval):";
    (AO_nop_write(), AO_short_store(&val, newval));




    (void)"AO_short_fetch_and_add_release(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_release(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_release(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_release";





    (void)"No AO_short_or_release";





    (void)"No AO_short_xor_release";





    (void)"No AO_short_compare_and_swap_release";







    (void)"No AO_short_fetch_compare_and_swap_release";



    (void)"AO_test_and_set_release(&ts):";
    AO_test_and_set_full(&ts);



}
# 2355 "list_atomic.c"
void short_list_atomic_acquire(void)
{






    static volatile unsigned short val ;
# 2375 "list_atomic.c"
    unsigned char ts;



    static unsigned short incr ;






    (void)"No AO_nop_acquire";



    (void)"AO_short_load_acquire(&val):";
    AO_short_load_read(&val);







    (void)"No AO_short_store_acquire";


    (void)"AO_short_fetch_and_add_acquire(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_acquire(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_acquire(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_acquire";





    (void)"No AO_short_or_acquire";





    (void)"No AO_short_xor_acquire";





    (void)"No AO_short_compare_and_swap_acquire";







    (void)"No AO_short_fetch_compare_and_swap_acquire";



    (void)"AO_test_and_set_acquire(&ts):";
    AO_test_and_set_full(&ts);



}
# 2472 "list_atomic.c"
void short_list_atomic_read(void)
{






    static volatile unsigned short val ;
# 2492 "list_atomic.c"
    unsigned char ts;



    static unsigned short incr ;



    (void)"AO_nop_read(): ";
    AO_nop_read();





    (void)"AO_short_load_read(&val):";
    AO_short_load_read(&val);







    (void)"No AO_short_store_read";


    (void)"AO_short_fetch_and_add_read(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_read(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_read(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_read";





    (void)"No AO_short_or_read";





    (void)"No AO_short_xor_read";





    (void)"No AO_short_compare_and_swap_read";







    (void)"No AO_short_fetch_compare_and_swap_read";



    (void)"AO_test_and_set_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 2589 "list_atomic.c"
void short_list_atomic_write(void)
{






    static volatile unsigned short val ;
# 2606 "list_atomic.c"
    static unsigned short newval ;


    unsigned char ts;



    static unsigned short incr ;



    (void)"AO_nop_write(): ";
    AO_nop_write();
# 2627 "list_atomic.c"
    (void)"No AO_short_load_write";


    (void)"AO_short_store_write(&val, newval):";
    (AO_nop_write(), AO_short_store(&val, newval));




    (void)"AO_short_fetch_and_add_write(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_write(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_write(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_write";





    (void)"No AO_short_or_write";





    (void)"No AO_short_xor_write";





    (void)"No AO_short_compare_and_swap_write";







    (void)"No AO_short_fetch_compare_and_swap_write";



    (void)"AO_test_and_set_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 2706 "list_atomic.c"
void short_list_atomic_full(void)
{






    static volatile unsigned short val ;
# 2723 "list_atomic.c"
    static unsigned short newval ;


    unsigned char ts;



    static unsigned short incr ;



    (void)"AO_nop_full(): ";
    AO_nop_full();





    (void)"AO_short_load_full(&val):";
    (AO_nop_full(), AO_short_load_read(&val));




    (void)"AO_short_store_full(&val, newval):";
    ((AO_nop_write(), AO_short_store(&val, newval)), AO_nop_full());




    (void)"AO_short_fetch_and_add_full(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_full(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_full(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_full";





    (void)"No AO_short_or_full";





    (void)"No AO_short_xor_full";





    (void)"No AO_short_compare_and_swap_full";







    (void)"No AO_short_fetch_compare_and_swap_full";



    (void)"AO_test_and_set_full(&ts):";
    AO_test_and_set_full(&ts);



}
# 2823 "list_atomic.c"
void short_list_atomic_release_write(void)
{






    static volatile unsigned short val ;
# 2840 "list_atomic.c"
    static unsigned short newval ;


    unsigned char ts;



    static unsigned short incr ;






    (void)"No AO_nop_release_write";






    (void)"No AO_short_load_release_write";


    (void)"AO_short_store_release_write(&val, newval):";
    (AO_nop_write(), AO_short_store(&val, newval));




    (void)"AO_short_fetch_and_add_release_write(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_release_write(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_release_write(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_release_write";





    (void)"No AO_short_or_release_write";





    (void)"No AO_short_xor_release_write";





    (void)"No AO_short_compare_and_swap_release_write";







    (void)"No AO_short_fetch_compare_and_swap_release_write";



    (void)"AO_test_and_set_release_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 2940 "list_atomic.c"
void short_list_atomic_acquire_read(void)
{






    static volatile unsigned short val ;
# 2960 "list_atomic.c"
    unsigned char ts;



    static unsigned short incr ;






    (void)"No AO_nop_acquire_read";



    (void)"AO_short_load_acquire_read(&val):";
    AO_short_load_read(&val);







    (void)"No AO_short_store_acquire_read";


    (void)"AO_short_fetch_and_add_acquire_read(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_acquire_read(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_acquire_read(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_acquire_read";





    (void)"No AO_short_or_acquire_read";





    (void)"No AO_short_xor_acquire_read";





    (void)"No AO_short_compare_and_swap_acquire_read";







    (void)"No AO_short_fetch_compare_and_swap_acquire_read";



    (void)"AO_test_and_set_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 3057 "list_atomic.c"
void short_list_atomic_dd_acquire_read(void)
{






    static volatile unsigned short val ;
# 3077 "list_atomic.c"
    unsigned char ts;



    static unsigned short incr ;






    (void)"No AO_nop_dd_acquire_read";



    (void)"AO_short_load_dd_acquire_read(&val):";
    AO_short_load(&val);







    (void)"No AO_short_store_dd_acquire_read";


    (void)"AO_short_fetch_and_add_dd_acquire_read(&val, incr):";
    AO_short_fetch_and_add_full(&val, incr);




    (void)"AO_short_fetch_and_add1_dd_acquire_read(&val):";
    AO_short_fetch_and_add_full(&val, 1);




    (void)"AO_short_fetch_and_sub1_dd_acquire_read(&val):";
    AO_short_fetch_and_add_full(&val, (unsigned short)(-1));







    (void)"No AO_short_and_dd_acquire_read";





    (void)"No AO_short_or_dd_acquire_read";





    (void)"No AO_short_xor_dd_acquire_read";





    (void)"No AO_short_compare_and_swap_dd_acquire_read";







    (void)"No AO_short_fetch_compare_and_swap_dd_acquire_read";



    (void)"AO_test_and_set_dd_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 3174 "list_atomic.c"
void int_list_atomic(void)
{






    static volatile unsigned val ;
# 3191 "list_atomic.c"
    static unsigned newval ;


    unsigned char ts;



    static unsigned incr ;



    (void)"AO_nop(): ";
    AO_nop();





    (void)"AO_int_load(&val):";
    AO_int_load(&val);




    (void)"AO_int_store(&val, newval):";
    AO_int_store(&val, newval);




    (void)"AO_int_fetch_and_add(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and";





    (void)"No AO_int_or";





    (void)"No AO_int_xor";





    (void)"No AO_int_compare_and_swap";







    (void)"No AO_int_fetch_compare_and_swap";



    (void)"AO_test_and_set(&ts):";
    AO_test_and_set_full(&ts);



}
# 3291 "list_atomic.c"
void int_list_atomic_release(void)
{






    static volatile unsigned val ;
# 3308 "list_atomic.c"
    static unsigned newval ;


    unsigned char ts;



    static unsigned incr ;






    (void)"No AO_nop_release";






    (void)"No AO_int_load_release";


    (void)"AO_int_store_release(&val, newval):";
    (AO_nop_write(), AO_int_store(&val, newval));




    (void)"AO_int_fetch_and_add_release(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_release(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_release(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_release";





    (void)"No AO_int_or_release";





    (void)"No AO_int_xor_release";





    (void)"No AO_int_compare_and_swap_release";







    (void)"No AO_int_fetch_compare_and_swap_release";



    (void)"AO_test_and_set_release(&ts):";
    AO_test_and_set_full(&ts);



}
# 3408 "list_atomic.c"
void int_list_atomic_acquire(void)
{






    static volatile unsigned val ;
# 3428 "list_atomic.c"
    unsigned char ts;



    static unsigned incr ;






    (void)"No AO_nop_acquire";



    (void)"AO_int_load_acquire(&val):";
    AO_int_load_read(&val);







    (void)"No AO_int_store_acquire";


    (void)"AO_int_fetch_and_add_acquire(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_acquire(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_acquire(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_acquire";





    (void)"No AO_int_or_acquire";





    (void)"No AO_int_xor_acquire";





    (void)"No AO_int_compare_and_swap_acquire";







    (void)"No AO_int_fetch_compare_and_swap_acquire";



    (void)"AO_test_and_set_acquire(&ts):";
    AO_test_and_set_full(&ts);



}
# 3525 "list_atomic.c"
void int_list_atomic_read(void)
{






    static volatile unsigned val ;
# 3545 "list_atomic.c"
    unsigned char ts;



    static unsigned incr ;



    (void)"AO_nop_read(): ";
    AO_nop_read();





    (void)"AO_int_load_read(&val):";
    AO_int_load_read(&val);







    (void)"No AO_int_store_read";


    (void)"AO_int_fetch_and_add_read(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_read(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_read(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_read";





    (void)"No AO_int_or_read";





    (void)"No AO_int_xor_read";





    (void)"No AO_int_compare_and_swap_read";







    (void)"No AO_int_fetch_compare_and_swap_read";



    (void)"AO_test_and_set_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 3642 "list_atomic.c"
void int_list_atomic_write(void)
{






    static volatile unsigned val ;
# 3659 "list_atomic.c"
    static unsigned newval ;


    unsigned char ts;



    static unsigned incr ;



    (void)"AO_nop_write(): ";
    AO_nop_write();
# 3680 "list_atomic.c"
    (void)"No AO_int_load_write";


    (void)"AO_int_store_write(&val, newval):";
    (AO_nop_write(), AO_int_store(&val, newval));




    (void)"AO_int_fetch_and_add_write(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_write(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_write(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_write";





    (void)"No AO_int_or_write";





    (void)"No AO_int_xor_write";





    (void)"No AO_int_compare_and_swap_write";







    (void)"No AO_int_fetch_compare_and_swap_write";



    (void)"AO_test_and_set_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 3759 "list_atomic.c"
void int_list_atomic_full(void)
{






    static volatile unsigned val ;
# 3776 "list_atomic.c"
    static unsigned newval ;


    unsigned char ts;



    static unsigned incr ;



    (void)"AO_nop_full(): ";
    AO_nop_full();





    (void)"AO_int_load_full(&val):";
    (AO_nop_full(), AO_int_load_read(&val));




    (void)"AO_int_store_full(&val, newval):";
    ((AO_nop_write(), AO_int_store(&val, newval)), AO_nop_full());




    (void)"AO_int_fetch_and_add_full(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_full(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_full(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_full";





    (void)"No AO_int_or_full";





    (void)"No AO_int_xor_full";





    (void)"No AO_int_compare_and_swap_full";







    (void)"No AO_int_fetch_compare_and_swap_full";



    (void)"AO_test_and_set_full(&ts):";
    AO_test_and_set_full(&ts);



}
# 3876 "list_atomic.c"
void int_list_atomic_release_write(void)
{






    static volatile unsigned val ;
# 3893 "list_atomic.c"
    static unsigned newval ;


    unsigned char ts;



    static unsigned incr ;






    (void)"No AO_nop_release_write";






    (void)"No AO_int_load_release_write";


    (void)"AO_int_store_release_write(&val, newval):";
    (AO_nop_write(), AO_int_store(&val, newval));




    (void)"AO_int_fetch_and_add_release_write(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_release_write(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_release_write(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_release_write";





    (void)"No AO_int_or_release_write";





    (void)"No AO_int_xor_release_write";





    (void)"No AO_int_compare_and_swap_release_write";







    (void)"No AO_int_fetch_compare_and_swap_release_write";



    (void)"AO_test_and_set_release_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 3993 "list_atomic.c"
void int_list_atomic_acquire_read(void)
{






    static volatile unsigned val ;
# 4013 "list_atomic.c"
    unsigned char ts;



    static unsigned incr ;






    (void)"No AO_nop_acquire_read";



    (void)"AO_int_load_acquire_read(&val):";
    AO_int_load_read(&val);







    (void)"No AO_int_store_acquire_read";


    (void)"AO_int_fetch_and_add_acquire_read(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_acquire_read(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_acquire_read(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_acquire_read";





    (void)"No AO_int_or_acquire_read";





    (void)"No AO_int_xor_acquire_read";





    (void)"No AO_int_compare_and_swap_acquire_read";







    (void)"No AO_int_fetch_compare_and_swap_acquire_read";



    (void)"AO_test_and_set_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 4110 "list_atomic.c"
void int_list_atomic_dd_acquire_read(void)
{






    static volatile unsigned val ;
# 4130 "list_atomic.c"
    unsigned char ts;



    static unsigned incr ;






    (void)"No AO_nop_dd_acquire_read";



    (void)"AO_int_load_dd_acquire_read(&val):";
    AO_int_load(&val);







    (void)"No AO_int_store_dd_acquire_read";


    (void)"AO_int_fetch_and_add_dd_acquire_read(&val, incr):";
    AO_int_fetch_and_add_full(&val, incr);




    (void)"AO_int_fetch_and_add1_dd_acquire_read(&val):";
    AO_int_fetch_and_add_full(&val, 1);




    (void)"AO_int_fetch_and_sub1_dd_acquire_read(&val):";
    AO_int_fetch_and_add_full(&val, (unsigned)(-1));







    (void)"No AO_int_and_dd_acquire_read";





    (void)"No AO_int_or_dd_acquire_read";





    (void)"No AO_int_xor_dd_acquire_read";





    (void)"No AO_int_compare_and_swap_dd_acquire_read";







    (void)"No AO_int_fetch_compare_and_swap_dd_acquire_read";



    (void)"AO_test_and_set_dd_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 4227 "list_atomic.c"
void double_list_atomic(void)
{
# 4247 "list_atomic.c"
    unsigned char ts;







    (void)"AO_nop(): ";
    AO_nop();
# 4265 "list_atomic.c"
    (void)"No AO_double_load";





    (void)"No AO_double_store";





    (void)"No AO_double_fetch_and_add";





    (void)"No AO_double_fetch_and_add1";





    (void)"No AO_double_fetch_and_sub1";





    (void)"No AO_double_and";





    (void)"No AO_double_or";





    (void)"No AO_double_xor";





    (void)"No AO_double_compare_and_swap";







    (void)"No AO_double_fetch_compare_and_swap";



    (void)"AO_test_and_set(&ts):";
    AO_test_and_set_full(&ts);



}
# 4344 "list_atomic.c"
void double_list_atomic_release(void)
{
# 4364 "list_atomic.c"
    unsigned char ts;
# 4375 "list_atomic.c"
    (void)"No AO_nop_release";






    (void)"No AO_double_load_release";





    (void)"No AO_double_store_release";





    (void)"No AO_double_fetch_and_add_release";





    (void)"No AO_double_fetch_and_add1_release";





    (void)"No AO_double_fetch_and_sub1_release";





    (void)"No AO_double_and_release";





    (void)"No AO_double_or_release";





    (void)"No AO_double_xor_release";





    (void)"No AO_double_compare_and_swap_release";







    (void)"No AO_double_fetch_compare_and_swap_release";



    (void)"AO_test_and_set_release(&ts):";
    AO_test_and_set_full(&ts);



}
# 4461 "list_atomic.c"
void double_list_atomic_acquire(void)
{
# 4481 "list_atomic.c"
    unsigned char ts;
# 4492 "list_atomic.c"
    (void)"No AO_nop_acquire";






    (void)"No AO_double_load_acquire";





    (void)"No AO_double_store_acquire";





    (void)"No AO_double_fetch_and_add_acquire";





    (void)"No AO_double_fetch_and_add1_acquire";





    (void)"No AO_double_fetch_and_sub1_acquire";





    (void)"No AO_double_and_acquire";





    (void)"No AO_double_or_acquire";





    (void)"No AO_double_xor_acquire";





    (void)"No AO_double_compare_and_swap_acquire";







    (void)"No AO_double_fetch_compare_and_swap_acquire";



    (void)"AO_test_and_set_acquire(&ts):";
    AO_test_and_set_full(&ts);



}
# 4578 "list_atomic.c"
void double_list_atomic_read(void)
{
# 4598 "list_atomic.c"
    unsigned char ts;







    (void)"AO_nop_read(): ";
    AO_nop_read();
# 4616 "list_atomic.c"
    (void)"No AO_double_load_read";





    (void)"No AO_double_store_read";





    (void)"No AO_double_fetch_and_add_read";





    (void)"No AO_double_fetch_and_add1_read";





    (void)"No AO_double_fetch_and_sub1_read";





    (void)"No AO_double_and_read";





    (void)"No AO_double_or_read";





    (void)"No AO_double_xor_read";





    (void)"No AO_double_compare_and_swap_read";







    (void)"No AO_double_fetch_compare_and_swap_read";



    (void)"AO_test_and_set_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 4695 "list_atomic.c"
void double_list_atomic_write(void)
{
# 4715 "list_atomic.c"
    unsigned char ts;







    (void)"AO_nop_write(): ";
    AO_nop_write();
# 4733 "list_atomic.c"
    (void)"No AO_double_load_write";





    (void)"No AO_double_store_write";





    (void)"No AO_double_fetch_and_add_write";





    (void)"No AO_double_fetch_and_add1_write";





    (void)"No AO_double_fetch_and_sub1_write";





    (void)"No AO_double_and_write";





    (void)"No AO_double_or_write";





    (void)"No AO_double_xor_write";





    (void)"No AO_double_compare_and_swap_write";







    (void)"No AO_double_fetch_compare_and_swap_write";



    (void)"AO_test_and_set_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 4812 "list_atomic.c"
void double_list_atomic_full(void)
{
# 4832 "list_atomic.c"
    unsigned char ts;







    (void)"AO_nop_full(): ";
    AO_nop_full();
# 4850 "list_atomic.c"
    (void)"No AO_double_load_full";





    (void)"No AO_double_store_full";





    (void)"No AO_double_fetch_and_add_full";





    (void)"No AO_double_fetch_and_add1_full";





    (void)"No AO_double_fetch_and_sub1_full";





    (void)"No AO_double_and_full";





    (void)"No AO_double_or_full";





    (void)"No AO_double_xor_full";





    (void)"No AO_double_compare_and_swap_full";







    (void)"No AO_double_fetch_compare_and_swap_full";



    (void)"AO_test_and_set_full(&ts):";
    AO_test_and_set_full(&ts);



}
# 4929 "list_atomic.c"
void double_list_atomic_release_write(void)
{
# 4949 "list_atomic.c"
    unsigned char ts;
# 4960 "list_atomic.c"
    (void)"No AO_nop_release_write";






    (void)"No AO_double_load_release_write";





    (void)"No AO_double_store_release_write";





    (void)"No AO_double_fetch_and_add_release_write";





    (void)"No AO_double_fetch_and_add1_release_write";





    (void)"No AO_double_fetch_and_sub1_release_write";





    (void)"No AO_double_and_release_write";





    (void)"No AO_double_or_release_write";





    (void)"No AO_double_xor_release_write";





    (void)"No AO_double_compare_and_swap_release_write";







    (void)"No AO_double_fetch_compare_and_swap_release_write";



    (void)"AO_test_and_set_release_write(&ts):";
    AO_test_and_set_full(&ts);



}
# 5046 "list_atomic.c"
void double_list_atomic_acquire_read(void)
{
# 5066 "list_atomic.c"
    unsigned char ts;
# 5077 "list_atomic.c"
    (void)"No AO_nop_acquire_read";






    (void)"No AO_double_load_acquire_read";





    (void)"No AO_double_store_acquire_read";





    (void)"No AO_double_fetch_and_add_acquire_read";





    (void)"No AO_double_fetch_and_add1_acquire_read";





    (void)"No AO_double_fetch_and_sub1_acquire_read";





    (void)"No AO_double_and_acquire_read";





    (void)"No AO_double_or_acquire_read";





    (void)"No AO_double_xor_acquire_read";





    (void)"No AO_double_compare_and_swap_acquire_read";







    (void)"No AO_double_fetch_compare_and_swap_acquire_read";



    (void)"AO_test_and_set_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
# 5163 "list_atomic.c"
void double_list_atomic_dd_acquire_read(void)
{
# 5183 "list_atomic.c"
    unsigned char ts;
# 5194 "list_atomic.c"
    (void)"No AO_nop_dd_acquire_read";






    (void)"No AO_double_load_dd_acquire_read";





    (void)"No AO_double_store_dd_acquire_read";





    (void)"No AO_double_fetch_and_add_dd_acquire_read";





    (void)"No AO_double_fetch_and_add1_dd_acquire_read";





    (void)"No AO_double_fetch_and_sub1_dd_acquire_read";





    (void)"No AO_double_and_dd_acquire_read";





    (void)"No AO_double_or_dd_acquire_read";





    (void)"No AO_double_xor_dd_acquire_read";





    (void)"No AO_double_compare_and_swap_dd_acquire_read";







    (void)"No AO_double_fetch_compare_and_swap_dd_acquire_read";



    (void)"AO_test_and_set_dd_acquire_read(&ts):";
    AO_test_and_set_full(&ts);



}
