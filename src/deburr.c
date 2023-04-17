#include "deburr.h"

VECTOR_SUITE(seat, struct cseat)
//VECTOR_SUITE(outp, struct coutp)

struct cstate {
  struct wl_display *dpy;
  struct wl_registry *reg;
  struct wl_shm *shm;
  struct wl_compositor *comp;
  struct xdg_wm_base *xwmBase;
  struct wl_region *oreg;
  struct xdg_surface *xsurf;
  struct xdg_toplevel *xtlev;
  struct zwlr_layer_shell_v1 *zwlr;
  struct zxdg_output_manager_v1 *xoutmgr;
  struct seatv seats;
  //struct outpv outs;
  struct wl_cursor_image *pImg;
  struct wl_surface *pSurf;
  struct cmon mon;

  uint32_t lframe;
  uint32_t width, height;
  uint8_t closed;
};
struct cstate state;
double ofs = 0;

// This section is here only because wayland refuses to allow NULL listeners, smh
// https://gitlab.freedesktop.org/wayland/wayland/-/issues/160                   
void zxout_logical_position(void *d, struct zxdg_output_v1 *z, int x, int y) { }
void zxout_logical_size(void *d, struct zxdg_output_v1 *z, int x, int y) { }
void zxout_done(void *d, struct zxdg_output_v1 *z) { }
void zxout_description(void *d, struct zxdg_output_v1 *z, const char *c) { }
void seat_name(void *d, struct wl_seat *s, const char *n) { }
void p_axis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v) { }
void p_axis_source(void *d, struct wl_pointer *p, uint32_t s) { }
void p_axis_stop(void *d, struct wl_pointer *p, uint32_t t, uint32_t s) { }
void p_axis_discrete(void *d, struct wl_pointer *p, uint32_t t, int s) { }
void reg_global_remove(void *data, struct wl_registry *reg, uint32_t name) { }

// Now onto the actual (shit) code

void xwmb_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}
const struct xdg_wm_base_listener xwmb_listener = { .ping = xwmb_ping };

void p_enter(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
  struct cseat *seat = data;
  seat->p.fmon = &state.mon; // TODO: getCurrentMonitor for multiple monitors
  if (!state.pImg) {
    struct wl_cursor_theme *cth = wl_cursor_theme_load(NULL, 32, state.shm);
    state.pImg = wl_cursor_theme_get_cursor(cth, "left_ptr")->images[0];
    state.pSurf = wl_compositor_create_surface(state.comp);
    wl_surface_attach(state.pSurf, wl_cursor_image_get_buffer(state.pImg), 0, 0);
    wl_surface_commit(state.pSurf);
  }
  wl_pointer_set_cursor(ptr, serial, state.pSurf, state.pImg->hotspot_x, state.pImg->hotspot_y);
}

void p_leave(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surf) {
  struct cseat *seat = data;
  seat->p.fmon = NULL;
}

void p_motion(void *data, struct wl_pointer *ptr, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
  struct cseat *seat = data;
  seat->p.x = wl_fixed_to_int(x);
  seat->p.y = wl_fixed_to_int(y);
}

void p_button(void *data, struct wl_pointer *ptr, uint32_t serial, uint32_t time, uint32_t button, uint32_t pressed) {
  struct cseat *seat = data;
  if (button == BTN_LEFT) {
    seat->p.cpres = (pressed == WL_POINTER_BUTTON_STATE_PRESSED);
  }
}

void p_frame(void *data, struct wl_pointer *ptr) {
  struct cseat *seat = data;
  if (!seat->p.fmon) {
    return;
  }
  /// TODO: HANDLE INPUT
  /*
  for (auto btn : seat.pointer->btns) {
    mon->bar.click(mon, seat.pointer->x, seat.pointer->y, btn);
  }
  seat.pointer->btns.clear();*/
}

const struct wl_pointer_listener pointer_listener = { .enter = p_enter, .leave = p_leave, .motion = p_motion, .button = p_button, .frame = p_frame, .axis = p_axis, .axis_source = p_axis_source, .axis_stop = p_axis_stop, .axis_discrete = p_axis_discrete };

void seat_caps(void *data, struct wl_seat *s, uint32_t caps) {
  struct cseat *seat = data;
  uint8_t hasPointer = caps & WL_SEAT_CAPABILITY_POINTER;
  if (!seat->p.p && hasPointer) {
    seat->p.p = wl_seat_get_pointer(seat->s);
    wl_pointer_add_listener(seat->p.p, &pointer_listener, &seat);
  }
}
const struct wl_seat_listener seat_listener = { .capabilities = seat_caps, .name = seat_name };

