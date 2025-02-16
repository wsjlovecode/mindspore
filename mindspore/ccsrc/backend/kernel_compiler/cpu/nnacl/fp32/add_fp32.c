/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nnacl/fp32/add_fp32.h"
#include "nnacl/fp32/arithmetic_fp32.h"

int ElementOptAdd(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
#ifdef ENABLE_AVX
  MS_FLOAT32X8 vin0_opt_8 = MS_MOV256_F32(in0[0]);
  MS_FLOAT32X8 vin1_opt_8 = MS_MOV256_F32(in1[0]);
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_FLOAT32X4 vin0_opt = MS_MOVQ_F32(in0[0]);
  MS_FLOAT32X4 vin1_opt = MS_MOVQ_F32(in1[0]);
#endif
  int index = 0;
  if (param->in_elements_num0_ == 1) {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
      MS_FLOAT32X8 vout = MS_ADD256_F32(vin0_opt_8, vin1);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
      MS_FLOAT32X4 vout = MS_ADDQ_F32(vin0_opt, vin1);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = in0[0] + in1[index];
    }
  } else {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
      MS_FLOAT32X8 vout = MS_ADD256_F32(vin0, vin1_opt_8);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
      MS_FLOAT32X4 vout = MS_ADDQ_F32(vin0, vin1_opt);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = in0[index] + in1[0];
    }
  }
  return NNACL_OK;
}

int ElementOptAddInt(const int *in0, const int *in1, int *out, int size, const ArithmeticParameter *param) {
#ifdef ENABLE_AVX
  MS_INT32X8 vin0_opt_8 = MS_MOV256_EPI32(in0[0]);
  MS_INT32X8 vin1_opt_8 = MS_MOV256_EPI32(in1[0]);
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_INT32X4 vin0_opt = MS_MOVQ_EPI32(in0[0]);
  MS_INT32X4 vin1_opt = MS_MOVQ_EPI32(in1[0]);
#endif
  int index = 0;
  if (param->in_elements_num0_ == 1) {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_INT32X8 vin1 = MS_LD256_EPI32(in1 + index);
      MS_INT32X8 vout = MS_ADD256_EPI32(vin0_opt_8, vin1);
      MS_ST256_EPI32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_INT32X4 vin1 = MS_LDQ_EPI32(in1 + index);
      MS_INT32X4 vout = MS_ADDQ_EPI32(vin0_opt, vin1);
      MS_STQ_EPI32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = in0[0] + in1[index];
    }
  } else {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_INT32X8 vin0 = MS_LD256_EPI32(in0 + index);
      MS_INT32X8 vout = MS_ADD256_EPI32(vin0, vin1_opt_8);
      MS_ST256_EPI32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_INT32X4 vin0 = MS_LDQ_EPI32(in0 + index);
      MS_INT32X4 vout = MS_ADDQ_EPI32(vin0, vin1_opt);
      MS_STQ_EPI32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = in0[index] + in1[0];
    }
  }
  return NNACL_OK;
}

int ElementOptAddRelu(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
#ifdef ENABLE_AVX
  MS_FLOAT32X8 vin0_opt_8 = MS_MOV256_F32(in0[0]);
  MS_FLOAT32X8 vin1_opt_8 = MS_MOV256_F32(in1[0]);
  MS_FLOAT32X8 zeros_8 = MS_MOV256_F32(0.0f);
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_FLOAT32X4 vin0_opt = MS_MOVQ_F32(in0[0]);
  MS_FLOAT32X4 vin1_opt = MS_MOVQ_F32(in1[0]);
  MS_FLOAT32X4 zeros = MS_MOVQ_F32(0.0f);
#endif
  int index = 0;
  if (param->in_elements_num0_ == 1) {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
      MS_FLOAT32X8 vout = MS_MAX256_F32(MS_ADD256_F32(vin0_opt_8, vin1), zeros_8);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
      MS_FLOAT32X4 vout = MS_MAXQ_F32(MS_ADDQ_F32(vin0_opt, vin1), zeros);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = MSMAX(in0[0] + in1[index], 0);
    }
  } else {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
      MS_FLOAT32X8 vout = MS_MAX256_F32(MS_ADD256_F32(vin0, vin1_opt_8), zeros_8);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
      MS_FLOAT32X4 vout = MS_MAXQ_F32(MS_ADDQ_F32(vin0, vin1_opt), zeros);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = MSMAX(in0[index] + in1[0], 0);
    }
  }
  return NNACL_OK;
}

