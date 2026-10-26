

static const byte C_Band_smc1_ic[24+1]={ 0,0,0,0,0,1,1,1,2,3,4,4,5,5,6,6,7,7,8,9,9,9,10,11,11 };
static const byte C_Band_smc1_id[24+1]={ 0,0,0,1,1,2,2,3,3,3,3,4,4,5,5,6,7,8,9,10,11,12,13,13,13 };
static const byte C_Band_smc1_ie[24+1]={ 0,0,0,1,1,2,3,3,3,4,5,5,5,5,5,6,6,6,6,7,8,9,10,10,10 };
static const byte C_Band_smc1_if[24+1]={ 0,0,0,1,1,1,1,1,1,2,2,3,3,3,4,5,5,6,6,6,6,7,8,9,9 };
static const byte C_Band_smc1_ig[24+1]={ 0,1,1,2,2,3,3,3,4,4,5,5,6,6,7,7,8,9,9,10,10,10,11,12,13 };
static const byte C_Band_smc1_ih[24+1]={ 0,1,2,3,3,4,5,6,7,8,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10 };
static const byte C_Band_smc1_ii[24+1]={ 0,0,1,2,2,3,3,3,3,4,4,5,5,5,5,5,5,5,5,6,6,6,7,8,8 };

static const byte C_Jglgain_e_i1[512]={ 0,0,1,2,3,3,4,4,5,5,5,5,6,6,6,6,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,9,10,9,10,9,10,9,10,9,10,9,10,9,10,9,10,11,12,11,12,11,12,11,12,11,12,11,12,11,12,11,12,13,13,14,14,13,13,14,14,13,13,14,14,13,13,14,14,13,13,14,14,13,13,14,14,13,13,14,14,13,13,14,14,15,15,16,16,15,15,16,16,15,15,16,16,15,15,16,16,15,15,16,16,15,15,16,16,15,15,16,16,15,15,16,16,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,17,18,17,18,19,20,19,20,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,21,22,21,22,23,24,23,24,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,25,26,27,28,25,26,27,28,29,30,31,32,29,30,31,32,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40,33,34,35,36,33,34,35,36,37,38,39,40,37,38,39,40 };

static const byte C_Jglgain_f_i1[512]={ 0,0,1,2,3,3,4,4,5,5,5,5,6,6,6,6,7,8,7,8,7,8,7,8,9,10,9,10,9,10,9,10,11,12,13,14,11,12,13,14,11,12,13,14,11,12,13,14,15,16,17,18,15,16,17,18,15,16,17,18,15,16,17,18,19,19,20,20,21,21,22,22,19,19,20,20,21,21,22,22,19,19,20,20,21,21,22,22,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,23,23,24,24,25,25,26,26,23,23,24,24,25,25,26,26,23,23,24,24,25,25,26,26,27,28,27,28,29,30,29,30,31,32,31,32,33,34,33,34,27,28,27,28,29,30,29,30,31,32,31,32,33,34,33,34,27,28,27,28,29,30,29,30,31,32,31,32,33,34,33,34,27,28,27,28,29,30,29,30,31,32,31,32,33,34,33,34,35,36,35,36,37,38,37,38,39,40,39,40,41,42,41,42,35,36,35,36,37,38,37,38,39,40,39,40,41,42,41,42,35,36,35,36,37,38,37,38,39,40,39,40,41,42,41,42,35,36,35,36,37,38,37,38,39,40,39,40,41,42,41,42,43,44,45,46,43,44,45,46,47,48,49,50,47,48,49,50,51,52,53,54,51,52,53,54,55,56,57,58,55,56,57,58,43,44,45,46,43,44,45,46,47,48,49,50,47,48,49,50,51,52,53,54,51,52,53,54,55,56,57,58,55,56,57,58,43,44,45,46,43,44,45,46,47,48,49,50,47,48,49,50,51,52,53,54,51,52,53,54,55,56,57,58,55,56,57,58,43,44,45,46,43,44,45,46,47,48,49,50,47,48,49,50,51,52,53,54,51,52,53,54,55,56,57,58,55,56,57,58,59,60,61,62,59,60,61,62,63,64,65,66,63,64,65,66,67,68,69,70,67,68,69,70,71,72,73,74,71,72,73,74,59,60,61,62,59,60,61,62,63,64,65,66,63,64,65,66,67,68,69,70,67,68,69,70,71,72,73,74,71,72,73,74,59,60,61,62,59,60,61,62,63,64,65,66,63,64,65,66,67,68,69,70,67,68,69,70,71,72,73,74,71,72,73,74,59,60,61,62,59,60,61,62,63,64,65,66,63,64,65,66,67,68,69,70,67,68,69,70,71,72,73,74,71,72,73,74 };

