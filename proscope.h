/*
 * @(#)$Id: proscope.h,v 1.5 2000/07/11 23:01:25 twitham Rel $
 *
 * Copyright (C) 1997 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines ProbeScope serial bits and function prototypes.
 *
 */

/* convenience/readability macro */
#define BIT_MASK(bitnum)	(1 << (bitnum))

/* Radio Shack ProbeScope serial data protocol bit definitions */

#define PS_WAIT		BIT_MASK(0) /* WAITING! for trigger? */

#define PS_10V		BIT_MASK(2) /* voltage switch */
#define PS_100V		BIT_MASK(3)
#define PS_AC		BIT_MASK(4) /* coupling switch */
#define PS_DC		BIT_MASK(5)

#define PS_SINGLE	BIT_MASK(0) /* trigger mode */
#define PS_MEXT		BIT_MASK(3) /* trigger type Plus/Minus Ext/Int */
#define PS_PEXT		BIT_MASK(4)
#define PS_MINT		BIT_MASK(5) /* (documentation was off-by-one here) */
#define PS_PINT		BIT_MASK(6)

#define PS_TP5		BIT_MASK(2) /* Trigger Plus/Minus 0.n */
#define PS_TP3		BIT_MASK(3)
#define PS_TP1		BIT_MASK(4)
#define PS_TM1		BIT_MASK(5)
#define PS_TM3		BIT_MASK(6)

#define PS_OVERFLOW	BIT_MASK(0)
#define PS_UNDERFLOW	BIT_MASK(1)
#define PS_MINUS	BIT_MASK(3)

#ifdef PSDEBUG
#undef PSDEBUG
#define PSDEBUG(format, arg) printf(format, arg)
#else
#define PSDEBUG(format, arg) ;
#endif

typedef struct ProbeScope {	/* The state of the ProbeScope */
  short found;
  short wait;
  short volts;
  char *coupling;
  unsigned char trigger;
  short level;
  short dvm;
  unsigned char flags;
} ProbeScope;
extern ProbeScope ps;

extern void init_probescope();
extern void probescope();
extern void init_serial();
extern void flush_serial();
extern void cleanup_serial();
extern int getonebyte();
