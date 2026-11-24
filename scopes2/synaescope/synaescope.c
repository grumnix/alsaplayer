/*  synaescope.c
 *  Copyright (C) 1999 Richard Boulton <richard@tartarus.org>
 *  Further support (C) 2002 Andy Lo A Foe <andy@loafoe.com>
 *
 *  Much code copied from Synaesthesia - a program to display sound
 *  graphically, by Paul Francis Harrison <pfh@yoyo.cc.monash.edu.au>
 *
 *  This file is part of AlsaPlayer.
 *
 *  AlsaPlayer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AlsaPlayer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
*/
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "scope_config.h"
#include "prefs.h"
#include "ap_unused.h"
#include "../scope_view.h"

#define FFT_BUFFER_SIZE_LOG 8
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)
typedef short int sound_sample;

#define syn_width 320
#define syn_height 200
#define brightMin 200
#define brightMax 2000
#define brightDec 10
#define brightInc 6
#define brTotTargetLow 5000
#define brTotTargetHigh 15000

static int autobrightness = 1;  /* Whether to use automatic brightness adjust */
static unsigned int brightFactor = 400;
/* Two bytes per pixel; faded a 32-bit word at a time in synaescope_coreGo() */
static guint32 output_words[syn_width * syn_height / 2];
static unsigned char * const output = (unsigned char *)output_words;
static sound_sample pcmt_l[FFT_BUFFER_SIZE];
static sound_sample pcmt_r[FFT_BUFFER_SIZE];
static sound_sample pcm_l[FFT_BUFFER_SIZE];
static sound_sample pcm_r[FFT_BUFFER_SIZE];
static double fftout_l[FFT_BUFFER_SIZE];
static double fftout_r[FFT_BUFFER_SIZE];
static double fftmult[FFT_BUFFER_SIZE / 2 + 1];
static double corr_l[FFT_BUFFER_SIZE];
static double corr_r[FFT_BUFFER_SIZE];
static int clarity[FFT_BUFFER_SIZE]; /* Surround sound */
static double cosTable[FFT_BUFFER_SIZE];
static double negSinTable[FFT_BUFFER_SIZE];
static int bitReverse[FFT_BUFFER_SIZE];
static int scaleDown[256];
static scope_view view = SCOPE_VIEW_INIT("Synaescope", syn_width, syn_height);
static pthread_t synaescope_thread;
static pthread_mutex_t pcm_mutex = PTHREAD_MUTEX_INITIALIZER;
static int running = 0;

static void synaes_fft(double *x, double *y);
static void synaescope_coreGo(void);
static int synaescope_running(void);

static void synaescope_set_data(void *audio_buffer, int size)
{
	short *sound = (short *)audio_buffer;
	if (pthread_mutex_trylock(&pcm_mutex) != 0)
		return;
	if (!sound) {
			memset(&pcmt_l, 0, sizeof(pcmt_l));
			memset(&pcmt_r, 0, sizeof(pcmt_r));
			pthread_mutex_unlock(&pcm_mutex);
			return;
	}
	if (running && size > FFT_BUFFER_SIZE * 2) {
		int i;
		sound_sample *newset_l = pcmt_l;
		sound_sample *newset_r = pcmt_r;
		sound += (size / 2 - FFT_BUFFER_SIZE) * 2; /* Use the very latest data */
		for (i=0; i < FFT_BUFFER_SIZE; i++) {
			*(newset_l++) = (sound_sample)(*(sound++));
			*(newset_r++) = (sound_sample)(*(sound++));
		}
	}
	pthread_mutex_unlock(&pcm_mutex);
}

static inline void addPixel(unsigned char *out, int x,int y,int br1,int br2) {
  unsigned char *p;
  if (x < 0 || x >= syn_width || y < 0 || y >= syn_height) return;

  p = out + x * 2 + y * syn_width * 2;
  if (p[0] < 255 - br1) p[0] += br1; else p[0] = 255;
  if (p[1] < 255 - br2) p[1] += br2; else p[1] = 255;
}

static inline void addPixelFast(unsigned char *p,int br1,int br2) {
  if (p[0] < 255 - br1) p[0] += br1; else p[0] = 255;
  if (p[1] < 255 - br2) p[1] += br2; else p[1] = 255;
}

