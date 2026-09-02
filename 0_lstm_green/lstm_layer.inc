
static inline void AdamW1(float g, float &m, float &v, float &w, float lr, qword t, uint lim) {
  const float beta1 = 0.025f, beta2 = 0.99991f, eps = 1e-6f;
  const float decay = 2e-4f;
  qword t_eff = t<lim ? t : lim;
  float alpha = lr*0.1f/sqrt(5e-5f*t_eff+1.0f)*(1.0f-(float)t_eff/(lim*1.5f));
  float t_pow = t>0 ? (float)t : 1.0f;
  float b1_t = 1.0f-pow(beta1, t_pow), b2_t = 1.0f-pow(beta2, t_pow);

  m = beta1*m+(1.0f-beta1)*g;
  v = beta2*v+(1.0f-beta2)*g*g;
  w -= alpha*((m/b1_t)/(sqrt(v/b2_t)+eps)+decay*w);
}

static inline void AdamW(cprfloat g, prfloat m, prfloat v, prfloat w, uint n, float lr, qword t, uint lim) {
  const float beta1 = 0.024f, beta2 = 0.9999f, eps = 1e-6f, decay = 1e-5f;
  qword t_eff = t<lim ? t : lim;
  float alpha = lr*0.1f/sqrt(5e-5f*t_eff+1.0f)*(1.0f-(float)t_eff/(lim*1.33f));
  float t_pow = t>0 ? (float)t : 1.0f;
  float b1_t = 1.0f-pow(beta1, t_pow), b2_t = 1.0f-pow(beta2, t_pow);

  for( uint i = 0; i<n; ++i ) {
    m[i] = beta1*m[i]+(1.0f-beta1)*g[i];
    v[i] = beta2*v[i]+(1.0f-beta2)*g[i]*g[i];
    w[i] -= alpha*((m[i]/b1_t)/(sqrt(v[i]/b2_t)+eps)+decay*w[i]);
  }
}

struct Mixer {
  float ivar_sym_[MAX_HORIZON];
  float ivar_ppmd_[MAX_HORIZON];
  float ivar_rec_[MAX_HORIZON];

  float A_[MAX_CELLS], A_u_[MAX_CELLS], A_m_[MAX_CELLS], A_v_[MAX_CELLS];
  float B_[MAX_CELLS], B_u_[MAX_CELLS], B_m_[MAX_CELLS], B_v_[MAX_CELLS];
  float C_[MAX_CELLS], C_u_[MAX_CELLS], C_m_[MAX_CELLS], C_v_[MAX_CELLS];

  std::vector<ArrOutput> sym_w_, sym_u_, sym_m_, sym_v_;
  std::vector<ArrInput> ppmd_w_, ppmd_u_, ppmd_m_, ppmd_v_;
  std::vector<ArrHidden> rec_w_, rec_u_, rec_m_, rec_v_;

  std::vector<ArrCells> state_, transpose_;
  std::vector<ArrCells> norm_sym_, norm_ppmd_, norm_rec_;

  uint num_cells_, input_size_, hidden_size_, out_size_, horizon_;

