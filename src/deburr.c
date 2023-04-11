#include "deburr.h"

VECTOR_SUITE(seat, struct wcseat)
VECTOR_SUITE(outp, struct wcoutp)

struct cstate {
  struct wl_display *dpy;
  struct wl_registry *reg;
  struct wl_shm *shm;
  struct wl_compositor *comp;
  struct xdg_wm_base *xwmBase;
  struct wl_surface *wsurf;
  struct wl_region *oreg;
  struct xdg_surface *xsurf;
  struct xdg_toplevel *xtlev;
  struct zwlr_layer_shell_v1* zwlr;
  struct seatv seats;
  struct outpv outs;

  uint32_t lframe;
  uint32_t width, height;
  uint8_t closed;
};
struct cstate state;

void xwmbping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}
const struct xdg_wm_base_listener xwmblistener = { .ping = xwmbping, };

void reghand(void *data, struct wl_registry *reg, uint32_t name, const char *iface, uint32_t ver) {
#define CHI(x,y,z,w) {if(!strcmp(iface,y .name)) {state. x=wl_registry_bind(reg, name, &y, z);w;return;}}
#define CHV(x,y,z,w) {

  if(!strcmp(iface,y .name)) {
    wl_seat *r = wl_registry_bind(reg, name, )
  if (state. x .l == state. 
  state. x=wl_registry_bind(reg, name, &y, z);w;return;}}

  CHI(comp   , wl_compositor_interface         , 4,);
  CHI(shm    , wl_shm_interface                , 1,);
  CHI(zwlr   , zwlr_layer_shell_v1_interface   , 4,);
  CHI(xwmBase, xdg_wm_base_interface           , 2,
      xdg_wm_base_add_listener(state.xwmBase, &xwmblistener, NULL));
  CHI(xoutmgr, zxdg_output_manager_v1_interface, 3,);
  CHI(seat   , zxdg_output_manager_v1_interface, 3,);

	if (wl_seat* wlSeat; reg.handle(wlSeat, wl_seat_interface, 7)) {
		auto& seat = seats.emplace_back(Seat {name, wl_unique_ptr<wl_seat> {wlSeat}});
		wl_seat_add_listener(wlSeat, &seatListener, &seat);
		return;
	}
}
void regremhand(void *data, struct wl_registry *wl_registry, uint32_t name) { }
const struct wl_registry_listener reglistener = { .global = reghand, .global_remove = regremhand };

void wbufrel(void *data, struct wl_buffer *wbuf) { wl_buffer_destroy(wbuf); }
const struct wl_buffer_listener wbuflistener = { .release = wbufrel };

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

double ofs = 0;
struct wl_buffer *__restrict gframe() {
  uint32_t stride = state.width * 4;
  uint32_t size = stride * state.height;

  int32_t fd = cshmf(size);

  uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  WLCHECK(data!=MAP_FAILED,"Could not mmap the shm data!");

  struct wl_shm_pool *__restrict pool = wl_shm_create_pool(state.shm, fd, size);
  struct wl_buffer *__restrict buf = wl_shm_pool_create_buffer(pool, 0, state.width, state.height, stride, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  /* Draw checkerboxed background */
  for (int y = 0; y < state.height; ++y) {
    for (int x = 0; x < state.width; ++x) {
      if ((x + y / 8 * 8 + (uint32_t)ofs) % 16 < 8)
        data[y * state.width + x] = 0xFF666666;
      else
        data[y * state.width + x] = 0xFFEEEEEE;
    }
  }

  munmap(data, size);
  wl_buffer_add_listener(buf, &wbuflistener, NULL);
  return buf;
}

void xsurfconfig(void *data, struct xdg_surface *xsurf, uint32_t serial) {
  xdg_surface_ack_configure(xsurf, serial);

  struct wl_buffer *__restrict buf = gframe();
  wl_surface_attach(state.wsurf, buf, 0, 0);
  wl_surface_commit(state.wsurf);
}
const struct xdg_surface_listener xsurflistener = { .configure = xsurfconfig };

void wsurffdone(void *data, struct wl_callback *cb, uint32_t time);

const struct wl_callback_listener wsurfflistener = { .done = wsurffdone };

void xtlevconfigure(void *data, struct xdg_toplevel *xtlev, int32_t width, int32_t height, struct wl_array *states) {
	if (width == 0 || height == 0) {
		return;
	}
	state.width = width;
	state.height = height;
}
void xtlevclose(void *data, struct xdg_toplevel *toplevel) { state.closed = 1; }
const struct xdg_toplevel_listener xtlevlistener = { .configure = xtlevconfigure, .close = xtlevclose };

void wsurffdone(void *data, struct wl_callback *cb, uint32_t time) {
  wl_callback_destroy(cb);

  cb = wl_surface_frame(state.wsurf);
  wl_callback_add_listener(cb, &wsurfflistener, NULL);
  if (state.lframe) { ofs = (time - state.lframe / 1000.0 * 30); }
  state.lframe = time;

  struct wl_buffer *buf = gframe();
  wl_surface_attach(state.wsurf, buf, 0, 0);
  wl_surface_damage_buffer(state.wsurf, 0, 0, INT32_MAX, INT32_MAX);
  wl_surface_commit(state.wsurf);
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  init_rand();

  memset(&state, 0, sizeof(state));
  WLCHECK(state.dpy=wl_display_connect(NULL),"Could not connect to the wayland display!");
  WLCHECK(state.reg=wl_display_get_registry(state.dpy),"Could not fetch the wayland registry!");
  wl_registry_add_listener(state.reg, &reglistener, NULL);
  wl_display_roundtrip(state.dpy);
  WLCHECK(state.shm,"Could not connect to the wayland shm interface!");
  WLCHECK(state.comp,"Could not connect with the wayland compositor!");
  WLCHECK(state.xwmBase,"Could not connect to the xdg wm base interface!");
  state.width = 500;
  state.height = 500;

  state.wsurf = wl_compositor_create_surface(state.comp);
  state.xsurf = xdg_wm_base_get_xdg_surface(state.xwmBase, state.wsurf);
  xdg_surface_add_listener(state.xsurf, &xsurflistener, NULL);
  state.xtlev = xdg_surface_get_toplevel(state.xsurf);
	xdg_toplevel_add_listener(state.xtlev, &xtlevlistener, NULL);
  xdg_toplevel_set_title(state.xtlev, "Deburr");
  xdg_toplevel_set_app_id(state.xtlev, "Deburr");
  wl_surface_commit(state.wsurf);

  struct wl_callback *cb = wl_surface_frame(state.wsurf);
  wl_callback_add_listener(cb, &wsurfflistener, NULL);

  state.oreg = wl_compositor_create_region(state.comp);
  wl_region_add(state.oreg, 0, 0, 10, 10);
  wl_surface_set_opaque_region(state.wsurf, state.oreg);

  while (!state.closed) {
    wl_display_dispatch(state.dpy);
  }

  return 0;
}
