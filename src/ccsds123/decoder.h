/*
Luca Fossati (Luca.Fossati@esa.int), European Space Agency

Software distributed under the "European Space Agency Public License – v2.0".

All Distribution of the Software and/or Modifications, as Source Code or Object Code,
must be, as a whole, under the terms of the European Space Agency Public License – v2.0.
If You Distribute the Software and/or Modifications as Object Code, You must:
(a)	provide in addition a copy of the Source Code of the Software and/or
Modifications to each recipient; or
(b)	make the Source Code of the Software and/or Modifications freely accessible by reasonable
means for anyone who possesses the Object Code or received the Software and/or Modifications
from You, and inform recipients how to obtain a copy of the Source Code.

The Software is provided to You on an “as is” basis and without warranties of any
kind, including without limitation merchantability, fitness for a particular purpose,
absence of defects or errors, accuracy or non-infringement of intellectual property
rights.
Except as expressly set forth in the "European Space Agency Public License – v2.0",
neither Licensor nor any Contributor shall be liable, including, without limitation, for direct, indirect,
incidental, or consequential damages (including without limitation loss of profit),
however caused and on any theory of liability, arising in any way out of the use or
Distribution of the Software or the exercise of any rights under this License, even
if You have been advised of the possibility of such damages.

*****************************************************************************************

XXXXXXXXXXXXXXXXXXXXXXX
*/

#ifndef DECODER_H
#define DECODER_H

#include <stdio.h>

#include "utils.h"
#include "predictor.h"
// MODIFIED: encoder_t / encoder_config_t used to be declared a second time here,
// identically to entropy_encoder.h.  The two headers could therefore never be
// included by the same translation unit, which a combined coder has to do.
#include "entropy_encoder.h"

/// Reads a compressed sample when compressed using the sample adaptive encoding method.
int read_element_sample(FILE * compressedStream, encoder_config_t encoder_params, input_feature_t input_params,
                        unsigned int temp_k, unsigned char *buffer, unsigned int *buffer_len);
/// Main routine for decoding the input stream compressed according to the sample adaptive
/// method: it iterates over the various compressed samples, calling read_element_sample to extract
/// each of them from the compressed stream
int decode_sample_adaptive(FILE * compressedStream, input_feature_t input_params, encoder_config_t encoder_params,
                           unsigned short int * residuals);

/// Reads a compressed block when using the block adaptive encoding method.
int read_nocomp_block(input_feature_t input_params, encoder_config_t encoder_params, FILE * compressedStream,
                      unsigned short int * residuals, unsigned int * read_elems, unsigned char *buffer,
                      unsigned int *buffer_len, unsigned int block_size);
int read_second_block(input_feature_t input_params, encoder_config_t encoder_params, FILE * compressedStream,
                      unsigned short int * residuals, unsigned int * read_elems, unsigned char *buffer,
                      unsigned int *buffer_len, unsigned int block_size);
int read_ksplit_block(input_feature_t input_params, encoder_config_t encoder_params, FILE * compressedStream,
                      unsigned int k, unsigned short int * residuals, unsigned int * read_elems,
                      unsigned char *buffer, unsigned int *buffer_len, unsigned int block_size);
int read_zero_block(input_feature_t input_params, encoder_config_t encoder_params, FILE * compressedStream,
                    unsigned short int * residuals, unsigned int * read_elems, unsigned char *buffer, unsigned int *buffer_len);
/// Main routine for decoding the input stream compressed according to the block adaptive
/// method: it determines the compression method for the block and the calls the appropriate
/// routine for its decoding.
int decode_block_adaptive(FILE * compressedStream, input_feature_t input_params,
                          encoder_config_t encoder_params, unsigned short int * residuals);

/// Reads the compressed file header, filling-in the appropriate data structures
int read_header(FILE * compressedStream, input_feature_t *input_params, encoder_config_t *encoder_params,
                predictor_config_t *predictor_params);

/// Main decoder function, from the file containing the compressed stream it produces the
/// file containins the mapped residuals, stored in BSQ format.
int decode(input_feature_t *input_params, predictor_config_t *predictor_params, 
    unsigned short int ** residuals, char inputFile[128]);

/// MODIFIED: the largest cube this decoder will allocate for.  The dimensions in the
/// header are three independent 16 bit fields, so their product does not fit in the
/// 32 bit arithmetic the codec indexes with, let alone in memory.
#define CCSDS_MAX_SAMPLES ((unsigned int)1 << 28)

/// MODIFIED: as decode(), but reading from an already-open stream positioned at the
/// first byte of the CCSDS header.  Lets a container carry its own metadata ahead of
/// the compressed image.  The stream is left positioned after the compressed data and
/// is not closed.
/// expected_samples, when not zero, is the sample count the caller already knows the
/// image to have: the header is checked against it before anything is allocated.
int decode_from_stream(FILE * compressedStream, input_feature_t *input_params,
    predictor_config_t *predictor_params, unsigned short int ** residuals,
    unsigned int expected_samples);

#endif