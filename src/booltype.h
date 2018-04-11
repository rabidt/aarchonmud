#ifndef BOOLTYPE_H_
#define BOOLTYPE_H_


typedef unsigned char C_bool_t;
#define C_TRUE 1
#define C_FALSE 0

#if !defined(__cplusplus)  
  typedef C_bool_t bool;
  #define true C_TRUE
  #define false C_FALSE
#else
  /* verify compatibility between C and C++ booleans */
  static_assert(sizeof(C_bool_t) == sizeof(bool), "");
  
  static_assert(C_TRUE == true, "");
  static_assert(C_FALSE == false, "");

  static_assert((!C_TRUE) == C_FALSE, "");
  static_assert((!C_TRUE) == false, "");
  static_assert((!true) == C_FALSE, "");
  static_assert((!true) == false, "");

  static_assert((!C_FALSE) == C_TRUE, "");
  static_assert((!C_FALSE) == true, "");
  static_assert((!false) == C_TRUE, "");
  static_assert((!false) == true, "");

#endif // !defined(__cplusplus)


/* Values specifically for use with C_bool_t */
#define TRUE     true
#define FALSE    false


#endif // BOOLTYPE_H_
