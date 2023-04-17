#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <linux/input.h>

#include <wayland-client.h>
#include <wayland-cursor.h>

#include "../protocol/xdgsh-protocol.h"
#include "../protocol/xout-protocol.h"
#include "../protocol/zwlr-protocol.h"

#include "config.h"
#include "vecs.h"

#define COMPV 4
#define SHMV 1
#define ZWLRV 4
#define XWMBASEV 2
#define XOUTMGRV 3
#define SEATV 7
#define WOUTV 1

struct sbuf {
  uint32_t height, width;
  uint32_t size;
  uint32_t fd;
  uint32_t *data;
  struct wl_buffer *__restrict b[2];
  uint8_t csel;
};

struct cfont {
};

struct cmon {
	uint32_t n;
  const char *xdgname;
  struct wl_output *out;
  struct zxdg_output_v1 *xout;
  struct zwlr_layer_surface_v1 *lsurf;
  struct wl_surface *surf;
	uint32_t tags;
  struct sbuf sb;
  struct cfont font;
};

struct cseatp {
  struct wl_pointer *p;
  struct cmon *fmon;
  int32_t x, y;
  uint8_t cpres;
};

struct cseat {
  uint32_t n;
  struct wl_seat *s;
  struct cseatp p;
};

struct coutp {
  uint32_t n;
  struct wl_output* o;
};

DEF_VECTOR_SUITE(seat, struct cseat)
//DEF_VECTOR_SUITE(outp, struct coutp)

#define FUNNIERCSTRING(x) #x
#define FUNNYCSTRING(x) FUNNIERCSTRING(x)
#define WLCHECK(x,e) {if(!(x)){fputs("Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e "\n", stderr); exit(1);}}
#define WLCHECKE(x,e) {if(!(x)){fprintf(stderr, "Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e " [%m]\n"); exit(1);}}
