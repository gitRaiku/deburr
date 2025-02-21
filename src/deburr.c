#include "deburr.h"

VECTOR_SUITE(seat, struct cseat)
//VECTOR_SUITE(outp, struct coutp)

FILE *__restrict errf;
struct fontInfo {
  char *fname;
  FT_F26Dot6 xsize, ysize;  /* pixel size */
  FcBool     aa;            /* doing antialiasing */
  FcBool     embold;        /* force emboldening */
  FcBool     color;         /* contains color glyphs */
  int        rgba;          /* subpixel order */
  int        lcd_filter;    /* lcd filter */
  FcBool     hinting;       /* non-identify matrix? */
  FT_Int     load_flags;    /* glyph load flags */

  /*
   * Internal fields
   */
  int      spacing;
  FcBool    minspace;
  int      cwidth;
};

struct cfont {
  FT_Library l;
  FT_Face face[2];
  struct fontInfo i[2];
};
struct cfont fts = {0};

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
  struct cmon *__restrict mons;
  uint32_t monsl;

  uint32_t width, height;
  uint8_t closed;
  uint32_t tlen[9]; // Length in px of every tag icon
};
struct cstate state = {0};
uint8_t rdr = 0;
int32_t barHeight = 0;
int32_t fontPadding = 0;

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

struct cmon *mon_from_surf(struct wl_surface *surf) {
  int32_t i;
  for(i = 0; i < state.monsl; ++i) {
    if (CMON.surf == surf) { return &CMON; }
  }
  LOG(10, "Could not find monitor from surface!\n");
  return NULL;
}

void p_enter(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
  struct cseat *seat = data;
  seat->p.fmon = mon_from_surf(surface);
  seat->p.cpres = 0;
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
  seat->p.x = seat->p.y = seat->p.cpres = 0;
  seat->p.fmon = NULL;
}

void p_motion(void *data, struct wl_pointer *ptr, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
  struct cseat *seat = data;
  seat->p.x = wl_fixed_to_int(x);
  seat->p.y = wl_fixed_to_int(y);
}

void p_button(void *data, struct wl_pointer *ptr, uint32_t serial, uint32_t time, uint32_t button, uint32_t pressed) {
  struct cseat *seat = data;
  uint8_t cp = pressed == WL_POINTER_BUTTON_STATE_PRESSED;
  if (button == BTN_LEFT) {
    LOG(0, "Pressing: %u %u -> ", seat->p.cpres, cp);
    if (seat->p.cpres && cp) {
      seat->p.cpres = 2;
    } else {
      seat->p.cpres = cp;
    }
    LOG(0, "%u\n", seat->p.cpres);
  }
}

void p_frame(void *data, struct wl_pointer *ptr) {
  struct cseat *seat = data;
  if (!seat->p.fmon) {
    return;
  }
  uint32_t px = 0;
  uint32_t cp = 0;
  uint32_t p2 = 1;
  int32_t i;
  if (seat->p.cpres == 1) {
    seat->p.cpres = 2;
    LOG(0, "Handle clik at %u!\n", seat->p.x);
    for(i = 0; i < 9; ++i) {
      if (!((seat->p.fmon->stag & p2) || (seat->p.fmon->ctag & p2))) {
        p2 <<= 1;
        continue;
      }
      cp = state.tlen[i];
      if (px <= seat->p.x && seat->p.x < px + cp) {
        LOG(0, "Got at %i\n", i);
        break;
      }

      px += cp;
      p2 <<= 1;
    }
  }
}

const struct wl_pointer_listener pointer_listener = { .enter = p_enter, .leave = p_leave, .motion = p_motion, .button = p_button, .frame = p_frame, .axis = p_axis, .axis_source = p_axis_source, .axis_stop = p_axis_stop, .axis_discrete = p_axis_discrete };

void seat_caps(void *data, struct wl_seat *s, uint32_t caps) {
  struct cseat *seat = data;
  uint8_t hasPointer = caps & WL_SEAT_CAPABILITY_POINTER;
  if (!seat->p.p && hasPointer) {
    seat->p.p = wl_seat_get_pointer(seat->s);
    wl_pointer_add_listener(seat->p.p, &pointer_listener, seat);
  }
}
const struct wl_seat_listener seat_listener = { .capabilities = seat_caps, .name = seat_name };