static const byte C_Jglgain_g_i1[512]={ 0,0,1,1,2,3,2,3,4,4,5,5,4,4,5,5,6,7,6,7,8,9,8,9,6,7,6,7,8,9,8,9,10,11,12,13,10,11,12,13,14,15,16,17,14,15,16,17,10,11,12,13,10,11,12,13,14,15,16,17,14,15,16,17,18,19,20,21,22,23,24,25,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,26,27,28,29,30,31,32,33,18,19,20,21,22,23,24,25,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129 };

static const byte C_Jrg0cnt_e_i1[32]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30 };


static const byte C_Jrg0cnt_g_i1[32]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30 };

static const byte C_Jrg1cnt_e_i1[16]={ 0,0,1,2,3,4,5,6,7,7,8,8,9,9,10,10 };


static const byte C_Jrg1cnt_g_i1[16]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };




static const byte C_Jxlong_e_i1[32]={ 0,0,1,1,2,3,2,3,4,5,6,7,4,5,6,7,8,9,10,11,12,13,14,15,8,9,10,11,12,13,14,15 };

static const byte C_Jxlong_f_i1[32]={ 0,0,1,1,2,2,2,2,3,4,3,4,3,4,3,4,5,6,7,8,5,6,7,8,5,6,7,8,5,6,7,8 };

static const byte C_Jxlong_g_i1[32]={ 0,0,1,1,2,3,2,3,4,5,6,7,4,5,6,7,8,9,10,11,12,13,14,15,8,9,10,11,12,13,14,15 };

static const byte C_Sglgain_d_i2[32+1]={ 0,0,1,1,1,1,1,1,1,1,1,2,2,3,4,4,5,5,5,5,6,6,6,6,6,6,6,7,8,8,8,8,8 };






static const byte C_Sxlong_d_i1[20+1]={ 0,1,1,1,1,1,2,2,2,2,2,3,3,3,4,4,4,5,6,7,8 };














static const byte C_pxlong_e_i1[20+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };

static const byte C_pxlong_f_i1[20+1]={ 0,1,2,3,4,5,6,6,7,7,8,9,10,10,11,12,13,14,15,16,17 };




static const byte C_xlong_c_i1[20+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,13,14,15,16,17,18,19 };