void zxout_name(void* data, struct zxdg_output_v1* xout, const char* name) {
  struct cmon *mon = data;
  mon->xdgname = name;
  zxdg_output_v1_destroy(xout);
}
const struct zxdg_output_v1_listener zxout_listener = { .name = zxout_name, .logical_position = zxout_logical_position, .logical_size = zxout_logical_size, .done = zxout_done, .description = zxout_description };

void gname(char *__restrict s) {
  uint32_t fnl = strlen(s);
  int32_t i;
  uint32_t r;
  for(i = 1; i <= 6; ++i) {
    r = random();
    s[fnl - i] = 'A' + (r & 15) + ((r & 16) << 1);
  }
}

int32_t cshmf(uint32_t size) {
  char fnm[] = "deburr-000000";

  int32_t fd = 0;
  uint32_t retries = 100;
  do {
    gname(fnm);
    //fprintf(stdout, "%s\n", fnm);
    fd = shm_open(fnm, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      break;
    }
  } while (--retries);
  if (retries == 0) {
    fprintf(stderr, "Could not create the shm file! [%m]\n");
    exit(1);
  }
  WLCHECKE(!shm_unlink(fnm),"Could not unlink the shm file!");
  WLCHECKE(!ftruncate(fd, size),"Could not truncate the shm file!");
  return fd;
}

void cbufs(struct cmon *mon, uint32_t width, uint32_t height, enum wl_shm_format form) {
  uint32_t stride = width * 4;
  uint32_t size = stride * height;
  uint32_t fd = cshmf(size * 2);
  struct wl_shm_pool *pool = wl_shm_create_pool(state.shm, fd, size * 2);
  uint32_t *data = mmap(NULL, size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  WLCHECK(data!=MAP_FAILED,"Could not mmap the shm data!");
  mon->sb.b[0] = wl_shm_pool_create_buffer(pool,    0, width, height, stride, form);
  WLCHECK(mon->sb.b[0],"Could not create the first shm buffer!");
  mon->sb.b[1] = wl_shm_pool_create_buffer(pool, size, width, height, stride, form);
  WLCHECK(mon->sb.b[1],"Could not create the second shm buffer!");
  wl_shm_pool_destroy(pool);

  mon->sb.fd = fd;
  mon->sb.data = data;
  mon->sb.size = size;
  mon->sb.width = width;
  mon->sb.height = height;
}

void render(struct cmon *mon) {
	if (!mon->sb.b[0]) { return; }

  /* Draw checkerboxed background */
  uint32_t co = mon->sb.size * mon->sb.csel / 4;
  uint32_t h = mon->sb.height;
  uint32_t w = mon->sb.width;
  fprintf(stdout, "Size %ux%u\n", mon->sb.width, mon->sb.height);
  fprintf(stdout, "Coffset %u\n", co);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if ((x + y / 8 * 8 + (uint32_t)ofs) % 16 < 8) {
        mon->sb.data[co + y * w + x] = 0xFF666666;
      } else {
        mon->sb.data[co + y * w + x] = 0xFFEEEEEE;
      }
    }
  }
  ofs += 0.5;

	wl_surface_attach(mon->surf, mon->sb.b[mon->sb.csel], 0, 0);
	wl_surface_damage(mon->surf, 0, 0, mon->sb.width, mon->sb.height);
	wl_surface_commit(mon->surf);
  mon->sb.csel = 1 - mon->sb.csel;
}

void zwlr_configure(void *data, struct zwlr_layer_surface_v1 *l, uint32_t serial, uint32_t width, uint32_t height) { 
  struct cmon *mon = data;
	zwlr_layer_surface_v1_ack_configure(l, serial);
	if (mon->sb.b[0] && width == mon->sb.width && height == mon->sb.height) { return; }
  cbufs(mon, width, height, WL_SHM_FORMAT_ARGB8888);
  render(mon);
}
void zwlr_closed(void *data, struct zwlr_layer_surface_v1 *l) { }
struct zwlr_layer_surface_v1_listener zwlr_listener = { .configure = zwlr_configure, .closed = zwlr_closed };

