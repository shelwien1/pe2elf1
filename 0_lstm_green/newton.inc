// =========================================================================
// Constants for the improved BinaryMixer / ParamUpdater (ported from coder0.cpp).
// The I_* hyperparameters come from MOD/sh_model_h.inc; SCALE from sh_v2f.inc.
// Expressions are copied verbatim from coder0.cpp so the two builds stay
// numerically identical.
#define ADAPT_L 0

const float F_Pmin      = float(I_Pmin) / (SCALE << 16);
const float F_Wclip     = float(I_Wclip) / (1<<8);
const float F_LW        = float(I_LW)/SCALE; //0.9999f;
const float F_W0        = float(I_W0)/SCALE;
const float F_W_M1_F    = 1.0f - float(I_W_M1) / (SCALE<<9);
const float F_W_M2_F    = 1.0f - float(I_W_M2) / (SCALE<<11);
const float F_W_NW_F    = float(I_W_NW) / (SCALE<<9);
const float F_W_INC_F   = float(I_W_INC) / (SCALE<<11);
const float F_W_STEP_F  = float(I_W_STEP) / (SCALE<<8);
const float F_W_G2_F    = float(I_W_G2) / (1<<5);
const float F_W_D_CLIP_F= float(I_W_D_CLIP) / (1<<4);
const float F_W_R_CLIP_F= float(I_W_R_CLIP) / (1<<4);

#define logf(x) logf_schraudolph(x)
#define sq(x) sq_schraudolph(x)

// =========================================================================
// Improved BinaryMixer (ported from coder0.cpp).
//
// Per-tree-node logistic mixing performed in the log-probability domain, with
// an independent second-order (Newton-style) weight update per node. Replaces
// the previous single-scalar AdamW1 mixer. Mix()/Update() keep the same
// signatures and still populate probs_[], so the surrounding frequency-table
// range coder in main() is unchanged.
struct ParamUpdater {
  float val;
  float D;
  float R;

  void Init(float init_val) {
    val = init_val;
    D = 0.0f;
    R = 0.0f;
  }

  void Update(
    float dp, float d2p, float inv_p, float inv_p2
  ) {
    float dp_inv = dp * inv_p;
    float dp2_inv = (dp * dp) * inv_p2;
    float d2p_inv = d2p * inv_p;

    d2p_inv = clip( d2p_inv, F_W_G2_F );

    D = D * F_W_M1_F - dp_inv;
    R = R * F_W_M2_F + dp2_inv - d2p_inv;

    D = clip(D, F_W_D_CLIP_F);
    R = clamp(R, 0.0f, F_W_R_CLIP_F);

    float safe_R = R + F_W_INC_F;
    float step = F_W_NW_F * D / safe_R;

    step = clip(step, F_W_STEP_F);

    val -= step;

    val = clamp(val, -F_Wclip, F_Wclip);
  }
};

// --- Fast approximations confined to the mixer (matches coder0.cpp) ---------
//#define logf(x) logf_schraudolph(x)
//#define sq(x)   sq_schraudolph(x)

struct BinaryMixer {
  ALIGN(64) ParamUpdater w_state[256];
  ALIGN(64) float probs_[256];
  ALIGN(64) float mix_p0[256];
  ALIGN(64) float st_lstm_[256];
  ALIGN(64) float st_ppmd_[256];
#if ADAPT_L
  float L;
#endif

  void Init() {
    float W0 = clip( st(F_W0), F_Wclip );
    for( uint i=0; i<256; i++ ) {
      w_state[i].Init(W0);
      probs_[i] = 0.0f;
      st_lstm_[i] = 0.0f;
      st_ppmd_[i] = 0.0f;
    }
#if ADAPT_L
    L = 0.0f;
#endif
  }