static const int C_count00_mw = (1+0) * (1);
static const word* C_count00_wr = TTable[31+0];
static const int C_count10_mw = (8+0) * (1);
static const word* C_count10_wr = TTable[28+0];
static const int C_count1_mw = (11776+0) * (1);
static const word* C_count1_wr0 = TTable[25+0];
static const word* C_count1_wr1 = TTable[20+0];
static const int C_count0_mw = (7126+0) * (1);
static const word* C_count0_wr0 = TTable[23+0];
static const word* C_count0_wr1 = TTable[16+0];
static const int C_count1_bas = (0+0) * (1);
static const int C_count0_bas = (219+0) * (1);
static const int C_xlong_bas = (0+0) * (1);
static const int C_xlong_mw4 = (6684+0) * (1);
static const int C_xlong_mw3 = (3616+0) * (1);
static const int C_xlong_mw2 = (3968+0) * (1);
static const int C_xlong_mw1 = (7296+0) * (1);
static const int C_xlong_mw0 = (4276+0) * (1);
static const word* C_xlong_wr4 = TTable[24+0];
static const word* C_xlong_wr3 = TTable[23+0];
static const word* C_xlong_wr2 = TTable[24+0];
static const word* C_xlong_wr1 = TTable[25+0];
static const word* C_xlong_wr0 = TTable[24+0];
static const word* C_xlong_wq4 = TTable[15+0];
static const word* C_xlong_wq3 = TTable[1+0];
static const word* C_xlong_wq2 = TTable[12+0];
static const word* C_xlong_wq1 = TTable[31+0];
static const word* C_xlong_wq0 = TTable[15+0];
static const int C_Sxlong_Q = (3+2) * (1);
static const int C_Sxlong_wi = (256+0) * (4);
static const int C_Sxlong_ti = (17+0) * (1);
static const int C_glgain_bas = (170+0) * (1);
static const int C_glgain_mw8 = (7796+0) * (1);
static const int C_glgain_mw7 = (8016+0) * (1);
static const int C_glgain_mw6 = (7301+0) * (1);
static const int C_glgain_mw5 = (5680+0) * (1);
static const int C_glgain_mw4 = (2052+0) * (1);
static const int C_glgain_mw3 = (2688+0) * (1);
static const int C_glgain_mw2 = (7208+0) * (1);
static const int C_glgain_mw1 = (1132+0) * (1);
static const int C_glgain_mw0 = (3456+0) * (1);
static const word* C_glgain_wr8 = TTable[25+0];
static const word* C_glgain_wr7 = TTable[25+0];
static const word* C_glgain_wr6 = TTable[25+0];
static const word* C_glgain_wr5 = TTable[25+0];
static const word* C_glgain_wr4 = TTable[19+0];
static const word* C_glgain_wr3 = TTable[27+0];
static const word* C_glgain_wr2 = TTable[30+0];
static const word* C_glgain_wr1 = TTable[10+0];
static const word* C_glgain_wr0 = TTable[25+0];
static const word* C_glgain_wq8 = TTable[0+0];
static const word* C_glgain_wq7 = TTable[2+0];
static const word* C_glgain_wq6 = TTable[15+0];
static const word* C_glgain_wq5 = TTable[6+0];
static const word* C_glgain_wq4 = TTable[15+0];
static const word* C_glgain_wq3 = TTable[30+0];
static const word* C_glgain_wq2 = TTable[31+0];
static const word* C_glgain_wq1 = TTable[31+0];
static const word* C_glgain_wq0 = TTable[3+0];
static const int C_Sglgain_Q = (3+2) * (1);
static const int C_Sglgain_wi = (160+0) * (4);
static const int C_Sglgain_ti = (20+0) * (1);
static const int C_sfcomp_bas = (12+0) * (1);
static const int C_sfcomp_mw4 = (5+0) * (1);
static const int C_sfcomp_mw3 = (3095+0) * (1);
static const int C_sfcomp_mw2 = (4160+0) * (1);
static const int C_sfcomp_mw1 = (1644+0) * (1);
static const int C_sfcomp_mw0 = (16+0) * (1);
static const word* C_sfcomp_wr4 = TTable[25+0];
static const word* C_sfcomp_wr3 = TTable[26+0];
static const word* C_sfcomp_wr2 = TTable[16+0];
static const word* C_sfcomp_wr1 = TTable[22+0];
static const word* C_sfcomp_wr0 = TTable[30+0];
static const word* C_sfcomp_wq4 = TTable[16+0];
static const word* C_sfcomp_wq3 = TTable[3+0];
static const word* C_sfcomp_wq2 = TTable[0+0];
static const word* C_sfcomp_wq1 = TTable[15+0];
static const word* C_sfcomp_wq0 = TTable[12+0];
static const int C_Ssfcomp_Q = (3+2) * (1);
static const int C_Ssfcomp_wi = (1344+0) * (4);
static const int C_Ssfcomp_ti = (12+0) * (1);
static const int C_rg0cnt_bas = (9+0) * (1);
static const int C_rg0cnt_mw4 = (3840+0) * (1);
static const int C_rg0cnt_mw3 = (2248+0) * (1);
static const int C_rg0cnt_mw2 = (2584+0) * (1);
static const int C_rg0cnt_mw1 = (3337+0) * (1);
static const int C_rg0cnt_mw0 = (4128+0) * (1);
static const word* C_rg0cnt_wr4 = TTable[27+0];
static const word* C_rg0cnt_wr3 = TTable[26+0];
static const word* C_rg0cnt_wr2 = TTable[27+0];
static const word* C_rg0cnt_wr1 = TTable[23+0];
static const word* C_rg0cnt_wr0 = TTable[26+0];
static const word* C_rg0cnt_wq4 = TTable[5+0];
static const word* C_rg0cnt_wq3 = TTable[2+0];
static const word* C_rg0cnt_wq2 = TTable[17+0];
static const word* C_rg0cnt_wq1 = TTable[11+0];
static const word* C_rg0cnt_wq0 = TTable[5+0];
static const int C_Srg0cnt_Q = (3+2) * (1);
static const int C_Srg0cnt_wi = (1576+0) * (4);
static const int C_Srg0cnt_ti = (28+0) * (1);
static const int C_rg1cnt_bas = (1+0) * (1);
static const int C_rg1cnt_mw3 = (192+0) * (1);
static const int C_rg1cnt_mw2 = (3646+0) * (1);
static const int C_rg1cnt_mw1 = (4624+0) * (1);
static const int C_rg1cnt_mw0 = (3233+0) * (1);
static const word* C_rg1cnt_wr3 = TTable[26+0];
static const word* C_rg1cnt_wr2 = TTable[26+0];
static const word* C_rg1cnt_wr1 = TTable[26+0];
static const word* C_rg1cnt_wr0 = TTable[27+0];
static const word* C_rg1cnt_wq3 = TTable[2+0];
static const word* C_rg1cnt_wq2 = TTable[3+0];
static const word* C_rg1cnt_wq1 = TTable[3+0];
static const word* C_rg1cnt_wq0 = TTable[5+0];
static const int C_Srg1cnt_Q = (3+2) * (1);
static const int C_Srg1cnt_wi = (1304+0) * (4);
static const int C_Srg1cnt_ti = (18+0) * (1);