int ElementOptAddRelu6(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
#ifdef ENABLE_AVX
  MS_FLOAT32X8 vin0_opt_8 = MS_MOV256_F32(in0[0]);
  MS_FLOAT32X8 vin1_opt_8 = MS_MOV256_F32(in1[0]);
  MS_FLOAT32X8 zeros_8 = MS_MOV256_F32(0.0f);
  MS_FLOAT32X8 bounds_8 = MS_MOV256_F32(6.0f);
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_FLOAT32X4 vin0_opt = MS_MOVQ_F32(in0[0]);
  MS_FLOAT32X4 vin1_opt = MS_MOVQ_F32(in1[0]);
  MS_FLOAT32X4 zeros = MS_MOVQ_F32(0.0f);
  MS_FLOAT32X4 bounds = MS_MOVQ_F32(6.0f);
#endif
  int index = 0;
  if (param->in_elements_num0_ == 1) {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
      MS_FLOAT32X8 vout = MS_MIN256_F32(MS_MAX256_F32(MS_ADD256_F32(vin0_opt_8, vin1), zeros_8), bounds_8);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
      MS_FLOAT32X4 vout = MS_MINQ_F32(MS_MAXQ_F32(MS_ADDQ_F32(vin0_opt, vin1), zeros), bounds);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = MSMIN(MSMAX(in0[0] + in1[index], 0), 6);
    }
  } else {
#ifdef ENABLE_AVX
    for (; index <= size - C8NUM; index += C8NUM) {
      MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
      MS_FLOAT32X8 vout = MS_MIN256_F32(MS_MAX256_F32(MS_ADD256_F32(vin0, vin1_opt_8), zeros_8), bounds_8);
      MS_ST256_F32(out + index, vout);
    }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
    for (; index <= size - C4NUM; index += C4NUM) {
      MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
      MS_FLOAT32X4 vout = MS_MINQ_F32(MS_MAXQ_F32(MS_ADDQ_F32(vin0, vin1_opt), zeros), bounds);
      MS_STQ_F32(out + index, vout);
    }
#endif
    for (; index < size; index++) {
      out[index] = MSMIN(MSMAX(in0[index] + in1[0], 0), 6);
    }
  }

  return NNACL_OK;
}

int BroadcastAdd(const float *in0, const float *in1, float *tile_in0, float *tile_in1, float *out, int size,
                 ArithmeticParameter *param) {
  TileDimensionsFp32(in0, in1, tile_in0, tile_in1, param);
  return ElementAdd(tile_in0, tile_in1, out, size);
}

int ElementAdd(const float *in0, const float *in1, float *out, int size) {
  int index = 0;
#ifdef ENABLE_AVX512
  for (; index <= size - C16NUM; index += C16NUM) {
    MS_FLOAT32X16 vin0 = MS_LD512_F32(in0 + index);
    MS_FLOAT32X16 vin1 = MS_LD512_F32(in1 + index);
    MS_FLOAT32X16 vout = MS_ADD512_F32(vin0, vin1);
    MS_ST512_F32(out + index, vout);
  }
#endif
#ifdef ENABLE_AVX
  for (; index <= size - C8NUM; index += C8NUM) {
    MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
    MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
    MS_FLOAT32X8 vout = MS_ADD256_F32(vin0, vin1);
    MS_ST256_F32(out + index, vout);
  }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  for (; index <= size - C4NUM; index += C4NUM) {
    MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
    MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
    MS_FLOAT32X4 vout = MS_ADDQ_F32(vin0, vin1);
    MS_STQ_F32(out + index, vout);
  }
#endif
  for (; index < size; index++) {
    out[index] = in0[index] + in1[index];
  }
  return NNACL_OK;
}

