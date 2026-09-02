
struct Lstm {
  uint input_history_[MAX_HORIZON];
  float hidden_[MAX_CELLS+1];

  std::vector<ArrInput> ppmd_history_;
  std::vector<ArrHidden> hidden_history_;
  std::vector<std::vector<ArrHidden> > output_layer_;
  std::vector<ArrHidden> output_m_, output_v_;
  std::vector<ArrOutput> output_;

  float state_[MAX_CELLS];
  std::vector<ArrCells> sq_state_, last_state_;

  float learning_rate_, gradient_clip_;
  uint num_cells_, epoch_, horizon_, input_size_, output_size_, last_input_;
  uint hidden_size_, hidden_error_size_;
  qword update_steps_;
  qword o_update_steps_;
  uint UPDATE_LIMIT;

  Mixer forget_gate_;
  Mixer input_node_;
  Mixer output_gate_;

  NOINLINE
  void Init(uint in_sz, uint out_sz, uint n_cells, uint horiz, float lr, float grad_clip, uint up_limit) {
    hidden_size_ = n_cells+1;
    hidden_error_size_ = n_cells;

    memset(input_history_, 0, sizeof(input_history_));
    memset(hidden_, 0, sizeof(hidden_));

    // Initialize states to 0.5f so they effectively start at 0 after the affine transform
    for( uint i = 0; i<MAX_CELLS; ++i ) {
      state_[i] = 0.5f;
    }

    ppmd_history_.resize(horiz);
    hidden_history_.resize(horiz);
    sq_state_.resize(horiz);
    last_state_.resize(horiz);
    output_layer_.resize(horiz);
    output_.resize(horiz);
    output_m_.resize(out_sz);
    output_v_.resize(out_sz);

    for( uint i = 0; i<out_sz; ++i ) {
      memset(output_m_[i].d, 0, sizeof(output_m_[i].d));
      memset(output_v_[i].d, 0, sizeof(output_v_[i].d));
    }

    for( uint i = 0; i<horiz; ++i ) {
      memset(ppmd_history_[i].d, 0, sizeof(ppmd_history_[i].d));
      memset(hidden_history_[i].d, 0, sizeof(hidden_history_[i].d));
      memset(sq_state_[i].d, 0, sizeof(sq_state_[i].d));
      for( uint j = 0; j<MAX_CELLS; ++j ) {
        last_state_[i].d[j] = 0.5f;
      }

      output_layer_[i].resize(out_sz);
      memset(&output_layer_[i][0], 0, out_sz*sizeof(ArrHidden));
      for( uint j = 0; j<out_sz; ++j )
        output_[i].d[j] = 1.0f/out_sz;
    }

    learning_rate_ = lr;
    num_cells_ = n_cells;
    epoch_ = 0;
    horizon_ = horiz;
    input_size_ = in_sz;
    output_size_ = out_sz;
    last_input_ = 0xFFFFFFFF;
    hidden_[hidden_size_-1] = 1.0f;
    gradient_clip_ = grad_clip;
    UPDATE_LIMIT = up_limit;
    update_steps_ = 0;
    o_update_steps_ = 0;

    forget_gate_.Init(in_sz, out_sz, n_cells, horiz, 0.1531f, true);
    input_node_.Init(in_sz, out_sz, n_cells, horiz, 0.1531f, false);
    output_gate_.Init(in_sz, out_sz, n_cells, horiz, 0.1531f, false);
  }