  NOINLINE
  void Mix(cprfloat lstm_probs, cprfloat ppmd_probs, const char* cmap) {
    ALIGN(64) float sum_lstm[512];
    ALIGN(64) float sum_ppmd[512];
    ALIGN(64) float mix_tree[256];

    mix_tree[1] = 1.0f;
    sum_lstm[0] = 0.5f;
    sum_ppmd[0] = 0.5f;
    mix_p0[0] = 0.0f;

    for( uint i = 0; i<CNUM; i++ ) {
      sum_lstm[256+i] = lstm_probs[i];
      sum_ppmd[256+i] = ppmd_probs[i];
    }

    for( uint j = 255; j>=1; j-- ) {
      sum_lstm[j] = sum_lstm[2*j]+sum_lstm[2*j+1];
      sum_ppmd[j] = sum_ppmd[2*j]+sum_ppmd[2*j+1];
    }

    for( int j = 0; j<512; j++ ) {
      sum_lstm[j] = logf( fmaxf(sum_lstm[j],F_Pmin) );
      sum_ppmd[j] = logf( fmaxf(sum_ppmd[j],F_Pmin) );
    }

    for( int j = 1; j<256; j++ ) {
      float w = sq( w_state[j].val );
      float st_l = sum_lstm[2*j+0] - sum_lstm[2*j+1];
      float st_p = sum_ppmd[2*j+0] - sum_ppmd[2*j+1];

      st_lstm_[j] = st_l;
      st_ppmd_[j] = st_p;

      float st_mix = (1.0f-w)*st_l + w*st_p;
      mix_p0[j] = st_mix;
    }

    for( int j = 0; j<256; j++ ) {
      mix_p0[j] = sq(mix_p0[j]);
    }

    for( int j = 1; j<128; j++ ) {
      float p0_mix = mix_p0[j];
      mix_tree[2*j+0] = mix_tree[j] * p0_mix;
      mix_tree[2*j+1] = mix_tree[j] * (1.0f-p0_mix);
    }

    for( int j = 128; j<256; j++ ) {
      float p0_mix = mix_p0[j];
      uint idx0 = 2*j+0 - 256;
      uint idx1 = 2*j+1 - 256;
      probs_[idx0] = cmap[idx0] ? mix_tree[j] * p0_mix : 0.0f;
      probs_[idx1] = cmap[idx1] ? mix_tree[j] * (1.0f-p0_mix) : 0.0f;
    }

  }

  void Update( uint c ) {
    uint i, ctx=1, bit;
    for( i = 7; i!=-1; i-- ) {
      bit = (c & (1<<i)) ? 1 : 0;
      float p0 = mix_p0[ctx];
      float pK_t = bit ? (1.0f - p0) : p0;

#if ADAPT_L
      L = L*F_LW - logf( fmaxf(pK_t, F_Pmin) );
#endif

      // Derivative Calculus
      float st_l = st_lstm_[ctx];
      float st_p = st_ppmd_[ctx];
      float W = w_state[ctx].val;
      float w_val = sq( W );

      float diff = st_p - st_l;

      float p0_1 = p0 * (1.0f - p0);
      float p0_2 = p0_1 * (1.0f - 2.0f * p0);

      float dp0_dst = p0_1;
      float d2p0_dst2 = p0_2;

      float dst_dw = diff;

      float w_1 = w_val * (1.0f - w_val);
      float w_2 = w_1 * (1.0f - 2.0f * w_val);

      float dp0_dw = dp0_dst * dst_dw;
      float d2p0_dw2 = d2p0_dst2 * (dst_dw * dst_dw);

      // Chain rules applied
      float dp0_dW = dp0_dw * w_1;
      float d2p0_dW2 = d2p0_dw2 * (w_1 * w_1) + dp0_dw * w_2;

      float sign = bit ? -1.0f : 1.0f;
      float dpK_dW = dp0_dW * sign;
      float d2pK_dW2 = d2p0_dW2 * sign;

      float inv_p = 1.0f / fmaxf(pK_t, 1e-12f);
      float inv_p2 = inv_p * inv_p;

      // Push context variable update
      w_state[ctx].Update(dpK_dW, d2pK_dW2, inv_p, inv_p2);

      ctx = ctx*2 + bit;
    }
  }

};

#undef logf
#undef sq