void finish_init() {
#define CHECK_INIT(x, e, v) {if (!state. x) { fputs("Your wayland compositor does not support " #e " version " #v "which is required for me to work :(\n", stderr); exit(1); }}
  CHECK_INIT(comp      , wl_compositor         , COMPV   );
  CHECK_INIT(shm       , wl_shm                , SHMV    );
  CHECK_INIT(zwlr      , zwlr_layer_shell_v1   , ZWLRV   );
  CHECK_INIT(xoutmgr   , zxdg_output_manager_v1, XOUTMGRV);
#undef CHECK_INIT

  seatvt(&state.seats);

  /// TODO: Add support for multiple monitors
  state.mon.xout = zxdg_output_manager_v1_get_xdg_output(state.xoutmgr, state.mon.out);
  zxdg_output_v1_add_listener(state.mon.xout, &zxout_listener, &state.mon.out);
	wl_display_roundtrip(state.dpy);
}

void reg_global(void *data, struct wl_registry *reg, uint32_t name, const char *iface, uint32_t ver) {
#define CHI(x,y,z,w) {if(!strcmp(iface,y .name)) {state. x=wl_registry_bind(reg, name, &y, z);w;return;}}
#define CHV(x,y,z,w) {if(!strcmp(iface,y .name)) {x cbind=wl_registry_bind(reg, name, &y, z);w;return;}}

  seatvi(&state.seats);

  CHI(comp           , wl_compositor_interface         , COMPV,);
  CHI(shm            , wl_shm_interface                , SHMV,);
  CHI(zwlr           , zwlr_layer_shell_v1_interface   , ZWLRV,);
  CHI(xwmBase        , xdg_wm_base_interface           , XWMBASEV,
      xdg_wm_base_add_listener(state.xwmBase, &xwmb_listener, NULL));
  CHI(xoutmgr        , zxdg_output_manager_v1_interface, XOUTMGRV,);
  CHV(struct wl_seat*, wl_seat_interface               , SEATV,
      struct cseat cs = {0};
      cs.n = name;
      cs.s = cbind;
      seatvp(&state.seats, cs);
      wl_seat_add_listener(cbind, &seat_listener, state.seats.v + state.seats.l - 1););
  CHV(struct wl_output*, wl_output_interface           , WOUTV,
      /// TODO: Handle multiple monitors
      struct cmon mon = {0};
      mon.n = name;
      mon.out = cbind;
      state.mon = mon;);
#undef CHI
#undef CHV
}
const struct wl_registry_listener reg_listener = { .global = reg_global, .global_remove = reg_global_remove};

void init_rand() {
  FILE *__restrict sr = fopen("/dev/urandom", "r");
  int32_t sd = 0;
  int32_t i;
  for(i = 0; i < 4; ++i) {
    sd = (sd << 8) + fgetc(sr);
  }
  srand(fgetc(sr));
  fclose(sr);
}

void frame_done(void *data, struct wl_callback *cb, uint32_t d) {
  struct cmon *mon = data;
  render(mon);
  wl_callback_destroy(cb);
}
const struct wl_callback_listener frame_listener = { .done = frame_done };

void redraw(struct cmon *mon) {
	struct wl_callback *frame = wl_surface_frame(mon->surf);
	wl_callback_add_listener(frame, &frame_listener, mon);
	wl_surface_commit(mon->surf);
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  init_rand();

  memset(&state, 0, sizeof(state));
  WLCHECK(state.dpy=wl_display_connect(NULL),"Could not connect to the wayland display!");
  WLCHECK(state.reg=wl_display_get_registry(state.dpy),"Could not fetch the wayland registry!");
  wl_registry_add_listener(state.reg, &reg_listener, NULL);
  wl_display_roundtrip(state.dpy);
  finish_init();

  state.mon.surf = wl_compositor_create_surface(state.comp); WLCHECK(state.mon.surf,"Cannot create wayland surface!");
	state.mon.lsurf = zwlr_layer_shell_v1_get_layer_surface(state.zwlr, state.mon.surf, state.mon.out, ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "Deburr"); WLCHECK(state.mon.lsurf,"Cannot create zwlr surface!");
	zwlr_layer_surface_v1_add_listener(state.mon.lsurf, &zwlr_listener, &state.mon);
	zwlr_layer_surface_v1_set_anchor(state.mon.lsurf, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

	zwlr_layer_surface_v1_set_size(state.mon.lsurf, 0, barHeight);
	zwlr_layer_surface_v1_set_keyboard_interactivity(state.mon.lsurf, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
	zwlr_layer_surface_v1_set_exclusive_zone(state.mon.lsurf, barHeight);
	wl_surface_commit(state.mon.surf);
  wl_display_dispatch(state.dpy);

  while (!state.closed) {
    wl_display_dispatch(state.dpy);
  }

  return 0;
}
