// inc/gui_layout.inc -- the window's controls made (MakeControls) and placed
//  (Layout, Fit)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the controls made and placed

static const char* const t_items[] = { "100%,100%", "50%+50%", "75%+25%", "1" };
static const char* const l_items[] = { "default", "9", "8", "7", "6", "5", "4", "3", "2", "1" };

static void MakeControls() {
  for( int i=0; i<=NJ; i++ ) split[i] = -1;
  c_comp = Button( L"&Compress", ID_COMP, BS_AUTORADIOBUTTON|WS_GROUP );
  c_dec = Button( L"&Decompress", ID_DEC, BS_AUTORADIOBUTTON );
  c_verify = Button( L"&Verify", ID_VERIFY, BS_AUTOCHECKBOX|WS_GROUP );
  c_cpu = Button( L"CPU I&nfo", ID_CPU );
  l_list = Label( L"List" );
  c_list = Make( L"COMBOBOX", L"", WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|CBS_NOINTEGRALHEIGHT, ID_LIST );
  c_listclr = Button( L"Cl&ear", ID_LISTCLR ); c_listb = Button( L"...", ID_LISTB );
  l_in = Label( L"&Input" ); c_in = Edit( ID_IN ); c_inb = Button( L"...", ID_INB );
  l_out = Label( L"&Output" ); c_out = Edit( ID_OUT ); c_outb = Button( L"...", ID_OUTB );
  l_w = Label( L"&Work dir" ); c_w = Edit( ID_W ); c_wb = Button( L"...", ID_WB );
  c_wtemp = Button( L"%TEMP%", ID_WTEMP ); c_wout = Button( L"Output dir", ID_WOUT ); c_k = Button( L"&keep it", ID_K, BS_AUTOCHECKBOX );

  g_opt = Group( L"Options" );
  l_t = Label( L"&Threads" ); c_t = Combo( ID_T, CBS_DROPDOWN|CBS_AUTOHSCROLL, t_items, 4 );
  l_free = Label( L"keep free" ); c_free = Edit( ID_FREE, ES_NUMBER|ES_RIGHT );
  c_freeud = Make( UPDOWN_CLASSW, L"", UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_NOTHOUSANDS, ID_FREEUD );
  c_f = Button( L"only currently idle t&hreads", ID_F, BS_AUTOCHECKBOX );
  c_plan = Note();
  l_l = Label( L"&Levels" );
  l_lh = Label( L"video" ); c_lh = Combo( ID_LH, CBS_DROPDOWNLIST, l_items, 10 );
  l_la = Label( L"audio" ); c_la = Combo( ID_LA, CBS_DROPDOWNLIST, l_items, 10 );
  l_lt = Label( L"tangelo" ); c_lt = Combo( ID_LT, CBS_DROPDOWNLIST, l_items, 10 );

  g_info = Group( L"Container" );
  c_info = Make( L"EDIT", L"", WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL, ID_INFO, WS_EX_CLIENTEDGE );
  SendMessageW( c_info, WM_SETFONT, (WPARAM)mono, FALSE );
  info_edit = (WNDPROC)SetWindowLongPtrW( c_info, GWLP_WNDPROC, (LONG_PTR)InfoProc ); // (dragged to scroll)

  g_prog = Group( L"Progress" );
  c_phase = Note( ID_PHASE ); c_bar = Bar(); c_pct = Make( L"STATIC", L"", SS_RIGHT|SS_CENTERIMAGE, ID_PCT );
  c_time = Note();
  for( int i=0; i<NJ; i++ ) { c_jl[i] = Note(); c_jb[i] = Bar(); c_jp[i] = Make( L"STATIC", L"", SS_RIGHT|SS_CENTERIMAGE ); }

  c_log = Make( L"EDIT", L"", WS_TABSTOP|ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_HSCROLL|ES_AUTOVSCROLL|ES_AUTOHSCROLL, ID_LOG, WS_EX_CLIENTEDGE );
  SendMessageW( c_log, WM_SETFONT, (WPARAM)mono, FALSE );
  SendMessageW( c_log, EM_SETLIMITTEXT, 0x7FFFFFFE, 0 );
  c_status = Note( ID_STATUS );
  c_start = Button( L"&Start", ID_START, BS_DEFPUSHBUTTON );
  c_pause = Button( L"P&ause", ID_PAUSE );
  c_stop = Button( L"Sto&p", ID_STOP );

  SendMessageW( c_freeud, UDM_SETBUDDY, (WPARAM)c_free, 0 );
  SendMessageW( c_freeud, UDM_SETRANGE32, 0, std::max( cpu_threads-1, 0 ) );
  SendMessageW( c_w, EM_SETCUEBANNER, TRUE, (LPARAM)L"the temp directory" ); // (comctl32 6)
  const wchar_t* lab[] = { L"&Input", L"&Output", L"&Work dir", L"&Threads", L"&Levels", L"List" };
  lw = 0;
  for( int i=0; i<6; i++ ) lw = std::max( lw, TextWidth( lab[i] ) );
  lw += DX(6);
}

