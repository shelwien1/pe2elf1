// rc_sprite.inc -- recon.inc's rc_sprite(): GMC, the reference warped by the warp points.

// the sprite's warping from the warp points' differences w (m4vdec's decode_sprite_trajectory); 0
// where it overflows
static int rc_sprite(Recon* d, const int* w) {
  int a = 2 << d->warp_acc, rho = SPR_RHO - d->warp_acc, r = SPR_SUB / a;
  int alpha = 1, beta = 0, wd = d->width, ht = d->height, min_ab, i, w2, h2, w3, h3;
  int sprite_ref[SPR_CORNERS][2], virtual_ref[2][2];
  S64 sprite_offset[2][2], sprite_delta[2][2];
  const int vop_ref[SPR_CORNERS][2] = { { 0, 0 }, { d->width, 0 }, { 0, d->height }, { d->width, d->height } };
  int dd[SPR_CORNERS][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
  const int divx413 = d->divx_version == DIVX413_VERSION && d->divx_build == DIVX413_BUILD;
  if (wd <= 0 || ht <= 0) return 0;
  for (i = 0; i < d->warp_points; i++) {
    dd[i][0] = w[2 * i];
    dd[i][1] = w[2 * i + 1];
  }
  while ((1 << alpha) < wd) alpha++;
  while ((1 << beta) < ht) beta++;
  w2 = 1 << alpha;
  h2 = 1 << beta;
  if (divx413) {
    sprite_ref[0][0] = a * vop_ref[0][0] + dd[0][0];
    sprite_ref[0][1] = a * vop_ref[0][1] + dd[0][1];
    sprite_ref[1][0] = a * vop_ref[1][0] + dd[0][0] + dd[1][0];
    sprite_ref[1][1] = a * vop_ref[1][1] + dd[0][1] + dd[1][1];
    sprite_ref[2][0] = a * vop_ref[2][0] + dd[0][0] + dd[2][0];
    sprite_ref[2][1] = a * vop_ref[2][1] + dd[0][1] + dd[2][1];
  } else {
    sprite_ref[0][0] = (a >> 1) * (2 * vop_ref[0][0] + dd[0][0]);
    sprite_ref[0][1] = (a >> 1) * (2 * vop_ref[0][1] + dd[0][1]);
    sprite_ref[1][0] = (a >> 1) * (2 * vop_ref[1][0] + dd[0][0] + dd[1][0]);
    sprite_ref[1][1] = (a >> 1) * (2 * vop_ref[1][1] + dd[0][1] + dd[1][1]);
    sprite_ref[2][0] = (a >> 1) * (2 * vop_ref[2][0] + dd[0][0] + dd[2][0]);
    sprite_ref[2][1] = (a >> 1) * (2 * vop_ref[2][1] + dd[0][1] + dd[2][1]);
  }
  {
    constexpr S64 S = SPR_SUB;
    virtual_ref[0][0] = SPR_SUB * (vop_ref[0][0] + w2) +
        (int)rounded_div(((wd - w2) * (r * sprite_ref[0][0] - S * vop_ref[0][0]) +
                          w2 * (r * sprite_ref[1][0] - S * vop_ref[1][0])), wd);
    virtual_ref[0][1] = SPR_SUB * vop_ref[0][1] +
        (int)rounded_div(((wd - w2) * (r * sprite_ref[0][1] - S * vop_ref[0][1]) +
                          w2 * (r * sprite_ref[1][1] - S * vop_ref[1][1])), wd);
    virtual_ref[1][0] = SPR_SUB * vop_ref[0][0] +
        (int)rounded_div(((ht - h2) * (r * sprite_ref[0][0] - S * vop_ref[0][0]) +
                          h2 * (r * sprite_ref[2][0] - S * vop_ref[2][0])), ht);
    virtual_ref[1][1] = SPR_SUB * (vop_ref[0][1] + h2) +
        (int)rounded_div(((ht - h2) * (r * sprite_ref[0][1] - S * vop_ref[0][1]) +
                          h2 * (r * sprite_ref[2][1] - S * vop_ref[2][1])), ht);
  }
  switch (d->warp_points) {
  case 0:
    sprite_offset[0][0] = sprite_offset[0][1] = sprite_offset[1][0] = sprite_offset[1][1] = 0;
    sprite_delta[0][0] = a; sprite_delta[0][1] = sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    break;
  case 1:
    sprite_offset[0][0] = sprite_ref[0][0] - a * vop_ref[0][0];
    sprite_offset[0][1] = sprite_ref[0][1] - a * vop_ref[0][1];
    sprite_offset[1][0] = ((sprite_ref[0][0] >> 1) | (sprite_ref[0][0] & 1)) - a * (vop_ref[0][0] / 2);
    sprite_offset[1][1] = ((sprite_ref[0][1] >> 1) | (sprite_ref[0][1] & 1)) - a * (vop_ref[0][1] / 2);
    sprite_delta[0][0] = a; sprite_delta[0][1] = sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    break;
  case 2:
    sprite_offset[0][0] = ((S64)sprite_ref[0][0] * (1 << (alpha + rho))) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-vop_ref[0][0]) +
                          ((S64)r * sprite_ref[0][1] - virtual_ref[0][1]) * ((S64)-vop_ref[0][1]) +
                          (1 << (alpha + rho - 1));
    sprite_offset[0][1] = ((S64)sprite_ref[0][1] * (1 << (alpha + rho))) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * ((S64)-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-vop_ref[0][1]) +
                          (1 << (alpha + rho - 1));
    sprite_offset[1][0] = (((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-2 * vop_ref[0][0] + 1) +
                           ((S64)r * sprite_ref[0][1] - virtual_ref[0][1]) * ((S64)-2 * vop_ref[0][1] + 1) +
                           2 * w2 * r * (S64)sprite_ref[0][0] - SPR_SUB * w2 + (1 << (alpha + rho + 1)));
    sprite_offset[1][1] = (((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * ((S64)-2 * vop_ref[0][0] + 1) +
                           ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-2 * vop_ref[0][1] + 1) +
                           2 * w2 * r * (S64)sprite_ref[0][1] - SPR_SUB * w2 + (1 << (alpha + rho + 1)));
    sprite_delta[0][0] = (-r * sprite_ref[0][0] + virtual_ref[0][0]);
    sprite_delta[0][1] = (+r * sprite_ref[0][1] - virtual_ref[0][1]);
    sprite_delta[1][0] = (-r * sprite_ref[0][1] + virtual_ref[0][1]);
    sprite_delta[1][1] = (-r * sprite_ref[0][0] + virtual_ref[0][0]);
    d->sprite_shift[0] = alpha + rho;
    d->sprite_shift[1] = alpha + rho + 2;
    break;
  case 3:
    min_ab = alpha < beta ? alpha : beta;
    w3 = w2 >> min_ab;
    h3 = h2 >> min_ab;
    sprite_offset[0][0] = ((S64)sprite_ref[0][0] * (1 << (alpha + beta + rho - min_ab))) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * h3 * (-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[1][0]) * w3 * (-vop_ref[0][1]) +
                          ((S64)1 << (alpha + beta + rho - min_ab - 1));
    sprite_offset[0][1] = ((S64)sprite_ref[0][1] * (1 << (alpha + beta + rho - min_ab))) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * h3 * (-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[1][1]) * w3 * (-vop_ref[0][1]) +
                          ((S64)1 << (alpha + beta + rho - min_ab - 1));
    sprite_offset[1][0] = ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * h3 * (-2 * vop_ref[0][0] + 1) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[1][0]) * w3 * (-2 * vop_ref[0][1] + 1) +
                          (S64)2 * w2 * h3 * r * sprite_ref[0][0] - SPR_SUB * w2 * h3 +
                          ((S64)1 << (alpha + beta + rho - min_ab + 1));
    sprite_offset[1][1] = ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * h3 * (-2 * vop_ref[0][0] + 1) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[1][1]) * w3 * (-2 * vop_ref[0][1] + 1) +
                          (S64)2 * w2 * h3 * r * sprite_ref[0][1] - SPR_SUB * w2 * h3 +
                          ((S64)1 << (alpha + beta + rho - min_ab + 1));
    sprite_delta[0][0] = (-r * (S64)sprite_ref[0][0] + virtual_ref[0][0]) * h3;
    sprite_delta[0][1] = (-r * (S64)sprite_ref[0][0] + virtual_ref[1][0]) * w3;
    sprite_delta[1][0] = (-r * (S64)sprite_ref[0][1] + virtual_ref[0][1]) * h3;
    sprite_delta[1][1] = (-r * (S64)sprite_ref[0][1] + virtual_ref[1][1]) * w3;
    d->sprite_shift[0] = alpha + beta + rho - min_ab;
    d->sprite_shift[1] = alpha + beta + rho - min_ab + 2;
    break;
  default:
    return 0;
  }
  if (sprite_delta[0][0] == (S64)a << d->sprite_shift[0] && sprite_delta[0][1] == 0 && sprite_delta[1][0] == 0 &&
      sprite_delta[1][1] == (S64)a << d->sprite_shift[0]) {
    sprite_offset[0][0] >>= d->sprite_shift[0];
    sprite_offset[0][1] >>= d->sprite_shift[0];
    sprite_offset[1][0] >>= d->sprite_shift[1];
    sprite_offset[1][1] >>= d->sprite_shift[1];
    sprite_delta[0][0] = a; sprite_delta[0][1] = 0; sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    d->real_warp_points = 1;
  } else {
    int shift_y = SPR_FIX - d->sprite_shift[0], shift_c = SPR_FIX - d->sprite_shift[1];
    constexpr S64 IMAX = INT32_MAX;
    constexpr S64 M = MB_W;  // (the overflow checks: a macroblock past the picture)
    for (i = 0; i < 2; i++) {
      if (shift_c < 0 || shift_y < 0 || llabs(sprite_offset[0][i]) >= IMAX >> shift_y ||
          llabs(sprite_offset[1][i]) >= IMAX >> shift_c || llabs(sprite_delta[0][i]) >= IMAX >> shift_y ||
          llabs(sprite_delta[1][i]) >= IMAX >> shift_y)
        goto overflow;
    }
    for (i = 0; i < 2; i++) {
      sprite_offset[0][i] *= (S64)1 << shift_y;
      sprite_offset[1][i] *= (S64)1 << shift_c;
      sprite_delta[0][i] *= (S64)1 << shift_y;
      sprite_delta[1][i] *= (S64)1 << shift_y;
      d->sprite_shift[i] = SPR_FIX;
    }
    for (i = 0; i < 2; i++) {
      S64 sd0 = sprite_delta[i][0] - a * (1LL << SPR_FIX), sd1 = sprite_delta[i][1] - a * (1LL << SPR_FIX);
      if (llabs(sprite_offset[0][i] + sprite_delta[i][0] * (wd + M)) >= IMAX ||
          llabs(sprite_offset[0][i] + sprite_delta[i][1] * (ht + M)) >= IMAX ||
          llabs(sprite_offset[0][i] + sprite_delta[i][0] * (wd + M) + sprite_delta[i][1] * (ht + M)) >= IMAX ||
          llabs(sprite_delta[i][0] * (wd + M)) >= IMAX || llabs(sprite_delta[i][1] * (ht + M)) >= IMAX ||
          llabs(sd0) >= IMAX || llabs(sd1) >= IMAX || llabs(sprite_offset[0][i] + sd0 * (wd + M)) >= IMAX ||
          llabs(sprite_offset[0][i] + sd1 * (ht + M)) >= IMAX ||
          llabs(sprite_offset[0][i] + sd0 * (wd + M) + sd1 * (ht + M)) >= IMAX)
        goto overflow;
    }
    d->real_warp_points = d->warp_points;
  }
  for (i = 0; i < 2 * 2; i++) {
    d->sprite_offset[i & 1][i >> 1] = (int)sprite_offset[i & 1][i >> 1];
    d->sprite_delta[i & 1][i >> 1] = (int)sprite_delta[i & 1][i >> 1];
  }
  return 1;
overflow:
  memset(d->sprite_offset, 0, sizeof(d->sprite_offset));
  memset(d->sprite_delta, 0, sizeof(d->sprite_delta));
  return 0;
}