static const qword C_Band_smc0_Volume = 1ULL+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (1-1)+ (2-1)+ (1-1)+ (1-1)+ (2-1)+ (1-1)+ (1-1)+ (2-1)+ (1-1)+ (2-1)+ (1-1)+ (1-1)+ (1-1)+ (2-1); // 6
static const qword C_Band_smc1_Volume = 1ULL+ (1-1)+ (2-1)+ (1-1)+ (2-1)+ (5-1)+ (4-1)+ (5-1)+ (3-1)+ (2-1)+ (1-1)+ (1-1)+ (2-1)+ (12-1)+ (14-1)+ (11-1)+ (10-1)+ (14-1)+ (11-1)+ (9-1)+ (3-1)+ (1-1)+ (3-1)+ (1-1)+ (6-1); // 101
static const qword C_Jglgain_e_Volume = 1ULL* (1<<1)* 41; // 82
static const qword C_Jglgain_f_Volume = 1ULL* 75; // 75
static const qword C_Jglgain_g_Volume = 1ULL* 130; // 130
static const qword C_Jrg0cnt_e_Volume = 1ULL* 31; // 31
static const qword C_Jrg0cnt_f_Volume = 1ULL* 32; // 31
static const qword C_Jrg0cnt_g_Volume = 1ULL* 31; // 31
static const qword C_Jrg1cnt_e_Volume = 1ULL* (1<<1)* 11; // 22
static const qword C_Jrg1cnt_f_Volume = 1ULL* 16; // 11
static const qword C_Jrg1cnt_g_Volume = 1ULL* 15; // 15
static const qword C_Jsfcomp_e_Volume = 1ULL* 32; // 75
static const qword C_Jsfcomp_f_Volume = 1ULL* 32; // 75
static const qword C_Jsfcomp_g_Volume = 1ULL* 32; // 75
static const qword C_Jxlong_e_Volume = 1ULL* (1<<1)* 16; // 32
static const qword C_Jxlong_f_Volume = 1ULL* 9; // 9
static const qword C_Jxlong_g_Volume = 1ULL* 16; // 16
static const qword C_Sglgain_d_Volume = 1ULL* 1* 1* 9* C_Jglgain_e_Volume; // 0
static const qword C_Smc0_d_Volume = 1ULL* 1* 1* 2; // 2
static const qword C_Smc1_d_Volume = 1ULL* 1* 1* 3; // 3
static const qword C_Srg0cnt_d_Volume = 1ULL* 2* 2* C_Jrg0cnt_e_Volume; // 0
static const qword C_Srg1cnt_d_Volume = 1ULL* 1* 2* 7* C_Jrg1cnt_e_Volume; // 0
static const qword C_Ssfcomp_d_Volume = 1ULL* 1* 5* (1<<3)* (1<<3)* C_Jsfcomp_e_Volume; // 0
static const qword C_Sxlong_d_Volume = 1ULL* 1* 9* 3* 2* 2* C_Jxlong_e_Volume; // 0
static const qword C_glgain_c_Volume = 1ULL* 1* 1* 1* (1<<7)* C_Jglgain_f_Volume; // 0
static const qword C_mc00_e_Volume = 1ULL* 1* 1* 1; // 1
static const qword C_mc0_e_Volume = 1ULL* 1* 1* 4* C_Band_smc0_Volume; // 0
static const qword C_mc10_e_Volume = 1ULL* 1* 1* 1; // 1
static const qword C_mc1_e_Volume = 1ULL* 2* 1* 2* C_Band_smc1_Volume; // 0
static const qword C_pglgain_e_Volume = 1ULL* 2* 1; // 2
static const qword C_pglgain_f_Volume = 1ULL* 1* 2* 2* C_Jglgain_g_Volume; // 0
static const qword C_prg0cnt_e_Volume = 1ULL* 1; // 1
static const qword C_prg0cnt_f_Volume = 1ULL* 2* 8* (1<<1)* C_Jrg0cnt_g_Volume; // 0
static const qword C_prg1cnt_e_Volume = 1ULL* 1; // 1
static const qword C_prg1cnt_f_Volume = 1ULL* 1* 3* (1<<4)* C_Jrg1cnt_g_Volume; // 0
static const qword C_psfcomp_e_Volume = 1ULL* 1; // 1
static const qword C_psfcomp_f_Volume = 1ULL* 1* 6* (1<<4)* C_Jsfcomp_g_Volume; // 0
static const qword C_pxlong_e_Volume = 1ULL* 2* 21* 3* 2; // 252
static const qword C_pxlong_f_Volume = 1ULL* 1* 18* 5* 1* 4* C_Jxlong_g_Volume; // 0
static const qword C_rg0cnt_c_Volume = 1ULL* 1* 1* C_Jrg0cnt_f_Volume; // 0
static const qword C_rg1cnt_c_Volume = 1ULL* 1* 1* (1<<4)* C_Jrg1cnt_f_Volume; // 0
static const qword C_sfcomp_c_Volume = 1ULL* 1* 8* (1<<3)* C_Jsfcomp_f_Volume; // 0
static const qword C_xlong_c_Volume = 1ULL* 1* 20* 7* 3* (1<<2)* C_Jxlong_f_Volume; // 0