void zxout_name(void* data, struct zxdg_output_v1* xout, const char* name) {
  struct cmon *mon = data;
  mon->xdgname = strdup(name);
  LOG(0, "Got monitor name %s[Added to mon %u]\n", name, mon->n);
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
    //LOG(0, "%s\n", fnm);
    fd = shm_open(fnm, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      break;
    }
  } while (--retries);
  if (retries == 0) {
    LOG(0, "Could not create the shm file! [%m]\n");
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

double __inline__ lerp(double t, double o1, double o2) { return (o1 * (1.0 - t)) + (o2 * t); }

uint64_t gcor(uint64_t c, double gc) {
#define CTG(a,x,b,c,d) {x=(double)((a&0xFF000000)>>24)/255.0;b=(double)((a&0xFF0000)>>16)/255.0;c=(double)((a&0x00FF00)>>8)/255.0;d=(double)((a&0x0000FF)>>0)/255.0;}
  double a, r, g, b;
  CTG(c, a, r, g, b);
  r = pow(r, gc) * 255.0;
  g = pow(g, gc) * 255.0;
  b = pow(b, gc) * 255.0;
  return ((uint64_t)a << 24) |
         ((uint64_t)r << 16) |
         ((uint64_t)g <<  8) |
         ((uint64_t)b <<  0);
#undef CTG
}

void pcol(uint64_t c) {
  fprintf(stdout, "%2X %2X %2X %2X\n", (uint8_t)(c&0xFF000000>>24),
                                       (uint8_t)(c&0xFF0000>>16),
                                       (uint8_t)(c&0x00FF00>>8),
                                       (uint8_t)(c&0x0000FF>>0));
}

double KMS_GAMMA = 1.0;

uint64_t ablend(uint64_t co, uint64_t fc, uint64_t bc, double gamma) { /// Why has god forsaken me
#define CTG(a,b,c,d) {b=(double)((a&0xFF0000)>>16)/255.0;c=(double)((a&0x00FF00)>>8)/255.0;d=(double)((a&0x0000FF)>>0)/255.0;}

  uint64_t ac = gcor(co, 1 / gamma);
  //uint64_t ac = co;
  uint64_t af = fc;
  uint64_t ab = bc;

  double r1, r2, r3, r4;
  double g1, g2, g3, g4;
  double b1, b2, b3, b4;
  CTG(ac, r1, g1, b1)
  CTG(af, r2, g2, b2)
  CTG(ab, r3, g3, b3)

	r4 = r1 * r2 + (1.0 - r1) * r3;
	g4 = g1 * g2 + (1.0 - g1) * g3;
	b4 = b1 * b2 + (1.0 - b1) * b3;

  uint64_t bt;
  if (co == 0xFFFFFFFF) {
    bt = (fc >> 24) << 24;
  } else {
    bt = (bc >> 24) << 24;
  }
  uint64_t res =  bt |
                  ((uint64_t)(r4 * 255.0) << 16) |
                  ((uint64_t)(g4 * 255.0) <<  8) |
                  ((uint64_t)(b4 * 255.0) <<  0);
  //res = gcor(res, 1 / gamma);
  return res;
#undef CTG
}

void draw_char(struct cmon *mon, int32_t x, int32_t y, uint8_t cs, uint64_t fc, uint64_t bc) {
#define CG fts.face[cs]->glyph
#define CB fts.face[cs]->glyph->bitmap
#define CM fts.face[cs]->glyph->metrics
#define G(a,y,x,w) ((a)[((y)*(w))+(x)])
  int32_t i, j;
  uint64_t col;
  uint32_t cw = CB.width;
  uint32_t co = mon->sb.size * mon->sb.csel / 4;
  if (CB.pixel_mode == FT_PIXEL_MODE_LCD) {
    cw /= 3;
  }
  //fprintf(stdout, "%u %u %u %u\n", CB.width, CB.pitch, CB.pixel_mode, cw);
  for (i = 0; i < CB.rows; ++i) {
    if ((i + y) < 0 || mon->sb.height <= (i + y)) {
      continue;
    }
    for (j = 0; j < cw; ++j) {
      if ((j + x) < 0 || mon->sb.width <= (j + x)) {
        continue;
      }
      if (CB.pixel_mode == FT_PIXEL_MODE_LCD) {
        col = 0xFF000000 | ((uint64_t)CB.buffer[i * CB.pitch + j * 3] << 16) | ((uint64_t)CB.buffer[i * CB.pitch + j * 3 + 1] << 8) | ((uint64_t)CB.buffer[i * CB.pitch + j * 3 + 2]);
      } else {
        col = 0xFF000000 | ((uint64_t)CB.buffer[i * CB.pitch + j] << 16) | ((uint64_t)CB.buffer[i * CB.pitch + j] << 8) | ((uint64_t)CB.buffer[i * CB.pitch + j]);
      }
      if (!MANUAL_GAMMA) {
        G(mon->sb.data + co, i + y, j + x, mon->sb.width) = ablend(col, fc, bc, gammaCorrections[cs]);
      } else {
        G(mon->sb.data + co, i + y, j + x, mon->sb.width) = ablend(col, fc, bc, KMS_GAMMA);
      }
    }
  }
}

void draw_rect(struct cmon *mon, uint32_t x, uint32_t y, int32_t w, int32_t h, uint64_t col) {
  int32_t i, j;
  uint32_t co = mon->sb.size * mon->sb.csel / 4;
  for (i = 0; i < h; ++i) {
    if ((i + y) < 0 || mon->sb.height <= (i + y)) { /// TODO: Make fast
      continue;
    }
    for (j = 0; j < w; ++j) {
      if ((j + x) < 0 || mon->sb.width <= (j + x)) { /// TODO: Make fast
        continue;
      }
      mon->sb.data[co + (i + y) * mon->sb.width + j + x] = col;
    }
  }
}

int32_t draw_string(struct cmon *mon, const wchar_t *__restrict s, uint32_t x, uint32_t y, uint64_t fc, uint64_t bc, uint8_t render) {
  int32_t i;
  int32_t px = 0;
  uint32_t cg;
  uint8_t cs = 0;
  uint32_t strl = wcslen(s);
  uint8_t pvt = 0;
  uint32_t pv = 0;
  FT_Vector kerning;

  for(i = 0; i < strl; ++i) {
    cg = FT_Get_Char_Index(fts.face[0], s[i]); cs = 0;
    if (cg == 0) { cg = FT_Get_Char_Index(fts.face[1], s[i]); cs = 1; }
    FTCHECK(FT_Load_Glyph(fts.face[cs], cg, fts.i[cs].load_flags), "Could not load a glyph!");

    FTCHECK(FT_Render_Glyph(CG, FT_RENDER_MODE_LCD), "Could not render a glyph!");

    /*if (fts.i[cs].lcd_filter && LCD) {
      FTCHECK(FT_Render_Glyph(CG, FT_RENDER_MODE_LCD), "Could not render a glyph!");
    } else {
      FTCHECK(FT_Render_Glyph(CG, FT_RENDER_MODE_LIGHT), "Could not render a glyph!");
    }*/

    if (render) {
      int32_t xoff =   CM.horiBearingX >> 6;
      int32_t yoff = -(CM.horiBearingY >> 6);
      //fprintf(stdout, "Drawing %lc with %u\n", s[i], fts.i[cs].lcd_filter);
      draw_char(mon, x + px + xoff, y + yoff, cs, fc, bc);
      //draw_rect(mon, x + px, y, CM.horiAdvance >> 6, 1, 0xFFFF0000);
    }

    if (i > 0 && (pvt == cs) && FT_HAS_KERNING(fts.face[cs])) {
      FTCHECK(FT_Get_Kerning(fts.face[cs], pv, cg, FT_KERNING_DEFAULT, &kerning),"Could not get kerning for the given fonts");
      /*
      if (render) {
        fprintf(stdout, "AAAAAA\n");
        draw_rect(mon, x + px, y + 1, kerning.x >> 6, 1, 0xFFFF0000);
      }*/
      px += kerning.x >> 6;
    } else {
      px += CM.horiAdvance >> 6;
    }
    pvt = cs;
    pv = cg;
  }
#undef CG
#undef CM
  return px;
}

void render(struct cmon *mon) {
  if (!mon->sb.b[0]) { return; }

  draw_rect(mon, 0, 0, mon->sb.width, mon->sb.height, colors[SchemeNorm][BACKG]);
  {
    int32_t i;
    uint32_t px = 0;
    uint32_t cl = 0;
    uint32_t p2 = 1;
    int32_t py = ((fts.face[0]->height + fts.face[0]->descender) >> 6) + fontPadding;
    for(i = 0; i < 9; ++i) {
      if (!((mon->stag & p2) || (mon->ctag & p2))) {
        p2 <<= 1;
        continue;
      }
      if (mon->ctag & p2) {
        cl = state.tlen[i];
        draw_rect(mon, px, 0, cl, mon->sb.height, colors[SchemeSel][BACKG]);
        px += draw_string(mon, L" "   , px, py, colors[SchemeSel][FOREG], colors[SchemeSel][BACKG], 1);
        px += draw_string(mon, tags[i], px, py, colors[SchemeSel][FOREG], colors[SchemeSel][BACKG], 1);
        px += draw_string(mon, L" "   , px, py, colors[SchemeSel][FOREG], colors[SchemeSel][BACKG], 1);
      } else {
        px += draw_string(mon, L" "   , px, py, colors[SchemeNorm][FOREG], colors[SchemeNorm][BACKG], 1);
        px += draw_string(mon, tags[i], px, py, colors[SchemeNorm][FOREG], colors[SchemeNorm][BACKG], 1);
        px += draw_string(mon, L" "   , px, py, colors[SchemeNorm][FOREG], colors[SchemeNorm][BACKG], 1);
      }
      p2 <<= 1;
    }
    px += draw_string(mon, L" "   , px, py, colors[SchemeSel][FOREG], colors[SchemeSel][BACKG], 1);
    px += draw_string(mon, mon->slayout, px, py, colors[SchemeNorm][FOREG], colors[SchemeNorm][BACKG], 1);

    cl = draw_string(mon, mon->status, 0, 0, 0, 0, 0) + draw_string(mon, L" ", 0, 0, 0, 0, 0);
    draw_string(mon, mon->status, mon->sb.width - cl, py, colors[SchemeNorm][FOREG], colors[SchemeNorm][BACKG], 1);
  }

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
#define CHECK_INIT(x, e, v) {if (!state. x) { LOG(10, "Your wayland compositor does not support " #e " version " #v "which is required for me to work :(\n"); exit(1); }}
  CHECK_INIT(comp      , wl_compositor         , COMPV   );
  CHECK_INIT(shm       , wl_shm                , SHMV    );
  CHECK_INIT(zwlr      , zwlr_layer_shell_v1   , zwlr_layer_shell_v1.version  );
  CHECK_INIT(xoutmgr   , zxdg_output_manager_v1, XOUTMGRV);
#undef CHECK_INIT

  seatvt(&state.seats);

  {
    int32_t i, j;
    for(i = 0; i < state.monsl; ++i) {
      uint32_t sl = draw_string(&CMON, L" ", 0, 0, 0, 0, 0);
      for(j = 0; j < 9; ++j) {
        state.tlen[j] = sl * 2 + draw_string(&CMON, tags[j], 0, 0, 0, 0, 0);
        LOG(0, "Len ticon %i = %u\n", j, state.tlen[j]);
      }

      wcscpy(CMON.status, L"");
      wcscpy(CMON.slayout, L"[M]");
      CMON.stag = 1|2|8|32;
      CMON.ctag = 4;
      CMON.xout = zxdg_output_manager_v1_get_xdg_output(state.xoutmgr, CMON.out);
      zxdg_output_v1_add_listener(CMON.xout, &zxout_listener, &CMON);
    }
  }
  wl_display_roundtrip(state.dpy);
}

void reg_global(void *data, struct wl_registry *reg, uint32_t name, const char *iface, uint32_t ver) {
#define CHI(x,y,z,w) {if(!strcmp(iface,y .name)) {state. x=wl_registry_bind(reg, name, &y, z);w;return;}}
#define CHV(x,y,z,w) {if(!strcmp(iface,y .name)) {x cbind=wl_registry_bind(reg, name, &y, z);w;return;}}

  CHI(comp           , wl_compositor_interface         , wl_compositor_interface.version,);
  CHI(shm            , wl_shm_interface                , wl_shm_interface.version,);
#if OVERRIDE_ZWLR_VERSION
  CHI(zwlr           , zwlr_layer_shell_v1_interface   , OVERRIDE_ZWLR_VERSION,);
#else
  CHI(zwlr           , zwlr_layer_shell_v1_interface   , zwlr_layer_shell_v1_interface.version,);
#endif
  CHI(xwmBase        , xdg_wm_base_interface           , xdg_wm_base_interface.version,
      xdg_wm_base_add_listener(state.xwmBase, &xwmb_listener, NULL));
  CHI(xoutmgr        , zxdg_output_manager_v1_interface, zxdg_output_manager_v1_interface.version,);
  CHV(struct wl_seat*, wl_seat_interface               , wl_seat_interface.version,
      struct cseat cs = {0};
      cs.n = name;
      cs.s = cbind;
      seatvp(&state.seats, cs);
      wl_seat_add_listener(cbind, &seat_listener, state.seats.v + state.seats.l - 1););
  CHV(struct wl_output*, wl_output_interface           , wl_output_interface.version,
      state.mons = realloc(state.mons, (state.monsl + 1) * sizeof(state.mons[0]));
      struct cmon mon = {0};
      mon.n = name;
      LOG(0, "Added monitor: %u\n", name);
      mon.out = cbind;
      state.mons[state.monsl] = mon;
      ++state.monsl;
      );
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

uint8_t get_font_info(FcPattern *pattern, struct fontInfo *__restrict i) {
#define QD(v,p,d) {switch (FcPatternGetDouble (pattern, p, 0, &v)) { case FcResultNoMatch: v = d; break; case FcResultMatch: break; default: goto fi_crash; }}
#define QB(v,p,d) {switch (FcPatternGetBool   (pattern, p, 0, &v)) { case FcResultNoMatch: v = d; break; case FcResultMatch: break; default: goto fi_crash; }}
#define QI(v,p,d) {switch (FcPatternGetInteger(pattern, p, 0, &v)) { case FcResultNoMatch: v = d; break; case FcResultMatch: break; default: goto fi_crash; }}
  char *cf;
  WLCHECK(FcPatternGetString(pattern, FC_FILE, 0, (FcChar8 **)&cf)==FcResultMatch,"Could not find a suitable font!");
  i->fname = malloc(strlen(cf));
  strcpy(i->fname, cf);
  
  double psize;
  double aspect;
  FcBool vl, hinting, ahint, gadv;
  int hstyle;

  WLCHECK(FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &psize)==FcResultMatch,"Could not get font pixel size!");
  QB(i->aa        , FC_ANTIALIAS      , FcTrue         );
  QI(i->rgba      , FC_RGBA           , FC_RGBA_UNKNOWN);
  QI(i->lcd_filter, FC_LCD_FILTER     , FC_LCD_LIGHT   );
  QB(i->embold    , FC_EMBOLDEN       , FcFalse        );
  QI(i->spacing   , FC_SPACING        , FC_PROPORTIONAL);
  QB(i->minspace  , FC_MINSPACE       , FcFalse        );
  QI(i->cwidth    , FC_CHAR_WIDTH     , 0              );
  QD(aspect       , FC_ASPECT         , 1.0            );
  QB(hinting      , FC_HINTING        , FcTrue         );
  QI(hstyle       , FC_HINT_STYLE     , FC_HINT_SLIGHT );
  QB(vl           , FC_VERTICAL_LAYOUT, FcFalse        );
  QB(ahint        , FC_AUTOHINT       , FcFalse        );
  QB(gadv         , FC_GLOBAL_ADVANCE , FcTrue         );
#undef QD
#undef QB
#undef QI

  i->ysize = psize * 64.0;
  i->xsize = psize * aspect * 64.0;

  i->load_flags = FT_LOAD_DEFAULT | FT_LOAD_COLOR;

  if (i->aa) { /// Code i defo didn't steal from libXft and i fully understand
    if (FC_HINT_NONE < hstyle && hstyle < FC_HINT_FULL) {
      i->load_flags |= FT_LOAD_TARGET_LIGHT;
    } else {
      switch (i->rgba) {
      case FC_RGBA_RGB:
      case FC_RGBA_BGR:
        i->load_flags |= FT_LOAD_TARGET_LCD;
        break;
      case FC_RGBA_VRGB:
      case FC_RGBA_VBGR:
        i->load_flags |= FT_LOAD_TARGET_LCD_V;
        break;
      }
    }
  } else { i->load_flags |= FT_LOAD_TARGET_MONO; }

  if (!hinting || hstyle == FC_HINT_NONE) { i->load_flags |= FT_LOAD_NO_HINTING; }
  if                                 (vl) { i->load_flags |= FT_LOAD_VERTICAL_LAYOUT; }
  if                              (ahint) { i->load_flags |= FT_LOAD_FORCE_AUTOHINT; }
  if                              (!gadv) { i->load_flags |= FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH; }
  if                       (i->cwidth) { i->spacing = FC_MONO; }

  return 0;
fi_crash:
  return 1;
}

void print_font_info(struct fontInfo *__restrict i) { LOG(0, "Got font: %s\n\tPsize: %lix%li\n\tAntialias: %u\n\tEmbold: %u\n\tColor: %u\n\tRgba: %u\n\tLcdFilter: %u\n\tHinting: %u\n\tLoadFlags: 0x%x\n\tSpacing: %x\n\tMinspace: %x\n\tChar Width: %x\n", i->fname, i->xsize, i->ysize, i->aa, i->embold, i->color, i->rgba, i->lcd_filter, i->hinting, i->load_flags, i->spacing, i->minspace, i->cwidth); }

void find_font_face(const char *fname, FT_Face *face, struct fontInfo *i) {
  FcInit(); /// Boy do i love fontconfig and all their incredible documentation
  FcResult result;
  FcConfig *config = FcInitLoadConfigAndFonts();
  FcPattern *pat = FcNameParse((const FcChar8*)fname);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);
  FcPattern *font = FcFontMatch(config, pat, &result);
  WLCHECK(!get_font_info(font, i),"Could not get font information about the selected font!");
  FcPatternDestroy(font);
  FcPatternDestroy(pat);
  FcConfigDestroy(config);
  FcFini();

  //print_font_info(i);
  FTCHECK(FT_New_Face(fts.l, i->fname, 0, face),"Could not create a font face from the given pattern!");
  FTCHECK(FT_Set_Char_Size(*face, 0, 16 * 64, 300, 300),"Could not set the character size!");
  FTCHECK(FT_Set_Pixel_Sizes(*face, i->xsize >> 6, i->ysize >> 6),"Could not set the pixel size!");
}

