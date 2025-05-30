/*
 * pulp_nn_matmul_u8_i8.c
 * Nazareno Bruschi <nazareno.bruschi@unibo.it>
 *
 * Copyright (C) 2019-2020 University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pmsis.h"
#include "pulp_nn_utils.h"


uint8_t *pulp_nn_matmul_u8_i8_2x2 (
                        uint8_t *pIn,
                        int8_t *pBias,
                        uint8_t *pOut,
                        uint8_t *pOut2,
                        int8_t *pWeight,
                        uint16_t out_shift,
                        uint16_t num_col_im2col,
                        uint16_t ch_out
){
  v4s vecA;
  v4s vecA2;
  v4u vecB;
  v4u vecB2;

  uint16_t ch_out_r = ch_out;
  uint16_t num_col_im2col_w = num_col_im2col;

  //uint8_t *pOut2 = pOut + ch_out_r;
  int8_t *pA = pWeight;

  uint16_t chan_left = ch_out & 0x3;

  for(int i=0; i < (ch_out >> 1); i++)
  {
    uint8_t *pB =  pIn;
    uint8_t *pB2 = (pB + num_col_im2col);
    int8_t *pA2 = (pA + num_col_im2col_w);

    int sum = 0;
    int sum2 = 0;
    int sum5 = 0;
    int sum6 = 0;

    if (pBias != NULL)
    {
      sum = ((int) (*pBias++));
      sum2 = ((int) (*pBias++)); 

      sum5 = sum;
      sum6 = sum2;
    }

    for(int j=0; j<(num_col_im2col_w >> 2); j++)
    {
      vecA = *((v4s*)pA);
      vecA2 = *((v4s*)pA2);

      vecB = *((v4u*)pB);
      vecB2 = *((v4u*)pB2);

      sum = SumDotp4(vecB, vecA, sum );
      sum2 = SumDotp4(vecB, vecA2, sum2);

      sum5 = SumDotp4(vecB2, vecA, sum5);
      sum6 = SumDotp4(vecB2, vecA2, sum6);

      pA+=4;
      pA2+=4;

      pB+=4;
      pB2+=4;
    }
    uint16_t col_cnt_im2col = num_col_im2col & 0x3;
    while (col_cnt_im2col)
    {
      int8_t inA = *pA++;
      int8_t inA2 = *pA2++;
      uint8_t inB = *pB++;
      uint8_t inB2 = *pB2++;
      asm volatile("": : :"memory");
      sum += inA * inB;
      sum2 += inA2 * inB;
      sum5 += inA * inB2;
      sum6 += inA2 * inB2;

      col_cnt_im2col--;
    }

    *pOut = (uint8_t) clip8(sum >> out_shift);
    pOut++;
    *pOut = (uint8_t) clip8(sum2 >> out_shift);
    pOut++;

    *pOut2 = (uint8_t) clip8(sum5 >> out_shift);
    pOut2++;
    *pOut2 = (uint8_t) clip8(sum6 >> out_shift);
    pOut2++;

    pA+=( num_col_im2col_w);
  }
  
  while(chan_left)
  {
    uint8_t *pB = pIn;
    uint8_t *pB2 = (pB + num_col_im2col);
    int sum = 0;
    if (pBias != NULL)
      sum = ((int) (*pBias++));    
    int sum2 = sum;

    for(int j=0; j < (num_col_im2col_w >> 2); j++)
    {
      vecA = *((v4s*) pA);
      vecB = *((v4u*) pB);
      vecB2 = *((v4u*) pB2);

      sum = SumDotp4(vecB, vecA, sum);
      sum2 = SumDotp4(vecB2, vecA, sum2);

      pA+=4;
      pB+=4;
      pB2+=4;
    }
    uint16_t col_cnt_im2col = num_col_im2col & 0x3;
    while(col_cnt_im2col)
    {
      int8_t inA = *pA++;
      uint8_t inB = *pB++;
      uint8_t inB2 = *pB2++;
      asm volatile("": : :"memory");
      sum += inA * inB;
      sum2 += inA * inB2;

      col_cnt_im2col--;
    }

    *pOut = (uint8_t) clip8(sum >> out_shift);
    pOut++;
    *pOut2 = (uint8_t) clip8(sum2 >> out_shift);
    pOut2++;

    chan_left--;
  }
  pOut+=ch_out_r;
  return pOut;
}