struct C_T {

  typedef Number2ii<10,1023> Number2c1;
  typedef SSEii<5,5,1> SSE_c1[(1024+1023)/2+1];
  typedef Number2ii<10,1023> Number2c0;
  typedef SSEii<5,5,2> SSE_c0[(1024+1023)/2+1];

//  SSEj<*(int*)& C_Sxlong_Q,*(int*)& C_Sxlong_d_Volume> Sxlong;
//  SSEj<*(int*)& C_Sglgain_Q,*(int*)& C_Sglgain_d_Volume> Sglgain;
//  SSEj<*(int*)& C_Ssfcomp_Q,*(int*)& C_Ssfcomp_d_Volume> Ssfcomp;
//  SSEj<*(int*)& C_Srg0cnt_Q,*(int*)& C_Srg0cnt_d_Volume> Srg0cnt;
//  SSEj<*(int*)& C_Srg1cnt_Q,*(int*)& C_Srg1cnt_d_Volume> Srg1cnt;

  int p_c1[8];
  int p_c0[8];

//  Number1ii<4>    x_sfcomp[2];
//  Number1ii<4>    x_rg0cnt[2];
//  Number1ii<3>    x_rg1cnt[32];


  Number2c0 count0[C_mc0_e_Volume];
  SSE_c0 Scount0[C_Smc0_d_Volume];
  Node2ii count00[C_mc00_e_Volume];
  Number2c1 count1[C_mc1_e_Volume];
  SSE_c1 Scount1[C_Smc1_d_Volume];
  Node2ii count10[C_mc10_e_Volume];
  Node2ii xlong[C_xlong_c_Volume];
  SSEii<5> Sxlong[C_Sxlong_d_Volume];
  byte pxlong[C_pxlong_e_Volume];
  byte qxlong[C_pxlong_f_Volume];
  Node2ii glgain[C_glgain_c_Volume];
  SSEii<5> Sglgain[C_Sglgain_d_Volume];
  byte qglgain[C_pglgain_f_Volume];
  byte pglgain[C_pglgain_e_Volume];
  byte pglgaio[C_pglgain_e_Volume];
  Node2ii sfcomp[C_sfcomp_c_Volume];
  SSEii<5> Ssfcomp[C_Ssfcomp_d_Volume];
  byte qsfcomp[C_psfcomp_f_Volume];
  byte psfcomp[C_psfcomp_e_Volume];
  byte psfcomq[C_psfcomp_e_Volume];
  Node2ii rg0cnt[C_rg0cnt_c_Volume];
  SSEii<5> Srg0cnt[C_Srg0cnt_d_Volume];
  byte qrg0cnt[C_prg0cnt_f_Volume];
  byte prg0cnt[C_prg0cnt_e_Volume];
  byte prg0cnu[C_prg0cnt_e_Volume];
  Node2ii rg1cnt[C_rg1cnt_c_Volume];
  SSEii<5> Srg1cnt[C_Srg1cnt_d_Volume];
  byte qrg1cnt[C_prg1cnt_f_Volume];
  byte prg1cnt[C_prg1cnt_e_Volume];
  byte prg1cnu[C_prg1cnt_e_Volume];
  
