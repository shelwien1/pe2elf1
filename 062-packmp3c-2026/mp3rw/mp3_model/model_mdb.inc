
Node2ii      f_mdb;
Number1ii<9> x_maindata;

void Init_MDB( void ) {
  f_mdb.Init_Node2i();
  x_maindata.Init_Number1i();
}

void ProcessMDB( void ) {

  int flag;
  int p_mdb = GrbPrev;//(rc.ProcMode?GrbPtrW:GrbPtrR);
  RangeCoder& rc = *this;

  p_mdb = ((p_mdb==-1)||fstframe())? 0 : (p_mdb-GrbLen)/8; // predicted m_d_b value

  flag = (p_mdb!=si.backup_mdb);
#ifndef DISABLE_f_mdb
  f_mdb.Process( rc, flag, M_f_mdb_wr, M_f_mdb_mw );
#endif
  if( flag ) {
#ifndef DISABLE_mdbmain
    x_maindata.Process( rc, si.backup_mdb, -1, 512, M_x_maindata_wr, M_x_maindata_mw );
#endif
  } else {
    si.backup_mdb = p_mdb;
  }

  si.main_data_begin = si.backup_mdb;
}