// a row of controls is rh high; the rest follows from the font, and the
// task rows from how many tasks there are
static void Layout() {
  RECT rc; GetClientRect( hwnd, &rc );
  int W = rc.right, H = rc.bottom;
  int m = DX(6), gap = DY(3), pad = DX(6), gc = by+DY(1), sp = DX(4); // gc: a group box's caption
  int x0 = m, wl = W-2*m, xr = x0+wl, bw = DX(18), y = m;
  int cw = TextWidth( L"CPU Info" )+DX(12);
  Place( c_comp, x0, y, DX(52), rh ); Place( c_dec, x0+DX(56), y, DX(60), rh );
  Place( c_verify, x0+DX(124), y, TextWidth( L"Verify" )+DX(16), rh );
  Place( c_cpu, xr-cw, y, cw, rh ); y += rh+gap;
  int clw = TextWidth( L"Clear" )+DX(12);
  Place( l_list, x0, y, lw, rh ); Place( c_list, x0+lw, y, wl-lw-bw-clw-2*sp, rh+DY(160) );
  Place( c_listclr, xr-bw-sp-clw, y, clw, rh ); Place( c_listb, xr-bw, y, bw, rh ); y += rh+gap;
  Place( l_in, x0, y, lw, rh ); Place( c_in, x0+lw, y, wl-lw-bw-sp, rh ); Place( c_inb, xr-bw, y, bw, rh ); y += rh+gap;
  Place( l_out, x0, y, lw, rh ); Place( c_out, x0+lw, y, wl-lw-bw-sp, rh ); Place( c_outb, xr-bw, y, bw, rh ); y += rh+gap;
  int kw = TextWidth( L"keep it" )+DX(16), ow = TextWidth( L"Output dir" )+DX(10), tw = TextWidth( L"%TEMP%" )+DX(10), a = xr;
  Place( c_k, a-kw, y, kw, rh ); a -= kw+sp;
  Place( c_wout, a-ow, y, ow, rh ); a -= ow+sp;
  Place( c_wtemp, a-tw, y, tw, rh ); a -= tw+sp;
  Place( c_wb, a-bw, y, bw, rh ); a -= bw+sp;
  Place( l_w, x0, y, lw, rh ); Place( c_w, x0+lw, y, a-x0-lw, rh ); y += rh+gap;

  int gy = y, x = x0+pad, w = wl-2*pad, xc = x+lw;
  y += gc;
  int tcw = DX(64), fw = DX(30), lfw = TextWidth( L"keep free" )+sp;
  Place( l_t, x, y, lw, rh ); Place( c_t, xc, y, tcw, rh+DY(80) );
  Place( l_free, xc+tcw+sp*2, y, lfw, rh ); Place( c_free, xc+tcw+sp*2+lfw, y, fw, rh );
  SendMessageW( c_freeud, UDM_SETBUDDY, (WPARAM)c_free, 0 ); // (it moves with it)
  a = xc+tcw+sp*4+lfw+fw;
  Place( c_f, a, y, x+w-a, rh ); y += rh;
  Place( c_plan, xc, y, x+w-xc, by+DY(3) ); y += by+DY(3)+gap;
  int lc = DX(50), w1 = TextWidth( L"video" )+sp, w2 = TextWidth( L"audio" )+sp, w3 = TextWidth( L"tangelo" )+sp;
  a = xc;
  Place( l_l, x, y, lw, rh );
  Place( l_lh, a, y, w1, rh ); a += w1; Place( c_lh, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_la, a, y, w2, rh ); a += w2; Place( c_la, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_lt, a, y, w3, rh ); a += w3; Place( c_lt, a, y, lc, rh+DY(100) ); y += rh+DY(4);
  Place( g_opt, x0, gy, wl, y-gy ); y += gap;

  gy = y; y += gc; // the input's container: 6 lines
  int ih = 6*mh+2*GetSystemMetrics( SM_CYEDGE )+2; // (the edit's 1 pixel above and below them; no part of a 7th: what is below the lines is not scrolled with them)
  Place( c_info, x0+pad, y, wl-2*pad, ih ); y += ih+DY(5);
  Place( g_info, x0, gy, wl, y-gy ); y += gap;

  gy = y; y += gc;
  int pw = DX(30), jw = DX(116), bx0 = x0+pad+jw, bw0 = xr-pad-pw-sp-bx0, jh = by+DY(4);
  Place( c_phase, x0+pad, y, jw, rh ); Place( c_bar, bx0, y+DY(2), bw0, rh-DY(4) ); Place( c_pct, xr-pad-pw, y, pw, rh ); y += rh;
  Place( c_time, x0+pad, y, wl-2*pad, jh ); y += jh+gap;
  for( int i=0; i<NJ; i++ ) {
    int yi = y+std::min( i, nrows )*jh; // (the hidden ones at the end)
    Place( c_jl[i], x0+pad+DX(8), yi, jw-DX(8), jh ); Place( c_jb[i], bx0, yi+DY(2), bw0, jh-DY(4) ); Place( c_jp[i], xr-pad-pw, yi, pw, jh );
  }
  y += nrows*jh+DY(4);
  Place( g_prog, x0, gy, wl, y-gy ); y += gap;

  log_y = y;
  int yb = H-m-rh, bb = DX(52);
  Place( c_log, x0, y, wl, yb-gap-y );
  Place( c_status, x0, yb, wl-3*bb-4*sp, rh );
  if( status_cut ) SetText( c_status, MiddleCut( c_status, status_text ) );
  Place( c_start, xr-3*bb-2*sp, yb, bb, rh );
  Place( c_pause, xr-2*bb-sp, yb, bb, rh );
  Place( c_stop, xr-bb, yb, bb, rh );
  InvalidateRect( hwnd, 0, TRUE );
}

// the window taller when the log would be too short (more task rows)
static void Fit() {
  RECT rc, wr, wa;
  GetClientRect( hwnd, &rc );
  int need = log_y+DY(36)+DY(3)+rh+DX(6);
  if( (rc.bottom>=need) || IsZoomed( hwnd ) ) return;
  GetWindowRect( hwnd, &wr );
  int h = wr.bottom-wr.top+need-rc.bottom;
  if( SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 ) ) h = std::min<int>( h, wa.bottom-wr.top );
  SetWindowPos( hwnd, 0, 0, 0, wr.right-wr.left, h, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
}

