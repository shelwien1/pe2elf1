// inc/gui_taskbar.inc -- the taskbar button's progress (Windows 7+)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the taskbar's progress (Windows 7+; none where it is not there)

static const GUID clsid_taskbar = { 0x56fdf344, 0xfd6d, 0x11d0, { 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90 } };
static const GUID iid_taskbar3 = { 0xea1afb91, 0x9e28, 0x4b86, { 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf } };
static ITaskbarList3* taskbar;
static UINT wm_taskbar;           // "TaskbarButtonCreated": it can be used from then on

static void TaskbarInit() {
  if( taskbar ) return;
  void* p = 0;
  if( SUCCEEDED( CoCreateInstance( clsid_taskbar, 0, CLSCTX_INPROC_SERVER, iid_taskbar3, &p ) ) ) {
    taskbar = (ITaskbarList3*)p;
    if( FAILED( taskbar->HrInit() ) ) { taskbar->Release(); taskbar = 0; }
  }
}

static void TaskbarShow( TBPFLAG st, double f ) {
  if( !taskbar ) return;
  taskbar->SetProgressState( hwnd, st );
  if( (st==TBPF_NORMAL) || (st==TBPF_PAUSED) ) taskbar->SetProgressValue( hwnd, ULONGLONG(f*1000), 1000 );
}

