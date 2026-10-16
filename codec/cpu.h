
#if !defined(WELS_CPU_DETECTION_H__)
#define WELS_CPU_DETECTION_H__

#include "typedefs.h"
#include "cpu_core.h"


#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#if defined(X86_ASM)
/*
 *  cpuid support verify routine
 *  return 0 if cpuid is not supported by cpu
 */
int32_t  WelsCPUIdVerify();

void WelsCPUId (uint32_t uiIndex, uint32_t* pFeatureA, uint32_t* pFeatureB, uint32_t* pFeatureC, uint32_t* pFeatureD);

int32_t WelsCPUSupportAVX (uint32_t eax, uint32_t ecx);
int32_t WelsCPUSupportFMA (uint32_t eax, uint32_t ecx);
uint32_t WelsCPUDetectAVX512();

void WelsEmms();

/*
 *  clear FPU registers states for potential float based calculation if support
 */
void     WelsCPURestore (const uint32_t kuiCPU);

#else
#define WelsEmms()
#endif

uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors);

#if defined(__cplusplus)
}
#endif//__cplusplus

#endif//WELS_CPU_DETECTION_H__
