/*
 *                      /-----------------------\
 *                      | Software Oscilloscope |
 *                      \-----------------------/
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ********************************************************************
 *
 * See the man page for a description of what this program does and what
 * the requirements to run it are.
 * 
 * It was developed using:
 * - Linux kernel 1.0
 * - gcc 2.4.5
 * - svgalib version 1.05
 * - SoundBlaster Pro
 * - Trident VGA card
 * - 80386DX40 CPU with 8MB RAM
 * 
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <vga.h>
#include <sys/soundcard.h>

/* global variables */
int quit_key_pressed;       /* set by handle_key() */
int snd;                    /* file descriptor for sound device */
unsigned char buffer[1024]; /* buffer for sound data */
unsigned char old[1024];    /* previous buffer for sound data */
int offset;                 /* vertical offset */
int sampling = 8000;        /* selected sampling rate */
int actual;                 /* actual sampling rate */
int mode = G640x480x16;     /* graphics mode */
int colour = 2;             /* colour */
int dma = 4;                /* DMA buffer divisor */
int point_mode = 0;         /* point v.s. line segment mode */
int verbose = 0;            /* verbose mode */
int v_points;               /* points in vertical axis */
int h_points;               /* points in horizontal axis */
int trigger = -1;           /* trigger level (-1 = disabled) */
int graticule = 0;          /* show graticule */

/* display command usage on standard error and exit */
void usage()
{
  fprintf(stderr,
	  "usage: scope -r<rate> -m<mode> -c<colour> -d<dma divisor>\n"
	  "             -d<trigger> -p -l -g -v\n"
	  "Options:\n"
	  "-r <rate>        sampling rate in Hz\n"
	  "-m <mode>        graphics mode\n"
	  "-c <colour>      trace colour\n"
	  "-d <dma divide>  DMA buffer size divisor (1,2,4)\n"
	  "-t <trigger>     trigger level (0 - 255)\n"
	  "-p               point mode (faster)\n"
	  "-l               line segment mode (slower)\n"
	  "-g               draw graticule\n"
	  "-v               verbose output\n"
	  );
  exit(1);
}

/* if verbose mode, show current parameter settings on standard out */
inline void show_info() {
  if (verbose) {
    printf("graphics mode: %d\n", mode);
    printf("       colour: %d\n", colour);
    printf("  DMA divisor: %d\n", dma);
    if (point_mode)
      printf(" drawing mode: point\n");
    else
      printf(" drawing mode: line segment\n");
    if (graticule)
      printf("    graticule: on\n");
    else
      printf("    graticule: off\n");
    if (trigger == -1)
      printf("trigger level: disabled\n");
    else
      printf("trigger level: %d\n", trigger);
    printf("sampling rate: %d\n", sampling);
    printf("  actual rate: %d\n", actual);
  }
}

/* handle command line options */
void parse_args(int argc, char **argv)
{
  const char     *flags = "r:m:c:d:t:plgv";
  int             c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    switch (c) {
    case 'r':
      sampling = strtol(optarg, NULL, 0);
      break;
    case 'm':
      mode = strtol(optarg, NULL, 0);
      break;
    case 'c':
      colour = strtol(optarg, NULL, 0);
      break;
    case 'd':
      dma = strtol(optarg, NULL, 0);
      break;
    case 't':
      trigger = strtol(optarg, NULL, 0);
      break;
    case 'p':
      point_mode = 1;
      break;
    case 'l':
      point_mode = 0;
      break;
    case 'g':
      graticule = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case '?':
      usage();
      break;
    }
  }

}

/* initialize screen data to zero level */
void init_data()
{
  int i;
  
  for (i = 0 ; i < 1024 ; i++) {
    buffer[i] = 128;
    old[i] = 128;
  }
}

/* draw graticule */
inline void draw_graticule()
{
  register int i,k;

  vga_clear();

  /* draw a frame */
  vga_setcolor(colour+1);
  vga_drawline(0, offset-1, h_points-1, offset-1);
  vga_drawline(0, offset+256, h_points-1, offset+256);
  vga_drawline(0, offset-1, 0, offset+256);
  vga_drawline(h_points-1, offset, h_points-1, offset+256);

  if (actual) {
    /* draw tick marks at 1 msec intervals */
    for (i = 0; (k = i / 1000) < h_points-1  ; i += actual) {
      vga_drawline(k, offset, k, offset+5);
      vga_drawline(k, offset+251, k, offset+256);
    }
    /* draw vertical lines at 10 msec intervals */
    for (i = 0; (k = i / 100) < h_points-1  ; i += actual) {
      vga_drawline(k, offset, k, offset+256);
    }
  }

  /* draw a tick mark where the trigger level is */
  if (trigger != -1) {
    vga_drawline(0, offset+trigger, 3, offset+trigger);
  }
}

/* initialize graphics screen */
void init_screen()
{
  vga_disabledriverreport();
  vga_init();
  vga_setmode(mode);
  v_points = vga_getydim();
  h_points = vga_getxdim();
  offset = v_points / 2 - 127;
  if (graticule)
    draw_graticule();
}