  void Init(uint in_sz, uint out_sz, uint n_cells, uint horiz, float init_range, bool is_forget = false) {
    num_cells_ = n_cells;
    input_size_ = in_sz;
    out_size_ = out_sz;
    hidden_size_ = n_cells+1;
    horizon_ = horiz;

    memset(ivar_sym_, 0, sizeof(ivar_sym_));
    memset(ivar_ppmd_, 0, sizeof(ivar_ppmd_));
    memset(ivar_rec_, 0, sizeof(ivar_rec_));

    sym_w_.resize(n_cells);
    sym_u_.resize(n_cells);
    sym_m_.resize(n_cells);
    sym_v_.resize(n_cells);

    ppmd_w_.resize(n_cells);
    ppmd_u_.resize(n_cells);
    ppmd_m_.resize(n_cells);
    ppmd_v_.resize(n_cells);

    rec_w_.resize(n_cells);
    rec_u_.resize(n_cells);
    rec_m_.resize(n_cells);
    rec_v_.resize(n_cells);

    const float val = init_range;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    for( uint i = 0; i<n_cells; ++i ) {
      A_[i] = 1.0f;
      A_u_[i] = A_m_[i] = A_v_[i] = 0.0f;
      for( uint j = 0; j<out_sz; ++j )
        sym_w_[i].d[j] = val*cos(M_PI/(i+1)*j);
      memset(sym_u_[i].d, 0, sizeof(sym_u_[i].d));
      memset(sym_m_[i].d, 0, sizeof(sym_m_[i].d));
      memset(sym_v_[i].d, 0, sizeof(sym_v_[i].d));

      B_[i] = 1.0f;
      B_u_[i] = B_m_[i] = B_v_[i] = 0.0f;
      for( uint j = 0; j<in_sz; ++j )
        ppmd_w_[i].d[j] = val*cos(M_PI/(i+1)*j);
      memset(ppmd_u_[i].d, 0, sizeof(ppmd_u_[i].d)); // logic fix
      memset(ppmd_m_[i].d, 0, sizeof(ppmd_m_[i].d));
      memset(ppmd_v_[i].d, 0, sizeof(ppmd_v_[i].d));

      C_[i] = 1.0f;
      C_u_[i] = C_m_[i] = C_v_[i] = 0.0f;
      for( uint j = 0; j<hidden_size_; ++j )
        rec_w_[i].d[j] = val*cos(M_PI/(i+1)*j);

      if( is_forget )
        rec_w_[i].d[n_cells] = 1.0f;

      memset(rec_u_[i].d, 0, sizeof(rec_u_[i].d));
      memset(rec_m_[i].d, 0, sizeof(rec_m_[i].d));
      memset(rec_v_[i].d, 0, sizeof(rec_v_[i].d));
    }

    state_.resize(horiz);
    norm_sym_.resize(horiz);
    norm_ppmd_.resize(horiz);
    norm_rec_.resize(horiz);
    for( uint i = 0; i<horiz; ++i ) {
      memset(state_[i].d, 0, sizeof(state_[i].d));
      memset(norm_sym_[i].d, 0, sizeof(norm_sym_[i].d));
      memset(norm_ppmd_[i].d, 0, sizeof(norm_ppmd_[i].d));
      memset(norm_rec_[i].d, 0, sizeof(norm_rec_[i].d));
    }

    transpose_.resize(hidden_size_);
    for( uint i = 0; i<hidden_size_; ++i )
      memset(transpose_[i].d, 0, sizeof(transpose_[i].d));
  }

  void Mix(cprfloat ppmd_in, cprfloat hid, uint in_sym, uint ep) {
    prfloat n_sym = norm_sym_[ep].d;
    prfloat n_ppmd = norm_ppmd_[ep].d;
    prfloat n_rec = norm_rec_[ep].d;

    for( uint i = 0; i<num_cells_; ++i ) {
      n_sym[i] = sym_w_[i].d[in_sym];
      n_ppmd[i] = DotProduct(ppmd_in, ppmd_w_[i].d, input_size_);
      n_rec[i] = DotProduct(hid, rec_w_[i].d, hidden_size_);
    }

    float sq_sym = SumOfSquares(n_sym, num_cells_)/num_cells_;
    float sq_ppmd = SumOfSquares(n_ppmd, num_cells_)/num_cells_;
    float sq_rec = SumOfSquares(n_rec, num_cells_)/num_cells_;

    ivar_sym_[ep] = 1.0f/sqrt(sq_sym+1e-5f);
    ivar_ppmd_[ep] = 1.0f/sqrt(sq_ppmd+1e-5f);
    ivar_rec_[ep] = 1.0f/sqrt(sq_rec+1e-5f);

    prfloat s_ep = state_[ep].d;
    for( uint i = 0; i<num_cells_; ++i ) {
      n_sym[i] *= ivar_sym_[ep];
      n_ppmd[i] *= ivar_ppmd_[ep];
      n_rec[i] *= ivar_rec_[ep];
      s_ep[i] = sq(A_[i]*n_sym[i]+B_[i]*n_ppmd[i]+C_[i]*n_rec[i]);
    }
  }