static void synaescope_coreGo(void) {
  int i,j;
  guint32 *ptr;
  guint32 *end;
  int heightFactor;
  int actualHeight;
  int heightAdd;
  double brightFactor2;
  long int brtot;

  pthread_mutex_lock(&pcm_mutex);
  memcpy(pcm_l, pcmt_l, sizeof(pcm_l));
  memcpy(pcm_r, pcmt_r, sizeof(pcm_r));
  pthread_mutex_unlock(&pcm_mutex);

  for(i = 0; i < FFT_BUFFER_SIZE; i++) {
    fftout_l[i] = pcm_l[i];
    fftout_r[i] = pcm_r[i];
  }

  synaes_fft(fftout_l,fftout_r);

  for(i=0 +1;i<FFT_BUFFER_SIZE;i++) {
    double x1 = fftout_l[bitReverse[i]];
    double y_1 = fftout_r[bitReverse[i]];
    double x2 = fftout_l[bitReverse[FFT_BUFFER_SIZE-i]];
    double y2 = fftout_r[bitReverse[FFT_BUFFER_SIZE-i]];
    double aa,bb;
    corr_l[i] = sqrt(aa= (x1+x2)*(x1+x2) + (y_1-y2)*(y_1-y2) );
    corr_r[i] = sqrt(bb= (x1-x2)*(x1-x2) + (y_1+y2)*(y_1+y2) );
    clarity[i] = (int)(
      ( (x1+x2) * (x1-x2) + (y_1+y2) * (y_1-y2) )/(aa+bb) * 256 );
  }

  /* Asger Alstrupt's optimized 32 bit fade */
  /* (alstrup@diku.dk) */
  ptr = output_words;
  end = output_words + G_N_ELEMENTS(output_words);
  do {
    /* Bytewize version was: *(ptr++) -= *ptr+(*ptr>>1)>>4; */
    if (*ptr) {

      if (*ptr & 0xf0f0f0f0) {
        *ptr = *ptr - ((*ptr & 0xf0f0f0f0) >> 4) - ((*ptr & 0xe0e0e0e0) >> 5);
      } else {
        *ptr = (*ptr * 14 >> 4) & 0x0f0f0f0f;
            /* Should be 29/32 to be consistent. Who cares. This is totally */
            /* hacked anyway.  */
/*
	unsigned char *subptr = (unsigned char*)(ptr++);
        subptr[0] = (int)subptr[0] * 29 / 32;
        subptr[1] = (int)subptr[0] * 29 / 32;
        subptr[2] = (int)subptr[0] * 29 / 32;
        subptr[3] = (int)subptr[0] * 29 / 32;
*/
      }
    }
    ptr++;
  } while(ptr < end);

  heightFactor = FFT_BUFFER_SIZE/2 / syn_height + 1;
  actualHeight = FFT_BUFFER_SIZE/2 / heightFactor;
  heightAdd = (syn_height + actualHeight) >> 1;

  /* Correct for window size */
  brightFactor2 = (brightFactor/65536.0/FFT_BUFFER_SIZE)*
      sqrt(actualHeight * syn_width / (320.0*200.0));

  brtot = 0;
  for(i=1;i<FFT_BUFFER_SIZE/2;i++) {
    /* int h = (int)( corr_r[i]*280 / (corr_l[i]+corr_r[i]+0.0001)+20 ); */
    if (corr_l[i] > 0 || corr_r[i] > 0) {
      int h = (int)( corr_r[i] * syn_width / (corr_l[i]+corr_r[i]) );
      /* int h = (int)( syn_width - 1 ); */
      int br1, br2, br = (int)(
          (corr_l[i]+corr_r[i])*i*brightFactor2 );
      int px = h,
          py = heightAdd - i / heightFactor;
	  brtot += br;
      br1 = br*(clarity[i]+128)>>8;
      br2 = br*(128-clarity[i])>>8;
      if (br1 < 0) br1 = 0; else if (br1 > 255) br1 = 255;
      if (br2 < 0) br2 = 0; else if (br2 > 255) br2 = 255;
      /* unsigned char *p = output+ h*2+(164-((i<<8)>>FFT_BUFFER_SIZE_LOG))*(syn_width*2);  */

      if (px < 30 || py < 30 || px > syn_width-30 || py > syn_height-30) {
        addPixel(output, px,py,br1,br2);
        for(j=1;br1>0||br2>0;j++,br1=scaleDown[br1],br2=scaleDown[br2]) {
          addPixel(output, px+j,py,br1,br2);
          addPixel(output, px,py+j,br1,br2);
          addPixel(output, px-j,py,br1,br2);
          addPixel(output, px,py-j,br1,br2);
        }
      } else {
        unsigned char *p = output+px*2+py*syn_width*2, *p1=p, *p2=p, *p3=p, *p4=p;
        addPixelFast(p,br1,br2);
        for(;br1>0||br2>0;br1=scaleDown[br1],br2=scaleDown[br2]) {
          p1 += 2;
          addPixelFast(p1,br1,br2);
          p2 -= 2;
          addPixelFast(p2,br1,br2);
          p3 += syn_width * 2;
          addPixelFast(p3,br1,br2);
          p4 -= syn_width * 2;
          addPixelFast(p4,br1,br2);
        }
      }
    }
  }

	/* Apply autoscaling: makes quiet bits brighter, and loud bits
	 * darker, but still keeps loud bits brighter than quiet bits. */
	if(brtot != 0 && autobrightness) {
		long int brTotTarget = brTotTargetHigh;
		if(brightMax != brightMin) {
			brTotTarget -= ((brTotTargetHigh - brTotTargetLow) *
							(brightFactor - brightMin)) /
					       (brightMax - brightMin);
		}
		if(brtot < brTotTarget) {
			brightFactor += brightInc;
			if(brightFactor > brightMax) brightFactor = brightMax;
		} else {
			brightFactor -= brightDec;
			if(brightFactor < brightMin) brightFactor = brightMin;
		}
		/* printf("brtot: %ld\tbrightFactor: %d\tbrTotTarget: %d\n",
			   brtot, brightFactor, brTotTarget); */
	}
}

