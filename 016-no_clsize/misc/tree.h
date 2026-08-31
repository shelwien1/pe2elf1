  // node 1
  rpre = (range>>SCALElog)*cty[1];
  if( code >= rpre ) {
    range -= rpre; code -= rpre; upd(1,1); RENORM
    // node 3
    rpre = (range>>SCALElog)*cty[3];
    if( code >= rpre ) {
      range -= rpre; code -= rpre; upd(3,1); RENORM
      // node 7
      rpre = (range>>SCALElog)*cty[7];
      if( code >= rpre ) {
        range -= rpre; code -= rpre; upd(7,1); RENORM
        // node 15
        rpre = (range>>SCALElog)*cty[15];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(15,1); RENORM
          // node 31
          rpre = (range>>SCALElog)*cty[31];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(31,1); RENORM
            // node 63
            rpre = (range>>SCALElog)*cty[63];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(63,1); RENORM
              // node 127
              rpre = (range>>SCALElog)*cty[127];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(127,1); RENORM
                // node 255
                rpre = (range>>SCALElog)*cty[255];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 511; upd(255,1); } else { range = rpre; sym = 510; upd(255,0); }
                RENORM
              } else {
                range = rpre; upd(127,0); RENORM
                // node 254
                rpre = (range>>SCALElog)*cty[254];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 509; upd(254,1); } else { range = rpre; sym = 508; upd(254,0); }
                RENORM
              }
            } else {
              range = rpre; upd(63,0); RENORM
              // node 126
              rpre = (range>>SCALElog)*cty[126];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(126,1); RENORM
                // node 253
                rpre = (range>>SCALElog)*cty[253];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 507; upd(253,1); } else { range = rpre; sym = 506; upd(253,0); }
                RENORM
              } else {
                range = rpre; upd(126,0); RENORM
                // node 252
                rpre = (range>>SCALElog)*cty[252];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 505; upd(252,1); } else { range = rpre; sym = 504; upd(252,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(31,0); RENORM
            // node 62
            rpre = (range>>SCALElog)*cty[62];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(62,1); RENORM
              // node 125
              rpre = (range>>SCALElog)*cty[125];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(125,1); RENORM
                // node 251
                rpre = (range>>SCALElog)*cty[251];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 503; upd(251,1); } else { range = rpre; sym = 502; upd(251,0); }
                RENORM
              } else {
                range = rpre; upd(125,0); RENORM
                // node 250
                rpre = (range>>SCALElog)*cty[250];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 501; upd(250,1); } else { range = rpre; sym = 500; upd(250,0); }
                RENORM
              }
            } else {
              range = rpre; upd(62,0); RENORM
              // node 124
              rpre = (range>>SCALElog)*cty[124];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(124,1); RENORM
                // node 249
                rpre = (range>>SCALElog)*cty[249];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 499; upd(249,1); } else { range = rpre; sym = 498; upd(249,0); }
                RENORM
              } else {
                range = rpre; upd(124,0); RENORM
                // node 248
                rpre = (range>>SCALElog)*cty[248];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 497; upd(248,1); } else { range = rpre; sym = 496; upd(248,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(15,0); RENORM
          // node 30
          rpre = (range>>SCALElog)*cty[30];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(30,1); RENORM
            // node 61
            rpre = (range>>SCALElog)*cty[61];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(61,1); RENORM
              // node 123
              rpre = (range>>SCALElog)*cty[123];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(123,1); RENORM
                // node 247
                rpre = (range>>SCALElog)*cty[247];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 495; upd(247,1); } else { range = rpre; sym = 494; upd(247,0); }
                RENORM
              } else {
                range = rpre; upd(123,0); RENORM
                // node 246
                rpre = (range>>SCALElog)*cty[246];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 493; upd(246,1); } else { range = rpre; sym = 492; upd(246,0); }
                RENORM
              }
            } else {
              range = rpre; upd(61,0); RENORM
              // node 122
              rpre = (range>>SCALElog)*cty[122];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(122,1); RENORM
                // node 245
                rpre = (range>>SCALElog)*cty[245];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 491; upd(245,1); } else { range = rpre; sym = 490; upd(245,0); }
                RENORM
              } else {
                range = rpre; upd(122,0); RENORM
                // node 244
                rpre = (range>>SCALElog)*cty[244];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 489; upd(244,1); } else { range = rpre; sym = 488; upd(244,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(30,0); RENORM
            // node 60
            rpre = (range>>SCALElog)*cty[60];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(60,1); RENORM
              // node 121
              rpre = (range>>SCALElog)*cty[121];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(121,1); RENORM
                // node 243
                rpre = (range>>SCALElog)*cty[243];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 487; upd(243,1); } else { range = rpre; sym = 486; upd(243,0); }
                RENORM
              } else {
                range = rpre; upd(121,0); RENORM
                // node 242
                rpre = (range>>SCALElog)*cty[242];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 485; upd(242,1); } else { range = rpre; sym = 484; upd(242,0); }
                RENORM
              }
            } else {
              range = rpre; upd(60,0); RENORM
              // node 120
              rpre = (range>>SCALElog)*cty[120];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(120,1); RENORM
                // node 241
                rpre = (range>>SCALElog)*cty[241];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 483; upd(241,1); } else { range = rpre; sym = 482; upd(241,0); }
                RENORM
              } else {
                range = rpre; upd(120,0); RENORM
                // node 240
                rpre = (range>>SCALElog)*cty[240];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 481; upd(240,1); } else { range = rpre; sym = 480; upd(240,0); }
                RENORM
              }
            }
          }
        }
      } else {
        range = rpre; upd(7,0); RENORM
        // node 14
        rpre = (range>>SCALElog)*cty[14];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(14,1); RENORM
          // node 29
          rpre = (range>>SCALElog)*cty[29];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(29,1); RENORM
            // node 59
            rpre = (range>>SCALElog)*cty[59];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(59,1); RENORM
              // node 119
              rpre = (range>>SCALElog)*cty[119];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(119,1); RENORM
                // node 239
                rpre = (range>>SCALElog)*cty[239];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 479; upd(239,1); } else { range = rpre; sym = 478; upd(239,0); }
                RENORM
              } else {
                range = rpre; upd(119,0); RENORM
                // node 238
                rpre = (range>>SCALElog)*cty[238];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 477; upd(238,1); } else { range = rpre; sym = 476; upd(238,0); }
                RENORM
              }
            } else {
              range = rpre; upd(59,0); RENORM
              // node 118
              rpre = (range>>SCALElog)*cty[118];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(118,1); RENORM
                // node 237
                rpre = (range>>SCALElog)*cty[237];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 475; upd(237,1); } else { range = rpre; sym = 474; upd(237,0); }
                RENORM
              } else {
                range = rpre; upd(118,0); RENORM
                // node 236
                rpre = (range>>SCALElog)*cty[236];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 473; upd(236,1); } else { range = rpre; sym = 472; upd(236,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(29,0); RENORM
            // node 58
            rpre = (range>>SCALElog)*cty[58];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(58,1); RENORM
              // node 117
              rpre = (range>>SCALElog)*cty[117];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(117,1); RENORM
                // node 235
                rpre = (range>>SCALElog)*cty[235];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 471; upd(235,1); } else { range = rpre; sym = 470; upd(235,0); }
                RENORM
              } else {
                range = rpre; upd(117,0); RENORM
                // node 234
                rpre = (range>>SCALElog)*cty[234];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 469; upd(234,1); } else { range = rpre; sym = 468; upd(234,0); }
                RENORM
              }
            } else {
              range = rpre; upd(58,0); RENORM
              // node 116
              rpre = (range>>SCALElog)*cty[116];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(116,1); RENORM
                // node 233
                rpre = (range>>SCALElog)*cty[233];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 467; upd(233,1); } else { range = rpre; sym = 466; upd(233,0); }
                RENORM
              } else {
                range = rpre; upd(116,0); RENORM
                // node 232
                rpre = (range>>SCALElog)*cty[232];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 465; upd(232,1); } else { range = rpre; sym = 464; upd(232,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(14,0); RENORM
          // node 28
          rpre = (range>>SCALElog)*cty[28];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(28,1); RENORM
            // node 57
            rpre = (range>>SCALElog)*cty[57];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(57,1); RENORM
              // node 115
              rpre = (range>>SCALElog)*cty[115];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(115,1); RENORM
                // node 231
                rpre = (range>>SCALElog)*cty[231];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 463; upd(231,1); } else { range = rpre; sym = 462; upd(231,0); }
                RENORM
              } else {
                range = rpre; upd(115,0); RENORM
                // node 230
                rpre = (range>>SCALElog)*cty[230];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 461; upd(230,1); } else { range = rpre; sym = 460; upd(230,0); }
                RENORM
              }
            } else {
              range = rpre; upd(57,0); RENORM
              // node 114
              rpre = (range>>SCALElog)*cty[114];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(114,1); RENORM
                // node 229
                rpre = (range>>SCALElog)*cty[229];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 459; upd(229,1); } else { range = rpre; sym = 458; upd(229,0); }
                RENORM
              } else {
                range = rpre; upd(114,0); RENORM
                // node 228
                rpre = (range>>SCALElog)*cty[228];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 457; upd(228,1); } else { range = rpre; sym = 456; upd(228,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(28,0); RENORM
            // node 56
            rpre = (range>>SCALElog)*cty[56];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(56,1); RENORM
              // node 113
              rpre = (range>>SCALElog)*cty[113];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(113,1); RENORM
                // node 227
                rpre = (range>>SCALElog)*cty[227];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 455; upd(227,1); } else { range = rpre; sym = 454; upd(227,0); }
                RENORM
              } else {
                range = rpre; upd(113,0); RENORM
                // node 226
                rpre = (range>>SCALElog)*cty[226];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 453; upd(226,1); } else { range = rpre; sym = 452; upd(226,0); }
                RENORM
              }
            } else {
              range = rpre; upd(56,0); RENORM
              // node 112
              rpre = (range>>SCALElog)*cty[112];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(112,1); RENORM
                // node 225
                rpre = (range>>SCALElog)*cty[225];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 451; upd(225,1); } else { range = rpre; sym = 450; upd(225,0); }
                RENORM
              } else {
                range = rpre; upd(112,0); RENORM
                // node 224
                rpre = (range>>SCALElog)*cty[224];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 449; upd(224,1); } else { range = rpre; sym = 448; upd(224,0); }
                RENORM
              }
            }
          }
        }
      }
    } else {
      range = rpre; upd(3,0); RENORM
      // node 6
      rpre = (range>>SCALElog)*cty[6];
      if( code >= rpre ) {
        range -= rpre; code -= rpre; upd(6,1); RENORM
        // node 13
        rpre = (range>>SCALElog)*cty[13];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(13,1); RENORM
          // node 27
          rpre = (range>>SCALElog)*cty[27];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(27,1); RENORM
            // node 55
            rpre = (range>>SCALElog)*cty[55];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(55,1); RENORM
              // node 111
              rpre = (range>>SCALElog)*cty[111];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(111,1); RENORM
                // node 223
                rpre = (range>>SCALElog)*cty[223];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 447; upd(223,1); } else { range = rpre; sym = 446; upd(223,0); }
                RENORM
              } else {
                range = rpre; upd(111,0); RENORM
                // node 222
                rpre = (range>>SCALElog)*cty[222];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 445; upd(222,1); } else { range = rpre; sym = 444; upd(222,0); }
                RENORM
              }
            } else {
              range = rpre; upd(55,0); RENORM
              // node 110
              rpre = (range>>SCALElog)*cty[110];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(110,1); RENORM
                // node 221
                rpre = (range>>SCALElog)*cty[221];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 443; upd(221,1); } else { range = rpre; sym = 442; upd(221,0); }
                RENORM
              } else {
                range = rpre; upd(110,0); RENORM
                // node 220
                rpre = (range>>SCALElog)*cty[220];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 441; upd(220,1); } else { range = rpre; sym = 440; upd(220,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(27,0); RENORM
            // node 54
            rpre = (range>>SCALElog)*cty[54];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(54,1); RENORM
              // node 109
              rpre = (range>>SCALElog)*cty[109];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(109,1); RENORM
                // node 219
                rpre = (range>>SCALElog)*cty[219];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 439; upd(219,1); } else { range = rpre; sym = 438; upd(219,0); }
                RENORM
              } else {
                range = rpre; upd(109,0); RENORM
                // node 218
                rpre = (range>>SCALElog)*cty[218];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 437; upd(218,1); } else { range = rpre; sym = 436; upd(218,0); }
                RENORM
              }
            } else {
              range = rpre; upd(54,0); RENORM
              // node 108
              rpre = (range>>SCALElog)*cty[108];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(108,1); RENORM
                // node 217
                rpre = (range>>SCALElog)*cty[217];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 435; upd(217,1); } else { range = rpre; sym = 434; upd(217,0); }
                RENORM
              } else {
                range = rpre; upd(108,0); RENORM
                // node 216
                rpre = (range>>SCALElog)*cty[216];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 433; upd(216,1); } else { range = rpre; sym = 432; upd(216,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(13,0); RENORM
          // node 26
          rpre = (range>>SCALElog)*cty[26];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(26,1); RENORM
            // node 53
            rpre = (range>>SCALElog)*cty[53];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(53,1); RENORM
              // node 107
              rpre = (range>>SCALElog)*cty[107];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(107,1); RENORM
                // node 215
                rpre = (range>>SCALElog)*cty[215];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 431; upd(215,1); } else { range = rpre; sym = 430; upd(215,0); }
                RENORM
              } else {
                range = rpre; upd(107,0); RENORM
                // node 214
                rpre = (range>>SCALElog)*cty[214];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 429; upd(214,1); } else { range = rpre; sym = 428; upd(214,0); }
                RENORM
              }
            } else {
              range = rpre; upd(53,0); RENORM
              // node 106
              rpre = (range>>SCALElog)*cty[106];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(106,1); RENORM
                // node 213
                rpre = (range>>SCALElog)*cty[213];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 427; upd(213,1); } else { range = rpre; sym = 426; upd(213,0); }
                RENORM
              } else {
                range = rpre; upd(106,0); RENORM
                // node 212
                rpre = (range>>SCALElog)*cty[212];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 425; upd(212,1); } else { range = rpre; sym = 424; upd(212,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(26,0); RENORM
            // node 52
            rpre = (range>>SCALElog)*cty[52];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(52,1); RENORM
              // node 105
              rpre = (range>>SCALElog)*cty[105];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(105,1); RENORM
                // node 211
                rpre = (range>>SCALElog)*cty[211];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 423; upd(211,1); } else { range = rpre; sym = 422; upd(211,0); }
                RENORM
              } else {
                range = rpre; upd(105,0); RENORM
                // node 210
                rpre = (range>>SCALElog)*cty[210];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 421; upd(210,1); } else { range = rpre; sym = 420; upd(210,0); }
                RENORM
              }
            } else {
              range = rpre; upd(52,0); RENORM
              // node 104
              rpre = (range>>SCALElog)*cty[104];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(104,1); RENORM
                // node 209
                rpre = (range>>SCALElog)*cty[209];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 419; upd(209,1); } else { range = rpre; sym = 418; upd(209,0); }
                RENORM
              } else {
                range = rpre; upd(104,0); RENORM
                // node 208
                rpre = (range>>SCALElog)*cty[208];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 417; upd(208,1); } else { range = rpre; sym = 416; upd(208,0); }
                RENORM
              }
            }
          }
        }
      } else {
        range = rpre; upd(6,0); RENORM
        // node 12
        rpre = (range>>SCALElog)*cty[12];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(12,1); RENORM
          // node 25
          rpre = (range>>SCALElog)*cty[25];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(25,1); RENORM
            // node 51
            rpre = (range>>SCALElog)*cty[51];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(51,1); RENORM
              // node 103
              rpre = (range>>SCALElog)*cty[103];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(103,1); RENORM
                // node 207
                rpre = (range>>SCALElog)*cty[207];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 415; upd(207,1); } else { range = rpre; sym = 414; upd(207,0); }
                RENORM
              } else {
                range = rpre; upd(103,0); RENORM
                // node 206
                rpre = (range>>SCALElog)*cty[206];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 413; upd(206,1); } else { range = rpre; sym = 412; upd(206,0); }
                RENORM
              }
            } else {
              range = rpre; upd(51,0); RENORM
              // node 102
              rpre = (range>>SCALElog)*cty[102];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(102,1); RENORM
                // node 205
                rpre = (range>>SCALElog)*cty[205];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 411; upd(205,1); } else { range = rpre; sym = 410; upd(205,0); }
                RENORM
              } else {
                range = rpre; upd(102,0); RENORM
                // node 204
                rpre = (range>>SCALElog)*cty[204];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 409; upd(204,1); } else { range = rpre; sym = 408; upd(204,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(25,0); RENORM
            // node 50
            rpre = (range>>SCALElog)*cty[50];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(50,1); RENORM
              // node 101
              rpre = (range>>SCALElog)*cty[101];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(101,1); RENORM
                // node 203
                rpre = (range>>SCALElog)*cty[203];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 407; upd(203,1); } else { range = rpre; sym = 406; upd(203,0); }
                RENORM
              } else {
                range = rpre; upd(101,0); RENORM
                // node 202
                rpre = (range>>SCALElog)*cty[202];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 405; upd(202,1); } else { range = rpre; sym = 404; upd(202,0); }
                RENORM
              }
            } else {
              range = rpre; upd(50,0); RENORM
              // node 100
              rpre = (range>>SCALElog)*cty[100];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(100,1); RENORM
                // node 201
                rpre = (range>>SCALElog)*cty[201];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 403; upd(201,1); } else { range = rpre; sym = 402; upd(201,0); }
                RENORM
              } else {
                range = rpre; upd(100,0); RENORM
                // node 200
                rpre = (range>>SCALElog)*cty[200];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 401; upd(200,1); } else { range = rpre; sym = 400; upd(200,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(12,0); RENORM
          // node 24
          rpre = (range>>SCALElog)*cty[24];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(24,1); RENORM
            // node 49
            rpre = (range>>SCALElog)*cty[49];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(49,1); RENORM
              // node 99
              rpre = (range>>SCALElog)*cty[99];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(99,1); RENORM
                // node 199
                rpre = (range>>SCALElog)*cty[199];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 399; upd(199,1); } else { range = rpre; sym = 398; upd(199,0); }
                RENORM
              } else {
                range = rpre; upd(99,0); RENORM
                // node 198
                rpre = (range>>SCALElog)*cty[198];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 397; upd(198,1); } else { range = rpre; sym = 396; upd(198,0); }
                RENORM
              }
            } else {
              range = rpre; upd(49,0); RENORM
              // node 98
              rpre = (range>>SCALElog)*cty[98];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(98,1); RENORM
                // node 197
                rpre = (range>>SCALElog)*cty[197];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 395; upd(197,1); } else { range = rpre; sym = 394; upd(197,0); }
                RENORM
              } else {
                range = rpre; upd(98,0); RENORM
                // node 196
                rpre = (range>>SCALElog)*cty[196];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 393; upd(196,1); } else { range = rpre; sym = 392; upd(196,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(24,0); RENORM
            // node 48
            rpre = (range>>SCALElog)*cty[48];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(48,1); RENORM
              // node 97
              rpre = (range>>SCALElog)*cty[97];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(97,1); RENORM
                // node 195
                rpre = (range>>SCALElog)*cty[195];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 391; upd(195,1); } else { range = rpre; sym = 390; upd(195,0); }
                RENORM
              } else {
                range = rpre; upd(97,0); RENORM
                // node 194
                rpre = (range>>SCALElog)*cty[194];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 389; upd(194,1); } else { range = rpre; sym = 388; upd(194,0); }
                RENORM
              }
            } else {
              range = rpre; upd(48,0); RENORM
              // node 96
              rpre = (range>>SCALElog)*cty[96];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(96,1); RENORM
                // node 193
                rpre = (range>>SCALElog)*cty[193];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 387; upd(193,1); } else { range = rpre; sym = 386; upd(193,0); }
                RENORM
              } else {
                range = rpre; upd(96,0); RENORM
                // node 192
                rpre = (range>>SCALElog)*cty[192];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 385; upd(192,1); } else { range = rpre; sym = 384; upd(192,0); }
                RENORM
              }
            }
          }
        }
      }
    }
  } else {
    range = rpre; upd(1,0); RENORM
    // node 2
    rpre = (range>>SCALElog)*cty[2];
    if( code >= rpre ) {
      range -= rpre; code -= rpre; upd(2,1); RENORM
      // node 5
      rpre = (range>>SCALElog)*cty[5];
      if( code >= rpre ) {
        range -= rpre; code -= rpre; upd(5,1); RENORM
        // node 11
        rpre = (range>>SCALElog)*cty[11];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(11,1); RENORM
          // node 23
          rpre = (range>>SCALElog)*cty[23];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(23,1); RENORM
            // node 47
            rpre = (range>>SCALElog)*cty[47];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(47,1); RENORM
              // node 95
              rpre = (range>>SCALElog)*cty[95];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(95,1); RENORM
                // node 191
                rpre = (range>>SCALElog)*cty[191];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 383; upd(191,1); } else { range = rpre; sym = 382; upd(191,0); }
                RENORM
              } else {
                range = rpre; upd(95,0); RENORM
                // node 190
                rpre = (range>>SCALElog)*cty[190];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 381; upd(190,1); } else { range = rpre; sym = 380; upd(190,0); }
                RENORM
              }
            } else {
              range = rpre; upd(47,0); RENORM
              // node 94
              rpre = (range>>SCALElog)*cty[94];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(94,1); RENORM
                // node 189
                rpre = (range>>SCALElog)*cty[189];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 379; upd(189,1); } else { range = rpre; sym = 378; upd(189,0); }
                RENORM
              } else {
                range = rpre; upd(94,0); RENORM
                // node 188
                rpre = (range>>SCALElog)*cty[188];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 377; upd(188,1); } else { range = rpre; sym = 376; upd(188,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(23,0); RENORM
            // node 46
            rpre = (range>>SCALElog)*cty[46];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(46,1); RENORM
              // node 93
              rpre = (range>>SCALElog)*cty[93];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(93,1); RENORM
                // node 187
                rpre = (range>>SCALElog)*cty[187];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 375; upd(187,1); } else { range = rpre; sym = 374; upd(187,0); }
                RENORM
              } else {
                range = rpre; upd(93,0); RENORM
                // node 186
                rpre = (range>>SCALElog)*cty[186];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 373; upd(186,1); } else { range = rpre; sym = 372; upd(186,0); }
                RENORM
              }
            } else {
              range = rpre; upd(46,0); RENORM
              // node 92
              rpre = (range>>SCALElog)*cty[92];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(92,1); RENORM
                // node 185
                rpre = (range>>SCALElog)*cty[185];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 371; upd(185,1); } else { range = rpre; sym = 370; upd(185,0); }
                RENORM
              } else {
                range = rpre; upd(92,0); RENORM
                // node 184
                rpre = (range>>SCALElog)*cty[184];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 369; upd(184,1); } else { range = rpre; sym = 368; upd(184,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(11,0); RENORM
          // node 22
          rpre = (range>>SCALElog)*cty[22];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(22,1); RENORM
            // node 45
            rpre = (range>>SCALElog)*cty[45];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(45,1); RENORM
              // node 91
              rpre = (range>>SCALElog)*cty[91];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(91,1); RENORM
                // node 183
                rpre = (range>>SCALElog)*cty[183];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 367; upd(183,1); } else { range = rpre; sym = 366; upd(183,0); }
                RENORM
              } else {
                range = rpre; upd(91,0); RENORM
                // node 182
                rpre = (range>>SCALElog)*cty[182];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 365; upd(182,1); } else { range = rpre; sym = 364; upd(182,0); }
                RENORM
              }
            } else {
              range = rpre; upd(45,0); RENORM
              // node 90
              rpre = (range>>SCALElog)*cty[90];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(90,1); RENORM
                // node 181
                rpre = (range>>SCALElog)*cty[181];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 363; upd(181,1); } else { range = rpre; sym = 362; upd(181,0); }
                RENORM
              } else {
                range = rpre; upd(90,0); RENORM
                // node 180
                rpre = (range>>SCALElog)*cty[180];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 361; upd(180,1); } else { range = rpre; sym = 360; upd(180,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(22,0); RENORM
            // node 44
            rpre = (range>>SCALElog)*cty[44];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(44,1); RENORM
              // node 89
              rpre = (range>>SCALElog)*cty[89];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(89,1); RENORM
                // node 179
                rpre = (range>>SCALElog)*cty[179];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 359; upd(179,1); } else { range = rpre; sym = 358; upd(179,0); }
                RENORM
              } else {
                range = rpre; upd(89,0); RENORM
                // node 178
                rpre = (range>>SCALElog)*cty[178];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 357; upd(178,1); } else { range = rpre; sym = 356; upd(178,0); }
                RENORM
              }
            } else {
              range = rpre; upd(44,0); RENORM
              // node 88
              rpre = (range>>SCALElog)*cty[88];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(88,1); RENORM
                // node 177
                rpre = (range>>SCALElog)*cty[177];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 355; upd(177,1); } else { range = rpre; sym = 354; upd(177,0); }
                RENORM
              } else {
                range = rpre; upd(88,0); RENORM
                // node 176
                rpre = (range>>SCALElog)*cty[176];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 353; upd(176,1); } else { range = rpre; sym = 352; upd(176,0); }
                RENORM
              }
            }
          }
        }
      } else {
        range = rpre; upd(5,0); RENORM
        // node 10
        rpre = (range>>SCALElog)*cty[10];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(10,1); RENORM
          // node 21
          rpre = (range>>SCALElog)*cty[21];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(21,1); RENORM
            // node 43
            rpre = (range>>SCALElog)*cty[43];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(43,1); RENORM
              // node 87
              rpre = (range>>SCALElog)*cty[87];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(87,1); RENORM
                // node 175
                rpre = (range>>SCALElog)*cty[175];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 351; upd(175,1); } else { range = rpre; sym = 350; upd(175,0); }
                RENORM
              } else {
                range = rpre; upd(87,0); RENORM
                // node 174
                rpre = (range>>SCALElog)*cty[174];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 349; upd(174,1); } else { range = rpre; sym = 348; upd(174,0); }
                RENORM
              }
            } else {
              range = rpre; upd(43,0); RENORM
              // node 86
              rpre = (range>>SCALElog)*cty[86];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(86,1); RENORM
                // node 173
                rpre = (range>>SCALElog)*cty[173];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 347; upd(173,1); } else { range = rpre; sym = 346; upd(173,0); }
                RENORM
              } else {
                range = rpre; upd(86,0); RENORM
                // node 172
                rpre = (range>>SCALElog)*cty[172];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 345; upd(172,1); } else { range = rpre; sym = 344; upd(172,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(21,0); RENORM
            // node 42
            rpre = (range>>SCALElog)*cty[42];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(42,1); RENORM
              // node 85
              rpre = (range>>SCALElog)*cty[85];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(85,1); RENORM
                // node 171
                rpre = (range>>SCALElog)*cty[171];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 343; upd(171,1); } else { range = rpre; sym = 342; upd(171,0); }
                RENORM
              } else {
                range = rpre; upd(85,0); RENORM
                // node 170
                rpre = (range>>SCALElog)*cty[170];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 341; upd(170,1); } else { range = rpre; sym = 340; upd(170,0); }
                RENORM
              }
            } else {
              range = rpre; upd(42,0); RENORM
              // node 84
              rpre = (range>>SCALElog)*cty[84];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(84,1); RENORM
                // node 169
                rpre = (range>>SCALElog)*cty[169];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 339; upd(169,1); } else { range = rpre; sym = 338; upd(169,0); }
                RENORM
              } else {
                range = rpre; upd(84,0); RENORM
                // node 168
                rpre = (range>>SCALElog)*cty[168];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 337; upd(168,1); } else { range = rpre; sym = 336; upd(168,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(10,0); RENORM
          // node 20
          rpre = (range>>SCALElog)*cty[20];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(20,1); RENORM
            // node 41
            rpre = (range>>SCALElog)*cty[41];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(41,1); RENORM
              // node 83
              rpre = (range>>SCALElog)*cty[83];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(83,1); RENORM
                // node 167
                rpre = (range>>SCALElog)*cty[167];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 335; upd(167,1); } else { range = rpre; sym = 334; upd(167,0); }
                RENORM
              } else {
                range = rpre; upd(83,0); RENORM
                // node 166
                rpre = (range>>SCALElog)*cty[166];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 333; upd(166,1); } else { range = rpre; sym = 332; upd(166,0); }
                RENORM
              }
            } else {
              range = rpre; upd(41,0); RENORM
              // node 82
              rpre = (range>>SCALElog)*cty[82];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(82,1); RENORM
                // node 165
                rpre = (range>>SCALElog)*cty[165];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 331; upd(165,1); } else { range = rpre; sym = 330; upd(165,0); }
                RENORM
              } else {
                range = rpre; upd(82,0); RENORM
                // node 164
                rpre = (range>>SCALElog)*cty[164];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 329; upd(164,1); } else { range = rpre; sym = 328; upd(164,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(20,0); RENORM
            // node 40
            rpre = (range>>SCALElog)*cty[40];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(40,1); RENORM
              // node 81
              rpre = (range>>SCALElog)*cty[81];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(81,1); RENORM
                // node 163
                rpre = (range>>SCALElog)*cty[163];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 327; upd(163,1); } else { range = rpre; sym = 326; upd(163,0); }
                RENORM
              } else {
                range = rpre; upd(81,0); RENORM
                // node 162
                rpre = (range>>SCALElog)*cty[162];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 325; upd(162,1); } else { range = rpre; sym = 324; upd(162,0); }
                RENORM
              }
            } else {
              range = rpre; upd(40,0); RENORM
              // node 80
              rpre = (range>>SCALElog)*cty[80];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(80,1); RENORM
                // node 161
                rpre = (range>>SCALElog)*cty[161];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 323; upd(161,1); } else { range = rpre; sym = 322; upd(161,0); }
                RENORM
              } else {
                range = rpre; upd(80,0); RENORM
                // node 160
                rpre = (range>>SCALElog)*cty[160];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 321; upd(160,1); } else { range = rpre; sym = 320; upd(160,0); }
                RENORM
              }
            }
          }
        }
      }
    } else {
      range = rpre; upd(2,0); RENORM
      // node 4
      rpre = (range>>SCALElog)*cty[4];
      if( code >= rpre ) {
        range -= rpre; code -= rpre; upd(4,1); RENORM
        // node 9
        rpre = (range>>SCALElog)*cty[9];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(9,1); RENORM
          // node 19
          rpre = (range>>SCALElog)*cty[19];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(19,1); RENORM
            // node 39
            rpre = (range>>SCALElog)*cty[39];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(39,1); RENORM
              // node 79
              rpre = (range>>SCALElog)*cty[79];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(79,1); RENORM
                // node 159
                rpre = (range>>SCALElog)*cty[159];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 319; upd(159,1); } else { range = rpre; sym = 318; upd(159,0); }
                RENORM
              } else {
                range = rpre; upd(79,0); RENORM
                // node 158
                rpre = (range>>SCALElog)*cty[158];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 317; upd(158,1); } else { range = rpre; sym = 316; upd(158,0); }
                RENORM
              }
            } else {
              range = rpre; upd(39,0); RENORM
              // node 78
              rpre = (range>>SCALElog)*cty[78];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(78,1); RENORM
                // node 157
                rpre = (range>>SCALElog)*cty[157];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 315; upd(157,1); } else { range = rpre; sym = 314; upd(157,0); }
                RENORM
              } else {
                range = rpre; upd(78,0); RENORM
                // node 156
                rpre = (range>>SCALElog)*cty[156];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 313; upd(156,1); } else { range = rpre; sym = 312; upd(156,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(19,0); RENORM
            // node 38
            rpre = (range>>SCALElog)*cty[38];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(38,1); RENORM
              // node 77
              rpre = (range>>SCALElog)*cty[77];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(77,1); RENORM
                // node 155
                rpre = (range>>SCALElog)*cty[155];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 311; upd(155,1); } else { range = rpre; sym = 310; upd(155,0); }
                RENORM
              } else {
                range = rpre; upd(77,0); RENORM
                // node 154
                rpre = (range>>SCALElog)*cty[154];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 309; upd(154,1); } else { range = rpre; sym = 308; upd(154,0); }
                RENORM
              }
            } else {
              range = rpre; upd(38,0); RENORM
              // node 76
              rpre = (range>>SCALElog)*cty[76];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(76,1); RENORM
                // node 153
                rpre = (range>>SCALElog)*cty[153];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 307; upd(153,1); } else { range = rpre; sym = 306; upd(153,0); }
                RENORM
              } else {
                range = rpre; upd(76,0); RENORM
                // node 152
                rpre = (range>>SCALElog)*cty[152];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 305; upd(152,1); } else { range = rpre; sym = 304; upd(152,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(9,0); RENORM
          // node 18
          rpre = (range>>SCALElog)*cty[18];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(18,1); RENORM
            // node 37
            rpre = (range>>SCALElog)*cty[37];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(37,1); RENORM
              // node 75
              rpre = (range>>SCALElog)*cty[75];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(75,1); RENORM
                // node 151
                rpre = (range>>SCALElog)*cty[151];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 303; upd(151,1); } else { range = rpre; sym = 302; upd(151,0); }
                RENORM
              } else {
                range = rpre; upd(75,0); RENORM
                // node 150
                rpre = (range>>SCALElog)*cty[150];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 301; upd(150,1); } else { range = rpre; sym = 300; upd(150,0); }
                RENORM
              }
            } else {
              range = rpre; upd(37,0); RENORM
              // node 74
              rpre = (range>>SCALElog)*cty[74];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(74,1); RENORM
                // node 149
                rpre = (range>>SCALElog)*cty[149];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 299; upd(149,1); } else { range = rpre; sym = 298; upd(149,0); }
                RENORM
              } else {
                range = rpre; upd(74,0); RENORM
                // node 148
                rpre = (range>>SCALElog)*cty[148];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 297; upd(148,1); } else { range = rpre; sym = 296; upd(148,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(18,0); RENORM
            // node 36
            rpre = (range>>SCALElog)*cty[36];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(36,1); RENORM
              // node 73
              rpre = (range>>SCALElog)*cty[73];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(73,1); RENORM
                // node 147
                rpre = (range>>SCALElog)*cty[147];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 295; upd(147,1); } else { range = rpre; sym = 294; upd(147,0); }
                RENORM
              } else {
                range = rpre; upd(73,0); RENORM
                // node 146
                rpre = (range>>SCALElog)*cty[146];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 293; upd(146,1); } else { range = rpre; sym = 292; upd(146,0); }
                RENORM
              }
            } else {
              range = rpre; upd(36,0); RENORM
              // node 72
              rpre = (range>>SCALElog)*cty[72];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(72,1); RENORM
                // node 145
                rpre = (range>>SCALElog)*cty[145];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 291; upd(145,1); } else { range = rpre; sym = 290; upd(145,0); }
                RENORM
              } else {
                range = rpre; upd(72,0); RENORM
                // node 144
                rpre = (range>>SCALElog)*cty[144];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 289; upd(144,1); } else { range = rpre; sym = 288; upd(144,0); }
                RENORM
              }
            }
          }
        }
      } else {
        range = rpre; upd(4,0); RENORM
        // node 8
        rpre = (range>>SCALElog)*cty[8];
        if( code >= rpre ) {
          range -= rpre; code -= rpre; upd(8,1); RENORM
          // node 17
          rpre = (range>>SCALElog)*cty[17];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(17,1); RENORM
            // node 35
            rpre = (range>>SCALElog)*cty[35];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(35,1); RENORM
              // node 71
              rpre = (range>>SCALElog)*cty[71];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(71,1); RENORM
                // node 143
                rpre = (range>>SCALElog)*cty[143];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 287; upd(143,1); } else { range = rpre; sym = 286; upd(143,0); }
                RENORM
              } else {
                range = rpre; upd(71,0); RENORM
                // node 142
                rpre = (range>>SCALElog)*cty[142];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 285; upd(142,1); } else { range = rpre; sym = 284; upd(142,0); }
                RENORM
              }
            } else {
              range = rpre; upd(35,0); RENORM
              // node 70
              rpre = (range>>SCALElog)*cty[70];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(70,1); RENORM
                // node 141
                rpre = (range>>SCALElog)*cty[141];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 283; upd(141,1); } else { range = rpre; sym = 282; upd(141,0); }
                RENORM
              } else {
                range = rpre; upd(70,0); RENORM
                // node 140
                rpre = (range>>SCALElog)*cty[140];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 281; upd(140,1); } else { range = rpre; sym = 280; upd(140,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(17,0); RENORM
            // node 34
            rpre = (range>>SCALElog)*cty[34];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(34,1); RENORM
              // node 69
              rpre = (range>>SCALElog)*cty[69];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(69,1); RENORM
                // node 139
                rpre = (range>>SCALElog)*cty[139];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 279; upd(139,1); } else { range = rpre; sym = 278; upd(139,0); }
                RENORM
              } else {
                range = rpre; upd(69,0); RENORM
                // node 138
                rpre = (range>>SCALElog)*cty[138];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 277; upd(138,1); } else { range = rpre; sym = 276; upd(138,0); }
                RENORM
              }
            } else {
              range = rpre; upd(34,0); RENORM
              // node 68
              rpre = (range>>SCALElog)*cty[68];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(68,1); RENORM
                // node 137
                rpre = (range>>SCALElog)*cty[137];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 275; upd(137,1); } else { range = rpre; sym = 274; upd(137,0); }
                RENORM
              } else {
                range = rpre; upd(68,0); RENORM
                // node 136
                rpre = (range>>SCALElog)*cty[136];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 273; upd(136,1); } else { range = rpre; sym = 272; upd(136,0); }
                RENORM
              }
            }
          }
        } else {
          range = rpre; upd(8,0); RENORM
          // node 16
          rpre = (range>>SCALElog)*cty[16];
          if( code >= rpre ) {
            range -= rpre; code -= rpre; upd(16,1); RENORM
            // node 33
            rpre = (range>>SCALElog)*cty[33];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(33,1); RENORM
              // node 67
              rpre = (range>>SCALElog)*cty[67];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(67,1); RENORM
                // node 135
                rpre = (range>>SCALElog)*cty[135];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 271; upd(135,1); } else { range = rpre; sym = 270; upd(135,0); }
                RENORM
              } else {
                range = rpre; upd(67,0); RENORM
                // node 134
                rpre = (range>>SCALElog)*cty[134];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 269; upd(134,1); } else { range = rpre; sym = 268; upd(134,0); }
                RENORM
              }
            } else {
              range = rpre; upd(33,0); RENORM
              // node 66
              rpre = (range>>SCALElog)*cty[66];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(66,1); RENORM
                // node 133
                rpre = (range>>SCALElog)*cty[133];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 267; upd(133,1); } else { range = rpre; sym = 266; upd(133,0); }
                RENORM
              } else {
                range = rpre; upd(66,0); RENORM
                // node 132
                rpre = (range>>SCALElog)*cty[132];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 265; upd(132,1); } else { range = rpre; sym = 264; upd(132,0); }
                RENORM
              }
            }
          } else {
            range = rpre; upd(16,0); RENORM
            // node 32
            rpre = (range>>SCALElog)*cty[32];
            if( code >= rpre ) {
              range -= rpre; code -= rpre; upd(32,1); RENORM
              // node 65
              rpre = (range>>SCALElog)*cty[65];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(65,1); RENORM
                // node 131
                rpre = (range>>SCALElog)*cty[131];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 263; upd(131,1); } else { range = rpre; sym = 262; upd(131,0); }
                RENORM
              } else {
                range = rpre; upd(65,0); RENORM
                // node 130
                rpre = (range>>SCALElog)*cty[130];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 261; upd(130,1); } else { range = rpre; sym = 260; upd(130,0); }
                RENORM
              }
            } else {
              range = rpre; upd(32,0); RENORM
              // node 64
              rpre = (range>>SCALElog)*cty[64];
              if( code >= rpre ) {
                range -= rpre; code -= rpre; upd(64,1); RENORM
                // node 129
                rpre = (range>>SCALElog)*cty[129];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 259; upd(129,1); } else { range = rpre; sym = 258; upd(129,0); }
                RENORM
              } else {
                range = rpre; upd(64,0); RENORM
                // node 128
                rpre = (range>>SCALElog)*cty[128];
                if( code >= rpre ) { range -= rpre; code -= rpre; sym = 257; upd(128,1); } else { range = rpre; sym = 256; upd(128,0); }
                RENORM
              }
            }
          }
        }
      }
    }
  }
