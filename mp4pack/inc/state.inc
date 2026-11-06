// inc/state.inc -- the settings (the options) and the state of a run
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- settings and state

static int th[N_GROUP];               // -t: threads of each group
static qword ch[N_GROUP];             // -c: part sizes, 0 = the default
static const int NO_LEVEL = -1000000;
static int lv[N_GROUP] = { NO_LEVEL, NO_LEVEL, NO_LEVEL, NO_LEVEL }; // -l: levels (NO_LEVEL: the tool's default)
static vector< vector<int> > stages;  // -t: groups run at once ('+'), stage after stage (',')
static int f_dec, f_verbose, f_keep, f_free;
static string t_spec, c_spec;         // -t and -c as given ("": none)
static wstring w_base;                // -w

static wstring exe_dir;               // ours and the tools', with a trailing separator
static wstring work;                  // the work directory, with a trailing separator
static HANDLE h_err, h_jobobj, ev_abort;
static DWORD tool_flags;              // the tools' CreateProcess flags: DETACHED_PROCESS from a GUI (else each opens a console)
static volatile LONG f_abort;
static volatile LONG f_pause;         // a frontend's Pause: no tool starts, those running are held
static HANDLE ev_wake;                // set with it, for the run to act at once
static int f_verify;                  // -verify: c, then d of the archive into a temporary file, as a check
static int f_con;                     // stderr is a console: there is a status line
static size_t status_len;             // its length on the screen
static qword t_start;                 // when the file's c or d began (set back by the pauses, as these)
static qword t_all;                   // when the run (of all its files) began
static qword t_run;                   // when what the status line follows began
static uint n_log;