  NOINLINE
  void Update(uint sym) {
    uint last_ep = epoch_==0 ? horizon_-1 : epoch_-1;
    uint old_input = input_history_[last_ep];
    input_history_[last_ep] = sym;

#if LSTM_TRAIN
    if( epoch_==0 ) {
      float state_err[MAX_CELLS] = {0}, stored_err[MAX_CELLS] = {0};

      for( uint ep = horizon_-1; ep<horizon_; --ep ) {
        if( ep==horizon_-1 ) {
          for( uint i = 0; i<num_cells_; ++i ) {
            state_err[i] = 0.0f;
            stored_err[i] = 0.0f;
          }
        }

        for( uint i = 0; i<output_size_; ++i ) {
          float err = output_[ep].d[i]-(i==input_history_[ep] ? 1.0f : 0.0f);
          cprfloat o_layer = output_layer_[ep][i].d;
          for( uint j = 0; j<hidden_error_size_; ++j )
            stored_err[j] += o_layer[j]*err;
        }

        uint in_sym = ep==0 ? old_input : input_history_[ep-1];
        cprfloat s_ep = sq_state_[ep].d, o_ep = output_gate_.state_[ep].d;
        cprfloat i_ep = input_node_.state_[ep].d;
        cprfloat f_ep = forget_gate_.state_[ep].d;
        cprfloat l_ep = last_state_[ep].d;

        float gate_err[MAX_CELLS];

        if( ep==0&&update_steps_<UPDATE_LIMIT )
          ++update_steps_;

        const float U = 4.0f;

        for( uint i = 0; i<num_cells_; ++i ) {
          float s_ep_mapped = 2.0f*(s_ep[i]-0.5f);
          gate_err[i] = s_ep_mapped*stored_err[i]*o_ep[i]*(1.0f-o_ep[i]);
          state_err[i] += stored_err[i]*o_ep[i]*2.0f*U*s_ep[i]*(1.0f-s_ep[i]);
          if( ep>0 )
            stored_err[i] = 0.0f;
        }
        output_gate_.Update(ppmd_history_[ep].d, hidden_history_[ep].d, ep, in_sym, stored_err, gate_err, learning_rate_, update_steps_, UPDATE_LIMIT);

        for( uint i = 0; i<num_cells_; ++i ) {
          gate_err[i] = state_err[i]*(1.0f-f_ep[i])*i_ep[i]*(1.0f-i_ep[i]);
        }
        input_node_.Update(ppmd_history_[ep].d, hidden_history_[ep].d, ep, in_sym, stored_err, gate_err, 2.0f*learning_rate_, update_steps_, UPDATE_LIMIT);

        for( uint i = 0; i<num_cells_; ++i ) {
          gate_err[i] = (l_ep[i]-i_ep[i])*state_err[i]*f_ep[i]*(1.0f-f_ep[i]);
          if( ep>0 )
            state_err[i] *= f_ep[i];
        }
        forget_gate_.Update(ppmd_history_[ep].d, hidden_history_[ep].d, ep, in_sym, stored_err, gate_err, learning_rate_, update_steps_, UPDATE_LIMIT);

        // Multiply the state gradient clip boundary by U to allow larger gradients
        ClipArray(state_err, num_cells_, gradient_clip_*U);
        ClipArray(stored_err, num_cells_, gradient_clip_);
      }
    }

    ++o_update_steps_;
#else
    (void)old_input;   // only the backward pass reads it
#endif

    for( uint i = 0; i<output_size_; ++i ) {
      prfloat o_layer_ep = output_layer_[epoch_][i].d;
      cprfloat o_layer_last = output_layer_[last_ep][i].d;

#if LSTM_TRAIN
      float err = output_[last_ep].d[i]-(i==sym ? 1.0f : 0.0f);
      float g_out[MAX_CELLS+1];
      for( uint j = 0; j<hidden_size_; ++j ) {
        g_out[j] = err*hidden_[j];
        o_layer_ep[j] = o_layer_last[j];
      }
      AdamW(g_out, output_m_[i].d, output_v_[i].d, o_layer_ep, hidden_size_, learning_rate_*1.0f, float(o_update_steps_)/10.0f, float(UPDATE_LIMIT)*2.0f);
#else
      // frozen: keep the rolling copy Mix() reads, drop the gradient step
      for( uint j = 0; j<hidden_size_; ++j )
        o_layer_ep[j] = o_layer_last[j];
#endif
    }
    last_input_ = sym;
  }