#define BOUND(x) ((x) > 255 ? 255 : (x))
#define PEAKIFY(x) BOUND((x) - (x)*(255-(x))/255/2)

static void the_synaescope(void)
{
	guint32 colEq[256];
	int i;

	for (i = 0; i < 256; i++) {
		colEq[i] = scope_rgb(PEAKIFY((i&15*16)),
				PEAKIFY((i&15)*16+(i&15*16)/4),
				PEAKIFY((i&15)*16));
	}

	while (running) {
		unsigned char *outptr = output;
		guint32 *bits;
		int w;

		synaescope_coreGo();

		bits = scope_view_lock(&view);
		if (bits) {
			for (w = 0; w < syn_width * syn_height; w++) {
				bits[w] = colEq[(outptr[0] >> 4) + (outptr[1] & 0xf0)];
				outptr += 2;
			}
		}
		scope_view_unlock_redraw(&view);
		dosleep(SCOPE_SLEEP);
	}
}

static void stop_synaescope(void);

static gboolean
close_synaescope_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_synaescope();
	return TRUE;
}


static void stop_synaescope(void)
{
	if (running) {
		running = 0;
		pthread_join(synaescope_thread, NULL);
	}
	scope_view_hide(&view);
}


static void *
run_synaescope(void * UNUSED (data))
{
	scope_thread_nice(); /* Be nice to most processes */
	the_synaescope();
	return NULL;
}


static void start_synaescope(void)
{
	if (running) {
		printf("synaescope already running\n");
		return;
	}
	scope_view_show(&view, G_CALLBACK(close_synaescope_window));
	running = 1;
	if (pthread_create(&synaescope_thread, NULL, run_synaescope, NULL) != 0)
		running = 0;
}


static int bitReverser(int i) {
    int sum = 0;
    int j;

    for(j = 0; j < FFT_BUFFER_SIZE_LOG; j++) {
	sum = (i & 1) + sum * 2;
	i >>= 1;
    }

    return sum;
}

static int
init_synaescope(void * UNUSED (arg))
{
    int i;

    for(i = 0; i <= FFT_BUFFER_SIZE / 2; i++) {
	double mult = (double)128 / ((FFT_BUFFER_SIZE * 16384) ^ 2);
	/* Result now guaranteed (well, almost) to be in range 0..128 */

	/* Low values represent more frequencies, and thus get more */
	/* intensity - this helps correct for that. */
	mult *= log(i + 1) / log(2);

	mult *= 3; /* Adhoc parameter, looks about right for me. */

	fftmult[i] = mult;
    }

	for(i = 0; i < FFT_BUFFER_SIZE; i++) {
		negSinTable[i] = -sin(M_PI * 2 / FFT_BUFFER_SIZE*i);
		cosTable[i] = cos(M_PI * 2 / FFT_BUFFER_SIZE*i);
		bitReverse[i] = bitReverser(i);
	}

	for(i=0;i<256;i++)
		scaleDown[i] = i*200>>8;

	memset(output, 0, syn_width * syn_height * 2);

	if (prefs_get_bool(ap_prefs, "synaescope", "active", 0))
		start_synaescope();

	return 1;
}

static void synaes_fft(double *x, double *y) {
	int n2 = FFT_BUFFER_SIZE;
	int n1;
	int twoToTheK;
	int j;
	for(twoToTheK = 1; twoToTheK < FFT_BUFFER_SIZE; twoToTheK *= 2) {
		n1 = n2;
		n2 /= 2;
		for(j = 0; j < n2; j++) {
			double c = cosTable[j * twoToTheK & (FFT_BUFFER_SIZE - 1)];
			double s = negSinTable[j * twoToTheK & (FFT_BUFFER_SIZE - 1)];
			int i;
			for(i = j; i < FFT_BUFFER_SIZE; i += n1) {
				int l = i + n2;
				double xt = x[i] - x[l];
				double yt = y[i] - y[l];
				x[i] = (x[i] + x[l]);
				y[i] = (y[i] + y[l]);
				x[l] = xt * c - yt * s;
				y[l] = xt * s + yt * c;
			}
		}
	}
}


static void shutdown_synaescope(void)
{
	prefs_set_bool(ap_prefs, "synaescope", "active", synaescope_running());

	stop_synaescope();
	scope_view_destroy(&view);
}


static int synaescope_running(void)
{
	return running;
}

scope_plugin synaescope_plugin = {
	SCOPE_PLUGIN_VERSION,
	"Synaescope",
	"Richard Boulton",
	NULL,
	init_synaescope,
	start_synaescope,
	synaescope_running,
	stop_synaescope,
	shutdown_synaescope,
	synaescope_set_data,
	NULL
};


scope_plugin *scope_plugin_info(void)
{
	return &synaescope_plugin;
}