  void Update(cprfloat ppmd_in, cprfloat hid, uint ep, uint in_sym, prfloat stored_err, prfloat err, float lr, qword up_steps, uint up_lim) {
    if( ep==horizon_-1 ) {
      for( uint i = 0; i<num_cells_; ++i ) {
        A_u_[i] = 0.0f;
        memset(sym_u_[i].d, 0, out_size_*sizeof(float));

        B_u_[i] = 0.0f;
        memset(ppmd_u_[i].d, 0, input_size_*sizeof(float));

        C_u_[i] = 0.0f;
        memset(rec_u_[i].d, 0, hidden_size_*sizeof(float));
        for( uint j = 0; j<hidden_size_; ++j )
          transpose_[j].d[i] = rec_w_[i].d[j];
      }
    }

    cprfloat n_sym = norm_sym_[ep].d;
    cprfloat n_ppmd = norm_ppmd_[ep].d;
    cprfloat n_rec = norm_rec_[ep].d;

    float sum_sym = 0.0f, sum_ppmd = 0.0f, sum_rec = 0.0f;
    for( uint i = 0; i<num_cells_; ++i ) {
      float g_sym = err[i]*n_sym[i];
      A_u_[i] += g_sym;
      sum_sym += g_sym*A_[i];

      float g_ppmd = err[i]*n_ppmd[i];
      B_u_[i] += g_ppmd;
      sum_ppmd += g_ppmd*B_[i];

      float g_rec = err[i]*n_rec[i];
      C_u_[i] += g_rec;
      sum_rec += g_rec*C_[i];
    }

    sum_sym = (sum_sym*ivar_sym_[ep])/num_cells_;
    sum_ppmd = (sum_ppmd*ivar_ppmd_[ep])/num_cells_;
    sum_rec = (sum_rec*ivar_rec_[ep])/num_cells_;

    float g_rec_arr[MAX_CELLS];
    for( uint i = 0; i<num_cells_; ++i ) {
      float g1 = err[i]*A_[i]*ivar_sym_[ep]-sum_sym*n_sym[i];
      sym_u_[i].d[in_sym] += g1;

      float g2 = err[i]*B_[i]*ivar_ppmd_[ep]-sum_ppmd*n_ppmd[i];
      for( uint j = 0; j<input_size_; ++j )
        ppmd_u_[i].d[j] += g2*ppmd_in[j];

      float g3 = err[i]*C_[i]*ivar_rec_[ep]-sum_rec*n_rec[i];
      g_rec_arr[i] = g3;
      for( uint j = 0; j<hidden_size_; ++j )
        rec_u_[i].d[j] += g3*hid[j];
    }

    if( ep>0 ) {
      for( uint j = 0; j<num_cells_; ++j )
        stored_err[j] += DotProduct(g_rec_arr, transpose_[j].d, num_cells_);
    }

    if( ep==0 ) {
      for( uint i = 0; i<num_cells_; ++i ) {
        for( uint j = 0; j<out_size_; ++j ) {
          if( sym_u_[i].d[j]!=0.0f ) {
            AdamW1(sym_u_[i].d[j], sym_m_[i].d[j], sym_v_[i].d[j], sym_w_[i].d[j], lr, up_steps, up_lim);
          }
        }
        AdamW(ppmd_u_[i].d, ppmd_m_[i].d, ppmd_v_[i].d, ppmd_w_[i].d, input_size_, lr, up_steps, up_lim);
        AdamW(rec_u_[i].d, rec_m_[i].d, rec_v_[i].d, rec_w_[i].d, hidden_size_, lr, up_steps, up_lim);
      }
      AdamW(A_u_, A_m_, A_v_, A_, num_cells_, lr, up_steps, up_lim);
      AdamW(B_u_, B_m_, B_v_, B_, num_cells_, lr, up_steps, up_lim);
      AdamW(C_u_, C_m_, C_v_, C_, num_cells_, lr, up_steps, up_lim);
    }
  }
};
