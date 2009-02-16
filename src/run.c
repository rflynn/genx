
#include <assert.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "typ.h"

extern int Dump;
extern struct target_f Target[];
extern u32 TargetLen;

/**
 * given a candidate function and an input, pass input as a parameter
 * and execute 'f'
 */
static u32 shim_i(const void *f, u32 in)
{
  u32 out;
  asm volatile(
    /* save and zero regs to prevent cheating by called function */
    /* note: eax contains param 'in' */
    "push %%ebx;"
    "push %%ecx;"
    "push %%edx;"
    //"xor  %%eax, %%eax;"
    "xor  %%ebx, %%ebx;"
    "xor  %%ecx, %%ecx;"
    "xor  %%edx, %%edx;"
    /* pass in first parameter */
    "movl %1, (%%esp);"
    /* call function pointer */
    "call *%2;"
    "pop  %%edx;"
    "pop  %%ecx;"
    "pop  %%ebx;"
    : "=a"(out)
    : "r"(in), "m"(f));
  return out;
}

/**
 * given a candidate function and an input, pass input as a parameter
 * and execute 'f'
 */
static float shim_f(const void *f, float in)
{
  float out;
  asm volatile(
    /* pass in first parameter */
    "fld  %1;"
    "fstp (%%esp);"
    /* call function pointer */
    "call *%2;"
    /* store st(0) in out */
    "fst   %0;"
    /*
     * clear entire (8 member) x86 float stack
     * every time, lest it eventually fill up.
     * this took a while to figure out :-/
     */
    "ffree %%st(0);"
    "ffree %%st(1);"
    "ffree %%st(2);"
    "ffree %%st(3);"
    "ffree %%st(4);"
    "ffree %%st(5);"
    "ffree %%st(6);"
    "ffree %%st(7);"
    : "=m"(out)
    : "m"(in), "r"(f));
  return out;
}

/**
 * given a candidate function, test it against all input and return a
 * score -- a distance from the ideal output.
 * a score of 0 indicates a perfect match against the test input
 */
float score_f(const void *f, int verbose)
{
  float scor = 0.f;
  u32 i;
  if (verbose || Dump >= 2)
    printf("%11s %11s %11s %11s %11s\n"
           "----------- ----------- ----------- ----------- -----------\n",
           "in", "target", "actual", "diff", "diffsum");
  for (i = 0; i < TargetLen; i++) {
    float diff,   /* difference between target and sc */
          sc;
    if (verbose || Dump >= 2)
      printf("%11g %11g ", Target[i].in, Target[i].out);
    sc = shim_f(f, Target[i].in);
    if (verbose || Dump >= 2)
      printf("%11g ", sc);
    if (!isfinite(sc)) {
      scor = FLT_MAX;
      if (verbose || Dump >= 2)
        fputc('\n', stdout);
      break;
    }
    diff = Target[i].out - sc;
    diff = fabs(diff);
    /* detect overflow */
    if (FLT_MAX - diff < scor) {
      scor = FLT_MAX;
      break;
    }
    scor += diff;
    if (verbose || Dump >= 2)
      printf("%11g %11g\n", diff, scor);
  }
  if (verbose || Dump >= 2)
    printf("score=%g\n", scor);
  return scor;
}


