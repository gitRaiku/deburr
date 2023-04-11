#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <wayland-client.h>

#include "../protocol/xdgsh-protocol.h"
#include "../protocol/xout-protocol.h"
#include "../protocol/zwlr-protocol.h"

#include "vecs.h"

struct cseat {
  uint32_t n;
  struct wl_seat* s;
};

struct coutp {
  uint32_t n;
  struct wl_output* o;
};

DEF_VECTOR_SUITE(seat, struct wcseat)
DEF_VECTOR_SUITE(outp, struct wcoutp)

#define FUNNIERCSTRING(x) #x
#define FUNNYCSTRING(x) FUNNIERCSTRING(x)
#define WLCHECK(x,e) {if(!(x)){fputs("Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e "\n", stderr); exit(1);}}
#define WLCHECKE(x,e) {if(!(x)){fprintf(stderr, "Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e " [%m]\n"); exit(1);}}
