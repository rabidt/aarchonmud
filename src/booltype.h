#ifndef BOOLTYPE_H
#define BOOLTYPE_H

/* http://www.pixelbeat.org/programming/gcc/static_assert.html */
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  #define STATIC_ASSERT(e,m) \
    enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/static_cast<int>(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(int)(!!(e)) }
#endif

typedef unsigned char C_bool_t;
#define C_TRUE 1
#define C_FALSE 0

#if !defined(__cplusplus)  
  typedef C_bool_t bool;
  #define true C_TRUE
  #define false C_FALSE
#else
  /* verify compatibility between C and C++ booleans */
  STATIC_ASSERT(sizeof(C_bool_t) == sizeof(bool), C_CPP_bool_size_mismatch);
  
  STATIC_ASSERT(C_TRUE == true, C_CPP_true_equiv);
  STATIC_ASSERT(C_FALSE == false, C_CPP_false_equiv);

  STATIC_ASSERT((!C_TRUE) == C_FALSE, C_TRUE_not);
  STATIC_ASSERT((!C_TRUE) == false, C_TRUE_not);
  STATIC_ASSERT((!true) == C_FALSE, CPP_true_not);
  STATIC_ASSERT((!true) == false, CPP_true_not);

  STATIC_ASSERT((!C_FALSE) == C_TRUE, C_FALSE_not);
  STATIC_ASSERT((!C_FALSE) == true, C_FALSE_not);
  STATIC_ASSERT((!false) == C_TRUE, CPP_false_not);
  STATIC_ASSERT((!false) == true, CPP_false_not);

#endif // !defined(__cplusplus)


/* Values specifically for use with C_bool_t */
#define TRUE     true
#define FALSE    false


#undef ASSERT_CONCAT_
#undef ASSERT_CONCAT
#undef STATIC_ASSERT


#endif