  NOINLINE
  void Mix(cprfloat ppmd_in) {
    prfloat p_ep = ppmd_history_[epoch_].d;
    prfloat h_ep = hidden_history_[epoch_].d;
    memcpy(p_ep, ppmd_in, input_size_*sizeof(float));
    memcpy(h_ep, hidden_, hidden_size_*sizeof(float));
    memcpy(last_state_[epoch_].d, state_, num_cells_*sizeof(float));

    forget_gate_.Mix(p_ep, h_ep, last_input_, epoch_);
    input_node_.Mix(p_ep, h_ep, last_input_, epoch_);
    output_gate_.Mix(p_ep, h_ep, last_input_, epoch_);

    prfloat f_ep = forget_gate_.state_[epoch_].d;
    prfloat i_ep = input_node_.state_[epoch_].d;
    prfloat o_ep = output_gate_.state_[epoch_].d;
    prfloat s_ep = sq_state_[epoch_].d;

    for( uint i = 0; i<num_cells_; ++i ) {
      state_[i] = state_[i]*f_ep[i]+i_ep[i]*(1.0f-f_ep[i]);

      s_ep[i] = sq(4.0f*(state_[i]-0.5f));
      float s_ep_mapped = 2.0f*(s_ep[i]-0.5f);
      hidden_[i] = o_ep[i]*s_ep_mapped;
    }

    float sum_exp = 0.0f;
    prfloat out_ep = output_[epoch_].d;
    for( uint i = 0; i<output_size_; ++i ) {
      float dp = DotProduct(hidden_, output_layer_[epoch_][i].d, hidden_size_);
      sum_exp += (out_ep[i] = exp(dp));
    }
    for( uint i = 0; i<output_size_; ++i )
      out_ep[i] /= sum_exp;

    if( ++epoch_==horizon_ )
      epoch_ = 0;
  }
};

template <typename LstmType> struct UnifiedModel {
  ppmd_Model ppmd_model_;
  LstmType* lstm_;
  char* vocab_;
  byte byte_map_[256];
  float ppmd_probs_[256], lstm_probs_[256];

  NOINLINE void Init(int order, int memory, char* vocab, LstmType* lstm, uint f_len ) {
    vocab_ = vocab;
    lstm_ = lstm;
    ppmd_model_.Init(order, memory, 50, f_len);

    for( uint i = 0, offset = 0; i<256; i++ ) {
      byte_map_[i] = offset;
      lstm_probs_[i] = ppmd_probs_[i] = vocab_[i] ? (++offset, 1.0f/256.0f) : 0.0f;
    }
    Renorm_probs(ppmd_probs_);
  }

  float packed_probs_[256];
  NOINLINE void Make_Packed() {
    for( uint i = 0, j = 0; i<256; i++ )
      if( vocab_[i] )
        packed_probs_[j++] = ppmd_probs_[i];
  }

  void Renorm_probs(prfloat probs) {
    float sum = SumOfAbs(probs, 256);
    float inv_sum = 1.0f/(sum+1e-12f);
    for( uint i = 0; i<256; ++i )
      probs[i] *= inv_sum;
  }

  NOINLINE void UpdatePPMD(uint byte_val) {
    ppmd_model_.ppmd_UpdateByte(byte_val&0xFF);
    ppmd_model_.ppmd_PrepareByte();

    for( uint i = 0; i<256; ++i )
      ppmd_probs_[i] = vocab_[i] ? (ppmd_model_.sqp[i]<1.0f ? 1.0f : ppmd_model_.sqp[i]) : 0.0f;
    Renorm_probs(ppmd_probs_);
  }

  NOINLINE void UpdateLSTM(int sym, cprfloat ppmd_in) {
    lstm_->Update(byte_map_[sym]);
#if 0
    lstm_->Mix(ppmd_in);
#else
    Make_Packed();
    lstm_->Mix(packed_probs_);
#endif
    uint last_ep = lstm_->epoch_==0 ? lstm_->horizon_-1 : lstm_->epoch_-1;
    for( uint i = 0, offset = 0; i<256; i++ )
      lstm_probs_[i] = vocab_[i] ? lstm_->output_[last_ep].d[offset++] : 0.0f;
  }
};
