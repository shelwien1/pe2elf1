@echo off

set path=C:\mingwG20x\bin

del _obj\cabac_decoder.o 

set O1=-O3 -DNDEBUG -DWIN32 -D_WIN32
set I1=-I./codec/

if not exist _obj/au_parser.o  g++ -c %O1% %I1% codec/au_parser.cpp -o_obj/au_parser.o
if not exist _obj/bit_stream.o  g++ -c %O1% %I1% codec/bit_stream.cpp -o_obj/bit_stream.o
if not exist _obj/cabac_decoder.o  g++ -c %O1% %I1% codec/cabac_decoder.cpp -o_obj/cabac_decoder.o
if not exist _obj/common_tables.o  g++ -c %O1% %I1% codec/common_tables.cpp -o_obj/common_tables.o
if not exist _obj/copy_mb.o  g++ -c %O1% %I1% codec/copy_mb.cpp -o_obj/copy_mb.o
if not exist _obj/cpu.o  g++ -c %O1% %I1% codec/cpu.cpp -o_obj/cpu.o
if not exist _obj/crt_util_safe_x.o  g++ -c %O1% %I1% codec/crt_util_safe_x.cpp -o_obj/crt_util_safe_x.o
if not exist _obj/deblocking.o  g++ -c %O1% %I1% codec/deblocking.cpp -o_obj/deblocking.o
if not exist _obj/deblocking_common.o  g++ -c %O1% %I1% codec/deblocking_common.cpp -o_obj/deblocking_common.o
if not exist _obj/decoder.o  g++ -c %O1% %I1% codec/decoder.cpp -o_obj/decoder.o
if not exist _obj/decoder_core.o  g++ -c %O1% %I1% codec/decoder_core.cpp -o_obj/decoder_core.o
if not exist _obj/decoder_data_tables.o  g++ -c %O1% %I1% codec/decoder_data_tables.cpp -o_obj/decoder_data_tables.o
if not exist _obj/decode_mb_aux.o  g++ -c %O1% %I1% codec/decode_mb_aux.cpp -o_obj/decode_mb_aux.o
if not exist _obj/decode_slice.o  g++ -c %O1% %I1% codec/decode_slice.cpp -o_obj/decode_slice.o
if not exist _obj/error_concealment.o  g++ -c %O1% %I1% codec/error_concealment.cpp -o_obj/error_concealment.o
if not exist _obj/expand_pic.o  g++ -c %O1% %I1% codec/expand_pic.cpp -o_obj/expand_pic.o
if not exist _obj/fmo.o  g++ -c %O1% %I1% codec/fmo.cpp -o_obj/fmo.o
if not exist _obj/get_intra_predictor.o  g++ -c %O1% %I1% codec/get_intra_predictor.cpp -o_obj/get_intra_predictor.o
if not exist _obj/h264dec.o  g++ -c %O1% %I1% codec/h264dec.cpp -o_obj/h264dec.o
if not exist _obj/intra_pred_common.o  g++ -c %O1% %I1% codec/intra_pred_common.cpp -o_obj/intra_pred_common.o
if not exist _obj/manage_dec_ref.o  g++ -c %O1% %I1% codec/manage_dec_ref.cpp -o_obj/manage_dec_ref.o
if not exist _obj/mc.o  g++ -c %O1% %I1% codec/mc.cpp -o_obj/mc.o
if not exist _obj/memmgr_nal_unit.o  g++ -c %O1% %I1% codec/memmgr_nal_unit.cpp -o_obj/memmgr_nal_unit.o
if not exist _obj/memory_align.o  g++ -c %O1% %I1% codec/memory_align.cpp -o_obj/memory_align.o
if not exist _obj/mv_pred.o  g++ -c %O1% %I1% codec/mv_pred.cpp -o_obj/mv_pred.o
if not exist _obj/parse_mb_syn_cabac.o  g++ -c %O1% %I1% codec/parse_mb_syn_cabac.cpp -o_obj/parse_mb_syn_cabac.o
if not exist _obj/parse_mb_syn_cavlc.o  g++ -c %O1% %I1% codec/parse_mb_syn_cavlc.cpp -o_obj/parse_mb_syn_cavlc.o
if not exist _obj/pic_queue.o  g++ -c %O1% %I1% codec/pic_queue.cpp -o_obj/pic_queue.o
if not exist _obj/read_config.o  g++ -c %O1% %I1% codec/read_config.cpp -o_obj/read_config.o
if not exist _obj/rec_mb.o  g++ -c %O1% %I1% codec/rec_mb.cpp -o_obj/rec_mb.o
if not exist _obj/sad_common.o  g++ -c %O1% %I1% codec/sad_common.cpp -o_obj/sad_common.o
if not exist _obj/utils.o  g++ -c %O1% %I1% codec/utils.cpp -o_obj/utils.o
if not exist _obj/welsCodecTrace.o  g++ -c %O1% %I1% codec/welsCodecTrace.cpp -o_obj/welsCodecTrace.o
if not exist _obj/welsDecoderExt.o  g++ -c %O1% %I1% codec/welsDecoderExt.cpp -o_obj/welsDecoderExt.o
if not exist _obj/WelsTaskThread.o  g++ -c %O1% %I1% codec/WelsTaskThread.cpp -o_obj/WelsTaskThread.o
if not exist _obj/WelsThread.o  g++ -c %O1% %I1% codec/WelsThread.cpp -o_obj/WelsThread.o
if not exist _obj/WelsThreadLib.o  g++ -c %O1% %I1% codec/WelsThreadLib.cpp -o_obj/WelsThreadLib.o
if not exist _obj/WelsThreadPool.o  g++ -c %O1% %I1% codec/WelsThreadPool.cpp -o_obj/WelsThreadPool.o
if not exist _obj/wels_decoder_thread.o  g++ -c %O1% %I1% codec/wels_decoder_thread.cpp -o_obj/wels_decoder_thread.o
if not exist _obj/d3d9_utils.o  g++ -c %O1% %I1% codec/d3d9_utils.cpp -o_obj/d3d9_utils.o

g++ -s -static @list2o -o h264dec.exe 
