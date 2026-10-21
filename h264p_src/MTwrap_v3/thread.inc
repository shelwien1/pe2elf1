
template <class child> 
struct thread {

  HANDLE th;
  volatile uint f_done; // processing complete
  HANDLE ev_exit;       // optional (0 = none): set after f_done, so a waiter on it sees the exit

  uint thread_start( void ) {
    f_done=0;
    th = CreateThread( 0, 0, &thread_w, this, 0, 0 );
    return th!=0;
  }

  void thread_quit( void ) {
    WaitForSingleObject( th, INFINITE );
    CloseHandle( th );
  }

  // wrappers to redirect static calls into object method calls
  static DWORD WINAPI thread_w( LPVOID lpParameter ) { 
    ((child*)lpParameter)->thread(); 
    ((child*)lpParameter)->f_done=1; 
    if( ((child*)lpParameter)->ev_exit ) SetEvent( ((child*)lpParameter)->ev_exit );
    //printf( "!thread done. this=%I64X f_done=%i = &%I64X\n", lpParameter, ((child*)lpParameter)->f_done, &((child*)lpParameter)->f_done );
    return 0; 
  }
};