int ElementAddRelu(const float *in0, const float *in1, float *out, int size) {
  int index = 0;
#ifdef ENABLE_AVX512
  MS_FLOAT32X16 zeros_16 = MS_MOV512_F32(0.0f);
  for (; index <= size - C16NUM; index += C16NUM) {
    MS_FLOAT32X16 vin0 = MS_LD512_F32(in0 + index);
    MS_FLOAT32X16 vin1 = MS_LD512_F32(in1 + index);
    MS_FLOAT32X16 vout = MS_ADD512_F32(vin0, vin1);
    __mmask16 mask = MS_CMP512_F32(vout, zeros_16, 30);
    vout = MS_BLEND512_F32(mask, zeros_16, vout);
    MS_ST512_F32(out + index, vout);
  }
#endif
#ifdef ENABLE_AVX
  MS_FLOAT32X8 zeros_8 = MS_MOV256_F32(0.0f);
  for (; index <= size - C8NUM; index += C8NUM) {
    MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
    MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
    MS_FLOAT32X8 vout = MS_ADD256_F32(vin0, vin1);
    vout = MS_BLEND256_F32(zeros_8, vout, MS_CMP256_F32(vout, zeros_8, 30));
    MS_ST256_F32(out + index, vout);
  }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_FLOAT32X4 zeros = MS_MOVQ_F32(0.0f);
  for (; index <= size - C4NUM; index += C4NUM) {
    MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
    MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
    MS_FLOAT32X4 vout = MS_ADDQ_F32(vin0, vin1);
    vout = MS_BLENDQ_F32(vout, zeros, MS_CMPLEQ_F32(vout, zeros));
    MS_STQ_F32(out + index, vout);
  }
#endif
  for (; index < size; index++) {
    float res = in0[index] + in1[index];
    out[index] = res > 0 ? res : 0;
  }
  return NNACL_OK;
}

int ElementAddRelu6(const float *in0, const float *in1, float *out, int size) {
  int index = 0;
#ifdef ENABLE_AVX512
  MS_FLOAT32X16 zeros_16 = MS_MOV512_F32(0.0f);
  MS_FLOAT32X16 bounds_16 = MS_MOV512_F32(6.0f);
  for (; index <= size - C16NUM; index += C16NUM) {
    MS_FLOAT32X16 vin0 = MS_LD512_F32(in0 + index);
    MS_FLOAT32X16 vin1 = MS_LD512_F32(in1 + index);
    MS_FLOAT32X16 vout = MS_MIN512_F32(MS_MAX512_F32(MS_ADD512_F32(vin0, vin1), zeros_16), bounds_16);
    MS_ST512_F32(out + index, vout);
  }
#endif
#ifdef ENABLE_AVX
  MS_FLOAT32X8 zeros_8 = MS_MOV256_F32(0.0f);
  MS_FLOAT32X8 bounds_8 = MS_MOV256_F32(6.0f);
  for (; index <= size - C8NUM; index += C8NUM) {
    MS_FLOAT32X8 vin0 = MS_LD256_F32(in0 + index);
    MS_FLOAT32X8 vin1 = MS_LD256_F32(in1 + index);
    MS_FLOAT32X8 vout = MS_MIN256_F32(MS_MAX256_F32(MS_ADD256_F32(vin0, vin1), zeros_8), bounds_8);
    MS_ST256_F32(out + index, vout);
  }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  MS_FLOAT32X4 zeros = MS_MOVQ_F32(0.0f);
  MS_FLOAT32X4 bounds = MS_MOVQ_F32(6.0f);
  for (; index <= size - C4NUM; index += C4NUM) {
    MS_FLOAT32X4 vin0 = MS_LDQ_F32(in0 + index);
    MS_FLOAT32X4 vin1 = MS_LDQ_F32(in1 + index);
    MS_FLOAT32X4 vout = MS_MINQ_F32(MS_MAXQ_F32(MS_ADDQ_F32(vin0, vin1), zeros), bounds);
    MS_STQ_F32(out + index, vout);
  }
#endif
  for (; index < size; index++) {
    out[index] = MSMIN(MSMAX(in0[index] + in1[index], 0), 6);
  }
  return NNACL_OK;
}

int ElementAddInt(const int *in0, const int *in1, int *out, int size) {
  int index = 0;
#ifdef ENABLE_AVX
  for (; index <= size - C8NUM; index += C8NUM) {
    MS_INT32X8 vin0 = MS_LD256_EPI32(in0 + index);
    MS_INT32X8 vin1 = MS_LD256_EPI32(in1 + index);
    MS_INT32X8 vout = MS_ADD256_EPI32(vin0, vin1);
    MS_ST256_EPI32(out + index, vout);
  }
#endif
#if defined(ENABLE_NEON) || defined(ENABLE_SSE)
  for (; index <= size - C4NUM; index += C4NUM) {
    MS_INT32X4 vin0 = MS_LDQ_EPI32(in0 + index);
    MS_INT32X4 vin1 = MS_LDQ_EPI32(in1 + index);
    MS_INT32X4 vout = MS_ADDQ_EPI32(vin0, vin1);
    MS_STQ_EPI32(out + index, vout);
  }
#endif
  for (; index < size; index++) {
    out[index] = in0[index] + in1[index];
  }
  return NNACL_OK;
}