void init_freetype() {
  FTCHECK(FT_Init_FreeType(&fts.l),"Could not initialise the Freetype lib!");
  find_font_face(fonts[0], fts.face, fts.i);
  if (fonts[1]) {
    find_font_face(fonts[1], fts.face + 1, fts.i + 1);
  }
}

void upt(struct cmon *curMon, char *__restrict s, uint32_t sl) {
  uint32_t cl = 0; 
  curMon->stag = 0;
  curMon->ctag = 0;
  while (s[cl] != ' ' && cl < sl) {
    curMon->stag *= 10;
    curMon->stag += s[cl] - '0';
    ++cl;
  }
  ++cl;
  while (s[cl] != ' ' && cl < sl) {
    curMon->ctag *= 10;
    curMon->ctag += s[cl] - '0';
    ++cl;
  }
}

uint32_t runel(char *__restrict str) {
  char *__restrict os = str;
  for (++str; (*str & 0xc0) == 0x80; ++str);
  return (uint32_t) (str - os);
}

uint32_t utf8_to_unicode(char *__restrict str, uint32_t l) {
  uint32_t res = 0;
  switch (l) {
    case 4:
      res |= *str & 0x7;
      break;
    case 3:
      res |= *str & 0xF;
      break;
    case 2:
      res |= *str & 0x1F;
      break;
    case 1:
      res |= *str & 0x7F;
      break;
  }

  --l;
  while (l) {
    ++str;
    res <<= 6;
    res |= *str & 0x3F;
    --l;
  }

  return res;
}

