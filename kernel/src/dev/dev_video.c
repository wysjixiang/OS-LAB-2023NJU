#include <os.h>
#include <devices.h>

#define NTEXTURE 16384
#define NSPRITE  16384

static sem_t fb_sem;
static uint8_t term_font[];

static void texture_fill(struct texture *tx, int top, uint8_t *bits, uint32_t fg, uint32_t bg) {
  uint32_t *px = tx->pixels;
  for (int y = 0; y < TEXTURE_H; y++)
    for (int x = 0; x < TEXTURE_W; x++) {
      int bitmask = top ? bits[y + TEXTURE_H] : bits[y];
      *px++ = ((bitmask >> (7 - x)) & 1) ? fg : bg;
    }
}

static void font_load(fb_t *fb, uint8_t *font) {
  for (int ch = 0; ch < 256; ch++) {
    texture_fill(&fb->textures[ch * 2 + 1], 0, &font[16 * ch], 0xeeeeee, 0x000000);
    texture_fill(&fb->textures[ch * 2 + 2], 1, &font[16 * ch], 0xeeeeee, 0x000000);
  }
}

int fb_init(device_t *dev) {

  DEBUG_PRINTF("fb_init");

  fb_t *fb = dev->ptr;
  fb->info = pmm->alloc(sizeof(struct display_info));
  fb->textures = pmm->alloc(sizeof(struct texture) * NTEXTURE);
  fb->sprites = pmm->alloc(sizeof(struct sprite) * NSPRITE);
  *(fb->info) = (struct display_info) {
    .width  = io_read(AM_GPU_CONFIG).width,
    .height = io_read(AM_GPU_CONFIG).height,
    .num_displays = 8,
    .num_textures = NTEXTURE,
    .num_sprites  = NSPRITE,
    .current = 0,
  };
  kmt->sem_init(&fb_sem, dev->name, 1);
  font_load(fb, term_font);

  DEBUG_PRINTF("fb_init end");

  return 0;
}

static int fb_read(device_t *dev, int offset, void *buf, int count) {
  fb_t *fb = dev->ptr;
  if (offset != 0) return 0;
  if (count != sizeof(struct display_info)) return 0;
  memcpy(buf, fb->info, sizeof(struct display_info));
  return 0;
}

static int fb_write(device_t *dev, int offset, const void *buf, int count) {
  fb_t *fb = dev->ptr;
  kmt->sem_wait(&fb_sem);
  if (offset == 0) {
    const struct display_info *info = buf;
    if (fb->info->current != info->current) {
      fb->info->current = info->current;
    }
  } else if (offset < SPRITE_BRK) {
    memcpy(((uint8_t *)fb->textures) + offset, buf, count);
  } else {
    // TODO: not remove stale sprites, and does not consider z-values
    for (struct sprite *sp = (struct sprite *)buf; sp < (struct sprite *)(buf + count); sp++) {
      if (sp->texture > 0 && sp->display == fb->info->current) {
        io_write(AM_GPU_FBDRAW, sp->x, sp->y, fb->textures[sp->texture].pixels, TEXTURE_W, TEXTURE_H, true);
      }
    }
  }
  kmt->sem_signal(&fb_sem);
  return count;
}

devops_t fb_ops = {
  .init  = fb_init,
  .read  = fb_read,
  .write = fb_write,
};

