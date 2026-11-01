// inc/gui_state.inc -- the window's controls (their IDs and handles),
//  messages, timers, fonts and sizes; the state of a run and of the list
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

enum {
  ID_COMP = 100, ID_DEC, ID_VERIFY, ID_CPU, ID_IN, ID_INB, ID_OUT, ID_OUTB, ID_W, ID_WB, ID_WTEMP, ID_WOUT, ID_K,
  ID_T, ID_FREE, ID_FREEUD, ID_F, ID_LH, ID_LA, ID_LT, ID_LOG, ID_START, ID_PAUSE, ID_STOP, ID_STATUS, ID_PHASE, ID_PCT, ID_INFO,
  ID_LIST, ID_LISTB, ID_LISTCLR
};
enum { WM_LOG = WM_APP+1, WM_PROG, WM_DONE, WM_INFO }; // from the run's thread; from the input's description's
enum { T_TICK = 1, T_OPTS, T_INFO };            // timers: the clock while it runs; options changed; the input changed
static const int NJ = 12;                       // task rows, at most

static HINSTANCE hinst;
static HWND hwnd, hcpu;           // the window; the CPU Info window
static HFONT font, mono;          // the system's message font; a fixed one
static int bx, by;                // the font's average character width and height
static int mh, mw;                // the fixed font's line height and character width
static int rh, lw;                // a row's height; the labels' width
static int log_y;                 // where the log begins (the rest is above it)
static int nrows;                 // task rows shown

static HWND c_comp, c_dec, c_verify, c_cpu, l_in, c_in, c_inb, l_out, c_out, c_outb, l_w, c_w, c_wb, c_wtemp, c_wout, c_k;
static HWND l_list, c_list, c_listb, c_listclr; // the list: its files, "...", Clear
static HWND g_opt, l_t, c_t, l_free, c_free, c_freeud, c_f, c_plan, l_l, l_lh, c_lh, l_la, c_la, l_lt, c_lt;
static HWND g_info, c_info;       // the input's container (the box above Progress)
static HWND g_prog, c_phase, c_bar, c_pct, c_time, c_jl[NJ], c_jb[NJ], c_jp[NJ];
static HWND c_log, c_status, c_start, c_pause, c_stop;
static int jl_gray[NJ], jp_color[NJ], pct_blue; // colors: a task waiting (gray); a gain (1, blue), failed (2, red)

static wstring out_auto;          // the output name we made from the input's (else the user's)
static wstring out_ok;            // an existing output the user chose to overwrite
static int f_overwrite, f_quit;   // -overwrite: any, without asking; -quit: quit when a run ends well
static int exit_code;             // the last run's, as mp4pack's
static int mode_dec;              // the mode as last set: a click that changes it swaps the files
static int swapping;              // (the input set by that: not a new one)
static string cl_c;               // -c and -i from the command line (no fields for them): for every run
static int cl_cap = -1;
static int plan_bad;
static string details;            // the CPU Info window's text (made when idle)

//---- the run

static HANDLE h_run;              // its thread
static int running, closing;      // closing: the window goes when it ends
static wstring run_in, run_out;
// the list: files done one after another (Start does those not done yet)
struct LItem { wstring path; int st; };      // st: 0 waiting, 1 running, 2 done, 3 failed
static vector<LItem> list;
static int list_run;              // the run is the list's
static vector<int> run_list;      // its files: their places in list
static vector<wstring> run_ins;   // and their paths
static int run_append, run_k;     // c into one archive, added to the one there; the file of the run now
static int run_dec, run_verify;   // d; c, then Verify's d
static qword run_t0, paused_at;
static CRITICAL_SECTION qlock;    // for what the run's thread hands over:
static string log_q;              //   messages (UTF-8 lines)
struct JobRow { string label; int st; double frac; int est, gain; qword work, made; }; // frac -1: not known; est: a guess; gain: c's or d's
struct Prog { string phase; double frac; qword eta; vector<JobRow> jobs; int file, fr; Prog() : frac(0), eta(0), file(-1), fr(0) {} }; // file>=0: a file of a list begins (fr -1) or ends (fr: how)
static vector<Prog> prog_q;       //   the progress: each change of a job's state or the phase (none lost when they come faster than the window looks)
static int log_posted, prog_posted;
static Prog prog;                 // the progress shown
static qword prog_at;             // when it came
static double shown;              // the overall bar (it does not go back)
static string result;             // the sizes and the time, when all went well

// a task: a job, or a stream's parts (aacpack's, tangelo's pieces) together
struct Task { string key, label; int st, est, gain; double frac; qword work, made; }; // made: the packed bytes
static vector<Task> tasks;        // this run's, as they came