void utf2wwch(char *__restrict s, wchar_t *__restrict t) {
  uint32_t tl = 0;
  uint32_t cl;
  while (*s) {
    cl = runel(s);
    t[tl] = utf8_to_unicode(s, cl);
    if (t[tl] == L'\n' || t[tl] == L'\r') { --tl; }
    ++tl;
    s += cl;
  }
  t[tl] = L'\0';
}

void check_status(int32_t fd) {
  char c[1024];
  int32_t rl = read(fd, c, SLEN(c));
  WLCHECK(rl>=0,"Could not read from the status file!\n");
  c[rl] = '\0';
  LOG(0, "Read status %u[%s]!\n", rl, c);
  int32_t i;
  for(i = 0; i < state.monsl; ++i) {
    utf2wwch(c, CMON.status);
  }
}

struct cmon *mon_from_name(char *name) {
  int32_t i;
  for(i = 0; i < state.monsl; ++i) {
    LOG(0, "MONF: %s\n", CMON.xdgname);
    if (!strcmp(CMON.xdgname, name)) { return &CMON; }
  }
  LOG(10, "Could not find monitor from name!\n");
  return NULL;
}

void check_dwl(int32_t rfd) {
  static char     b[3][256] = { 0 };
  static uint8_t bl[3]      = { 0 };
  static uint8_t cc = 0;

  char cbuf[1024];
  int32_t rl = 0;
  rl = read(rfd, cbuf, SLEN(cbuf));
  cbuf[rl] = '\0';
  LOG(0, "TMPTMPTMPTMP\n");
  LOG(0, "Cbf: %s\n", cbuf);

  if (rl) {
    int32_t i;
    for(i = 0; i < rl; ++i) {
      if (cbuf[i] == ' ' && cc < 2) { b[cc][bl[cc]] = '\0'; ++cc; } 
      else if (cbuf[i] == '\n') {
        b[cc][bl[cc]] = '\0';
        LOG(0, "Got read: %s - %s - %s\n", b[0], b[1], b[2]);
        struct cmon *curMon = mon_from_name(b[0]);
        LOG(0, "Got monitor: %p\n", curMon);
        cc = 0;

        if (strcmp(b[1], "tags") == 0) {
          upt(curMon, b[2], bl[2]);
          rdr = 1;
        } else if (strcmp(b[1], "layout") == 0) {
          utf2wwch(b[2], curMon->slayout);
          rdr = 1;
        } else if (strcmp(b[1], "debug") == 0) {
          upt(curMon, b[2], bl[2]);
          rdr = 1;
        }

        bl[0] = bl[1] = bl[2] = 0;
      } else {
        b[cc][bl[cc]] = cbuf[i];
        ++bl[cc];
      }
    }
  }

  if (rl == SLEN(cbuf)) { check_dwl(rfd); }
}