static uint8_t term_font[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x81, 0xa5, 0x81, 0x81, 0xbd, 0x99, 0x81, 0x81, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0xff, 0xdb, 0xff, 0xff, 0xc3, 0xe7, 0xff, 0xff, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x36, 0x7f, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x3e, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x7e, 0x3c, 0xc3, 0xe7, 0xe7, 0xdb, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x08, 0x1c, 0x3e, 0x7f, 0x7f, 0x7f, 0x3e, 0x08, 0x08, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x3e, 0x3e, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xc1, 0xc1, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x76, 0x62, 0x62, 0x76, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0x89, 0x9d, 0x9d, 0x89, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x00, 0x00, 0x3f, 0x0f, 0x1d, 0x38, 0x7e, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x1c, 0x7f, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0c, 0x0e, 0x0d, 0x0d, 0x0e, 0x0d, 0x0d, 0x0c, 0x1c, 0x7f, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0x1f, 0x3b, 0x33, 0x37, 0x3b, 0x33, 0x33, 0x37, 0x75, 0x57, 0x70, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x18, 0x18, 0xdb, 0x3c, 0xe7, 0x3c, 0xdb, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x60, 0x70, 0x78, 0x7c, 0x7e, 0x7f, 0x7e, 0x7c, 0x78, 0x70, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x3e, 0x7f, 0x1c, 0x1c, 0x1c, 0x7f, 0x3e, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3f, 0x6d, 0x6d, 0x6d, 0x3d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x60, 0x3e, 0x63, 0x63, 0x3e, 0x03, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x3e, 0x7f, 0x1c, 0x1c, 0x1c, 0x7f, 0x3e, 0x1c, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x3e, 0x7f, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x7f, 0x3e, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x38, 0x1c, 0x0e, 0x7f, 0x7f, 0x0e, 0x1c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0e, 0x1c, 0x38, 0x7f, 0x7f, 0x38, 0x1c, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x36, 0x7f, 0x36, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x1c, 0x3e, 0x3e, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x3e, 0x3e, 0x1c, 0x1c, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x33, 0x77, 0xee, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0xff, 0xff, 0x66, 0x66, 0xff, 0xff, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x18, 0x3c, 0x66, 0x30, 0x18, 0x0c, 0x66, 0x3c, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x70, 0x51, 0x73, 0x06, 0x0c, 0x18, 0x30, 0x67, 0x45, 0x07, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x38, 0x6b, 0x66, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0e, 0x1c, 0x38, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x30, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x30, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x38, 0x0c, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0c, 0x38, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x63, 0x36, 0x1c, 0x7f, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x36, 0x63, 0x63, 0x6b, 0x6b, 0x63, 0x63, 0x36, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0e, 0x1e, 0x36, 0x66, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x03, 0x03, 0x03, 0x3e, 0x03, 0x03, 0x03, 0x03, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x07, 0x0f, 0x1b, 0x33, 0x63, 0x63, 0x7f, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x60, 0x60, 0x60, 0x7e, 0x03, 0x03, 0x03, 0x03, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0x30, 0x60, 0x60, 0x7e, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x63, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x36, 0x1c, 0x36, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x3f, 0x03, 0x03, 0x03, 0x06, 0x7c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x06, 0x0c, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x6f, 0x6b, 0x6b, 0x6e, 0x60, 0x60, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x63, 0x63, 0x63, 0x7e, 0x63, 0x63, 0x63, 0x63, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x33, 0x61, 0x60, 0x60, 0x60, 0x60, 0x61, 0x33, 0x1e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7c, 0x66, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x66, 0x7c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x60, 0x60, 0x60, 0x7e, 0x60, 0x60, 0x60, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x60, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x33, 0x63, 0x60, 0x60, 0x67, 0x63, 0x63, 0x33, 0x1e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x78, 0x6c, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x77, 0x7f, 0x6b, 0x6b, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x73, 0x6b, 0x67, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x63, 0x63, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7b, 0x6f, 0x3e, 0x06, 0x03, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x63, 0x63, 0x63, 0x7e, 0x6c, 0x66, 0x66, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x30, 0x18, 0x0c, 0x06, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x36, 0x1c, 0x08, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x6b, 0x6b, 0x7f, 0x77, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x22, 0x36, 0x1c, 0x1c, 0x36, 0x22, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x03, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x08, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 
  0x00, 0x70, 0x38, 0x1c, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x60, 0x60, 0x60, 0x6e, 0x73, 0x63, 0x63, 0x63, 0x73, 0x6e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x63, 0x60, 0x60, 0x60, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x03, 0x03, 0x3b, 0x67, 0x63, 0x63, 0x63, 0x67, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x33, 0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x67, 0x63, 0x63, 0x63, 0x67, 0x3b, 0x03, 0x03, 0x7e, 0x00, 
  0x00, 0x00, 0x60, 0x60, 0x60, 0x6e, 0x73, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3c, 0x00, 
  0x00, 0x00, 0x60, 0x60, 0x60, 0x66, 0x7c, 0x70, 0x78, 0x7c, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x7f, 0x6b, 0x6b, 0x6b, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x76, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x73, 0x63, 0x63, 0x63, 0x73, 0x6e, 0x60, 0x60, 0x60, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x67, 0x63, 0x63, 0x63, 0x67, 0x3b, 0x03, 0x03, 0x03, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x3b, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x66, 0x30, 0x18, 0x0c, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x63, 0x6b, 0x6b, 0x6b, 0x7f, 0x36, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x36, 0x1c, 0x1c, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x67, 0x3b, 0x03, 0x03, 0x7e, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x33, 0x61, 0x60, 0x60, 0x60, 0x60, 0x61, 0x33, 0x1e, 0x06, 0x06, 0x7c, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x0c, 0x18, 0x00, 0x3e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x08, 0x1c, 0x36, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x36, 0x36, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x0c, 0x06, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x1c, 0x36, 0x1c, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x63, 0x60, 0x60, 0x60, 0x63, 0x3e, 0x06, 0x06, 0x3c, 0x00, 
  0x00, 0x08, 0x1c, 0x36, 0x00, 0x3e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x36, 0x36, 0x00, 0x3e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x0c, 0x06, 0x00, 0x3e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x6c, 0x6c, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x3c, 0x66, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x30, 0x18, 0x0c, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x63, 0x00, 0x1c, 0x36, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x1c, 0x36, 0x1c, 0x36, 0x63, 0x63, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0e, 0x18, 0x7f, 0x60, 0x60, 0x60, 0x7e, 0x60, 0x60, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3b, 0x1b, 0x3f, 0x6c, 0x6c, 0x37, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0x3c, 0x6c, 0x6c, 0x7f, 0x6c, 0x6c, 0x6c, 0x6c, 0x6f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x08, 0x1c, 0x36, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x36, 0x36, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x0c, 0x06, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x3c, 0x66, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x30, 0x18, 0x0c, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x36, 0x36, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x67, 0x3b, 0x03, 0x03, 0x7e, 0x00, 
  0x00, 0x63, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x63, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x18, 0x3c, 0x66, 0x62, 0x60, 0x62, 0x66, 0x3c, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x3e, 0x32, 0x30, 0x30, 0x7e, 0x30, 0x30, 0x33, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x6c, 0x76, 0x66, 0x66, 0x78, 0x64, 0x64, 0x6e, 0x64, 0x64, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0e, 0x19, 0x18, 0x18, 0x18, 0x7f, 0x0c, 0x0c, 0x0c, 0x4c, 0x38, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x0c, 0x18, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x0c, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x0c, 0x18, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0c, 0x18, 0x30, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3b, 0x6e, 0x00, 0x6c, 0x76, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x73, 0x8c, 0x63, 0x63, 0x63, 0x73, 0x6b, 0x67, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x3e, 0x66, 0x66, 0x3f, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x30, 0x73, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x70, 0x70, 0x70, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x60, 0x60, 0x61, 0x63, 0x66, 0x0c, 0x18, 0x30, 0x6e, 0x43, 0x06, 0x0c, 0x1f, 0x00, 0x00, 
  0x00, 0x60, 0x60, 0x61, 0x63, 0x66, 0x0c, 0x18, 0x33, 0x67, 0x4b, 0x13, 0x3f, 0x03, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x66, 0xcc, 0x66, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x66, 0x33, 0x66, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 
  0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 
  0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0x18, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0xf6, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x18, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0xf6, 0x06, 0xf6, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x06, 0xf6, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0xf6, 0x06, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0x18, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x18, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x30, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x30, 0x37, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0xf7, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xf7, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x30, 0x37, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0xf7, 0x00, 0xf7, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x18, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x18, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0xff, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0xff, 0x18, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 
  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0xce, 0xcc, 0xcc, 0xcc, 0xce, 0x7b, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x2c, 0x76, 0x66, 0x66, 0x6e, 0x63, 0x63, 0x63, 0x63, 0x6e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x63, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xb6, 0x36, 0x36, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x63, 0x30, 0x18, 0x0c, 0x0c, 0x18, 0x30, 0x63, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7b, 0x60, 0x60, 0x60, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x8c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x7e, 0xdb, 0xdb, 0xdb, 0xdb, 0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x66, 0xc3, 0xc3, 0xbd, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x66, 0xe7, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x73, 0x38, 0x1c, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0xdb, 0xdb, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x7e, 0xdb, 0xdb, 0xdb, 0x7e, 0x18, 0x18, 0x18, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x30, 0x60, 0x3c, 0x60, 0x30, 0x1e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x70, 0x38, 0x1c, 0x0e, 0x1c, 0x38, 0x70, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0e, 0x1c, 0x38, 0x70, 0x38, 0x1c, 0x0e, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0e, 0x1b, 0x1b, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xd8, 0xd8, 0xd8, 0x70, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x7e, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x6e, 0x00, 0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0xcc, 0x6c, 0x6c, 0x3c, 0x1c, 0x00, 0x00, 0x00, 
  0x00, 0x6c, 0x76, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x26, 0x0c, 0x18, 0x30, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