  void C_Init( void ) {
  
    int i;
  
    for( i=0; i<C_mc0_e_Volume; i++ ) count0[i].Init_Number2i();
    for( i=0; i<C_Smc0_d_Volume; i++ ) Init_SSEii( Scount0[i] );
    for( i=0; i<C_mc00_e_Volume; i++ ) count00[i].Init_Node2i();
  
    for( i=0; i<C_mc1_e_Volume; i++ ) count1[i].Init_Number2i();
    for( i=0; i<C_Smc1_d_Volume; i++ ) Init_SSEii( Scount1[i] );
    for( i=0; i<C_mc10_e_Volume; i++ ) count10[i].Init_Node2i();
  
    memset( &p_c0, 0, sizeof(p_c0) );
    memset( &p_c1, 0, sizeof(p_c1) );
  
    for( i=0; i<C_xlong_c_Volume; i++ ) xlong[i].Init_Node2i();
    for( i=0; i<C_Sxlong_d_Volume; i++ ) Sxlong[i].Init_SSEi(C_Sxlong_wi,C_Sxlong_ti);
    for( i=0; i<C_pxlong_e_Volume; i++ ) pxlong[i]=0;
    for( i=0; i<C_pxlong_f_Volume; i++ ) qxlong[i]=0;
  
  //  Init_Number1ii( x_glgain );
    for( i=0; i<C_glgain_c_Volume; i++ ) glgain[i].Init_Node2i();
    for( i=0; i<C_Sglgain_d_Volume; i++ ) Sglgain[i].Init_SSEi(C_Sglgain_wi,C_Sglgain_ti);
    for( i=0; i<C_pglgain_f_Volume; i++ ) qglgain[i]=0;
    for( i=0; i<C_pglgain_e_Volume; i++ ) pglgain[i]=0;
    for( i=0; i<C_pglgain_e_Volume; i++ ) pglgaio[i]=0;
  
  //  Init_Number1ii( x_sfcomp );
    for( i=0; i<C_sfcomp_c_Volume; i++ ) sfcomp[i].Init_Node2i();
    for( i=0; i<C_Ssfcomp_d_Volume; i++ ) Ssfcomp[i].Init_SSEi(C_Ssfcomp_wi,C_Ssfcomp_ti);
    for( i=0; i<C_psfcomp_f_Volume; i++ ) qsfcomp[i]=0;
    for( i=0; i<C_psfcomp_e_Volume; i++ ) psfcomp[i]=0;
    for( i=0; i<C_psfcomp_e_Volume; i++ ) psfcomq[i]=0;
  
  //  Init_Number1ii( x_rg0cnt );
    for( i=0; i<C_rg0cnt_c_Volume; i++ ) rg0cnt[i].Init_Node2i();
    for( i=0; i<C_Srg0cnt_d_Volume; i++ ) Srg0cnt[i].Init_SSEi(C_Srg0cnt_wi,C_Srg0cnt_ti);
    for( i=0; i<C_prg0cnt_f_Volume; i++ ) qrg0cnt[i]=0;
    for( i=0; i<C_prg0cnt_e_Volume; i++ ) prg0cnt[i]=0;
    for( i=0; i<C_prg0cnt_e_Volume; i++ ) prg0cnu[i]=0;
  
  //  Init_Number1ii( x_rg1cnt );
    for( i=0; i<C_rg1cnt_c_Volume; i++ ) rg1cnt[i].Init_Node2i();
    for( i=0; i<C_Srg1cnt_d_Volume; i++ ) Srg1cnt[i].Init_SSEi(C_Srg1cnt_wi,C_Srg1cnt_ti);
    for( i=0; i<C_prg1cnt_f_Volume; i++ ) qrg1cnt[i]=0;
    for( i=0; i<C_prg1cnt_e_Volume; i++ ) prg1cnt[i]=0;
    for( i=0; i<C_prg1cnt_e_Volume; i++ ) prg1cnu[i]=0;
  
  }
  void C_Quit( void ) {
  }
};