int32_t mkpip(int32_t *wfd) {
  errno = 0;
  if (mkfifo(statusPath, 0666) == 0 || errno == EEXIST) {
    int32_t fd = open(statusPath, O_CLOEXEC | O_NONBLOCK | O_RDONLY);
    if (fd < 0) {
      LOG(10, "Could not open the pipe at %s! %m\n", statusPath);
      exit(1);
    }
    *wfd = open(statusPath, O_CLOEXEC | O_WRONLY);
    if (*wfd < 0) {
      LOG(10, "Could not open the pipe at %s for writing! %m\n", statusPath);
      exit(1);
    }
    return fd;
  } else {
    LOG(10, "Could not create a pipe at %s! %m\n", statusPath);
    exit(1);
  }
  return 0;
}

void interrupt_handler(sig_atomic_t sig) {
  unlink(statusPath);
  fclose(errf);
  exit(sig);
}

void render_mons() { int32_t i; for(i = 0; i < state.monsl; ++i) { render(state.mons + i); } }

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0) {
      fprintf(stderr, "Usage:\n");
      fprintf(stderr, "  deburr [status <status_message>]\n");
      exit(0);
    }
    if (strcmp(argv[1], "status") == 0) {
      char a[1024] = { 0 };
      uint32_t al = 0;
      {
        int32_t i;
        uint32_t cl;
        for(i = 2; i < argc; ++i) {
          cl = strlen(argv[i]);
          strncpy(a + al, argv[i], cl);
          a[al + cl] = ' ';
          al += cl + 1;
        }
        --al;
        a[al] = '\0';
      }

      struct stat s;
      if (stat(statusPath, &s)) {
        fprintf(stderr, "Stat %m!\n");
      }
      int32_t fd = open(statusPath, O_WRONLY);
      if (fd < 0) {
        fprintf(stderr, "Could not open %s! [%m]\n", statusPath);
        return 1;
      }
      if (write(fd, a, al) < 0) {
        fprintf(stderr, "Could not write to %s! [%m]\n", statusPath);
        return 1;
      }
      close(fd);

      return 0;
    } else {
      fprintf(stderr, "Unknown argument %s\n", argv[1]);
    }
    return 0;
  }

  if (debugf[0] == '\0') {
    errf = stderr;
  } else {
    errf = fopen(debugf, "w");
  }
    
  setbuf(errf, NULL);
  setlocale(LC_ALL, "");
  init_rand();

  signal(SIGINT, interrupt_handler);
  signal(SIGTERM, interrupt_handler);
  signal(SIGHUP, interrupt_handler);

  init_freetype();
  barHeight = ((fts.face[0]->bbox.yMax + fts.face[0]->underline_thickness) >> 6);
  fontPadding = barHeight * padding;
  barHeight += fontPadding * 2;
  memset(&state, 0, sizeof(state));
  WLCHECK(state.dpy=wl_display_connect(NULL),"Could not connect to the wayland display!");
  WLCHECK(state.reg=wl_display_get_registry(state.dpy),"Could not fetch the wayland registry!");
  seatvi(&state.seats);
  wl_registry_add_listener(state.reg, &reg_listener, NULL);
  wl_display_roundtrip(state.dpy);
  finish_init();

  {
    int32_t i;
    for(i = 0; i < state.monsl; ++i) {
      CMON.surf = wl_compositor_create_surface(state.comp); WLCHECK(CMON.surf,"Cannot create wayland surface!");
      CMON.lsurf = zwlr_layer_shell_v1_get_layer_surface(state.zwlr, CMON.surf, CMON.out, ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "Deburr"); WLCHECK(CMON.lsurf,"Cannot create zwlr surface!");
      zwlr_layer_surface_v1_add_listener(CMON.lsurf, &zwlr_listener, &CMON);
      zwlr_layer_surface_v1_set_anchor(CMON.lsurf, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

      zwlr_layer_surface_v1_set_size(CMON.lsurf, 0, barHeight);
      zwlr_layer_surface_v1_set_keyboard_interactivity(CMON.lsurf, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
      zwlr_layer_surface_v1_set_exclusive_zone(CMON.lsurf, barHeight);
      wl_surface_commit(CMON.surf);
    }
  }
  wl_display_dispatch(state.dpy);

  int32_t dpyfd = wl_display_get_fd(state.dpy); WLCHECK(dpyfd,"Could not get the display fd!");
  int32_t wfd;
  int32_t pipfd = mkpip(&wfd); WLCHECK(pipfd,"Could not create the pipe!");
  int32_t rfd = STDIN_FILENO;

  struct pollfd pfds[] = { {.fd = dpyfd, .events = POLLIN },
                           {.fd = rfd  , .events = POLLIN },
                           {.fd = pipfd, .events = POLLIN } };

  /*
  struct pollfd pfds[] = { 
                           {.fd = rfd  , .events = POLLIN },
                           {.fd = pipfd, .events = POLLIN } };
                           */

  rdr = 1;
  LOG(0, "Started!\n");
  render_mons();
  wl_display_dispatch(state.dpy);
  int32_t pr;
  uint32_t cframe = 0;
  while (!state.closed) {
    if (!MANUAL_GAMMA) {
      LOG(0, "WAIT FLUSH %u\n", cframe);

      wl_display_flush(state.dpy);
      LOG(0, "WAIT ROUNDTRIP %u\n", cframe);
      wl_display_roundtrip(state.dpy);
      LOG(0, "WAIT POLL %u\n", cframe);
      pr = poll(pfds, SLEN(pfds), -1);

      LOG(0, "GO POLL %u\n", cframe); ++cframe;
      LOG(0, "GOT (pr)%u - %u %u %u\n", pr, pfds[0].revents, pfds[1].revents, pfds[2].revents);
      if (pr < 0 && errno != EINTR) { 
        LOG(0, "Could not poll for the filedescriptors! %m\n") ;
      } else if (pr > 0) {
        if (pfds[0].revents & POLLIN) {
          LOG(0, "Got wayland events!\n");
          rdr = 1;
          pfds[0].revents = 0; // Don't think this is necessary but some bloke on stackoverflow said it's good so here it is
        }
        if (pfds[1].revents & POLLIN) {
          LOG(0, "Got dwl events!\n");
          check_dwl(rfd);
          LOG(0, "Finish dwl events!\n");
          pfds[1].revents = 0;
          rdr = 1;
        }
        if (pfds[2].revents & POLLIN) {
          LOG(0, "Got status events!\n");
          check_status(pipfd);
          LOG(0, "Finish status events!\n");
          pfds[2].revents = 0;
          rdr = 1;
        }
        if (rdr) {
          LOG(0, "prerender!\n");
          render_mons();
          LOG(0, "postredn!\n");
          wl_display_dispatch(state.dpy);
          LOG(0, "posdispatch!\n");
          rdr = 0;
        }
      }
    } else {
      rdr = 1;
      if (fscanf(stdin, "%lf", &KMS_GAMMA)) {
        fprintf(stdout, "ERROR READING\n");
      }
      if (rdr) {
        render_mons();
        wl_display_dispatch(state.dpy);
        rdr = 0;
      }
    }
    
  }

  close(rfd);
  close(wfd);
  close(pipfd);
  close(dpyfd);
  unlink(statusPath);
  fclose(errf);

  return 0;
}