/* cleanup: restore text mode and close sound device */
void cleanup()
{
  /* restore text screen */
  vga_setmode(TEXT);

  /* close sound device */
  close(snd);
}

/* initialize /dev/dsp */
void init_sound_card()
{
  int parm;
  int status;

  /* open DSP device for read */
  snd = open("/dev/dsp", O_RDONLY);
  if (snd < 0) {
    perror("cannot open /dev/dsp");
    cleanup();
    exit(1);
  }
    
  /* set mono */
  parm = 1;
  status = ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm);
  if (status < 0) {
    perror("error from sound device ioctl");
    cleanup();
    exit(1);
  }

  /* set 8-bit samples */
  parm = 8;
  status = ioctl(snd, SOUND_PCM_WRITE_BITS, &parm);
  if (status < 0) {
    perror("error from sound device ioctl");
    cleanup();
    exit(1);
  }

  /* set DMA buffer size */
  status = ioctl(snd, SOUND_PCM_SUBDIVIDE, &dma);
  if (status < 0) {
    perror("error from sound device ioctl");
    cleanup();
    exit(1);
  }

  /* set sampling rate */
  status = ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling);
  if (status < 0) {
    perror("error from sound device ioctl");
    cleanup();
    exit(1);
  }
}

/* handle single key commands */
inline void handle_key()
{
  switch (vga_getkey()) {
  case 0:
  case -1:
    /* no key pressed */
    return;
    break;
  case 'q':
  case 'Q':
    quit_key_pressed = 1;
    break;
  case 'R':
    sampling *= 1.1;
    ioctl(snd, SOUND_PCM_SYNC, 0);
    ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling);
    ioctl(snd, SOUND_PCM_READ_RATE, &actual);
    if (graticule)
      draw_graticule();
    break;
  case 'r':
    sampling *= 0.9;
    ioctl(snd, SOUND_PCM_SYNC, 0);
    ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling);
    ioctl(snd, SOUND_PCM_READ_RATE, &actual);
    if (graticule)
      draw_graticule();
    break;
  case 'T':
    if (trigger != -1) {
      trigger += 10;
      if (trigger > 255)
	trigger = 255;
      if (graticule)
	draw_graticule();
    }
    break;
  case 't':
    if (trigger != -1) {
      trigger -= 10;
      if (trigger < 0)
	trigger = 0;
    if (graticule)
      draw_graticule();
    }
    break;
  case 'l':
  case 'L':
    if (point_mode == 1) {
      point_mode = 0;
      vga_clear();
      if (graticule)
	draw_graticule();
    }
    break;
  case 'p':
  case 'P':
    if (point_mode == 0) {
      point_mode = 1;
      vga_clear();
      if (graticule)
	draw_graticule();
    }
    break;
  case 'C':
    colour++;
    if (graticule)
      draw_graticule();
    break;
  case 'c':
    if (colour > 0) {
      colour--;
      if (graticule)
	draw_graticule();
    }
    break;
  case 'G':
    graticule = 1;
    draw_graticule();
    break;
  case 'g':
    if (graticule == 1) {
      graticule = 0;
      vga_clear();
    }
    break;
  case ' ':
    /* pause until key pressed */
    while (vga_getkey() == 0)
      ;
    break;
  default:
    break;
  }
  show_info();
}

/* get data from sound card */
inline void get_data()
{
  unsigned char datum;
  
  /* simple trigger function */
  if (trigger != -1) {
    /* positive trigger */
    if (trigger >128)
      do {
	read(snd, &datum, 1);
      } while (datum < trigger);
    else
      /* negative trigger */
      do {
	read(snd, &datum, 1);
      } while (datum > trigger);
  }
  
  /* now get the real data */  
  read(snd, buffer, h_points-2);
  
}

/* graph the data */
inline void graph_data()
{
  register int i;

  if (point_mode) {
    for (i = 1; i < h_points-1  ; i++) {
      /* erase previous point */
      vga_setcolor(0);
      vga_drawpixel(i, old[i] + offset);
      /* draw new point */
      vga_setcolor(colour);
      vga_drawpixel(i, buffer[i] + offset);
      /* this becomes the old point next time */
      old[i] = buffer[i];
    }
  } else { /* line mode */
    for (i = 1; i < h_points-2  ; i++) {
      /* erase previous point */
      vga_setcolor(0);
      vga_drawline(i, old[i] + offset, i+1, old[i+1] + offset);
      /* draw new point */
      vga_setcolor(colour);
      vga_drawline(i, buffer[i] + offset, i+1, buffer[i+1] + offset);
      old[i] = buffer[i];
    }
    /* this becomes the old point next time */
    old[i] = buffer[i];
  }
}

/* main program */
int
main(int argc, char **argv)
{
  parse_args(argc, argv);
  init_screen();
  init_data();  
  init_sound_card();
  show_info();
  
  while (!quit_key_pressed) {
    handle_key();
    get_data();
    graph_data();
  }

  cleanup();
  exit(0);
}