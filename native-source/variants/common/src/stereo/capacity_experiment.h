#ifndef GF2_CAPACITY_EXPERIMENT_H
#define GF2_CAPACITY_EXPERIMENT_H
/* User-approved audit only. Default builds retain their exact two-flight path. */
#ifdef GF2VR_CAPACITY_EXPERIMENT
#ifndef GF2VR_EXPERIMENT_N
#define GF2VR_EXPERIMENT_N 3
#endif
#if GF2VR_EXPERIMENT_N != 3
#error This audited round supports numeric capacity 3 only
#endif
#endif
#endif
