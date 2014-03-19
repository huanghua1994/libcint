#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "erd_integral.h"
#include "basisset.h"
#include "config.h"
#include "cint_def.h"

#ifdef __INTEL_OFFLOAD
#pragma offload_attribute(push, target(mic))
#endif


static void erd_max_scratch(BasisSet_t basis, ERD_t erd)
{
    int max_momentum;
    int max_primid;
    int maxnpgto;

    max_momentum = basis->max_momentum;
    max_primid = basis->max_nexp_id;
    maxnpgto = basis->nexp[max_primid];
        
    if (max_momentum < 2) {
        erd__memory_1111_csgto(maxnpgto, maxnpgto, maxnpgto, maxnpgto,
            max_momentum, max_momentum,
            max_momentum, max_momentum,
            1.0, 1.0, 1.0, 2.0, 2.0, 2.0,
            3.0, 3.0, 3.0, 4.0, 4.0, 4.0,
            &(erd->int_memory_opt),
            &(erd->fp_memory_opt));
    } else {
        erd__memory_csgto(maxnpgto, maxnpgto, maxnpgto, maxnpgto,
            max_momentum, max_momentum,
            max_momentum, max_momentum,
            1.0, 1.0, 1.0, 2.0, 2.0, 2.0,
            3.0, 3.0, 3.0, 4.0, 4.0, 4.0,
            ERD_SPHERIC,
            &(erd->int_memory_opt),
            &(erd->fp_memory_opt));
    }
}


#if 1
static CIntStatus_t create_vrrtable(BasisSet_t basis, ERD_t erd) {
    const int max_shella = basis->max_momentum + 1;
    erd->max_shella = max_shella;
    const int max_shellp = 2 * (max_shella - 1);
    const int tablesize = max_shellp + 1;
    const int total_combinations = (max_shellp + 1) * (max_shellp + 2) * (max_shellp + 3) / 6;
    int **vrrtable = (int **)malloc(sizeof(int *) * tablesize);

    int *vrrtable__ = (int *)malloc(sizeof(int) * 4 * total_combinations);
    if (NULL == vrrtable || NULL == vrrtable__) {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "memory allocation failed\n");
#endif
        return CINT_STATUS_ALLOC_FAILED;
    }

    int n = 0;
    for(int shella = 0; shella <= max_shellp; shella++)
    {
        vrrtable[shella] = &vrrtable__[n];
        int count = 0;
        for(int xf = shella; xf >= 0; xf--)
        {
            for(int yf = shella - xf; yf >= 0; yf--)
            {
                int zf = shella - xf - yf;
                    vrrtable[shella][count + 0] = xf;
                    vrrtable[shella][count + 1] = yf;
                    vrrtable[shella][count + 2] = zf;
                    vrrtable[shella][count + 3] = count;
                    count += 4;
            }
        }
        n += count;
    }
    erd->vrrtable = vrrtable;
    return CINT_STATUS_SUCCESS;
}

static CIntStatus_t destroy_vrrtable(ERD_t erd) {
    free(erd->vrrtable[0]);
    free(erd->vrrtable);
    return CINT_STATUS_SUCCESS;
}

#else
static CIntStatus_t create_vrrtable(BasisSet_t basis, ERD_t erd) {
    const int max_shella = basis->max_momentum + 1;
    erd->max_shella = max_shella;
    const int max_shellp = 2 * (max_shella - 1);
    const int tablesize = max_shellp + 1;
    const int total_combinations = (max_shellp + 1) * (max_shellp + 2) * (max_shellp + 3) / 6;
    int **vrrtable = (int **)malloc(sizeof(int *) * tablesize * tablesize);
    int *cum_sum = (int *)malloc(sizeof(int) * tablesize);
    int *temptable = (int *)malloc(sizeof(int) * 4 * total_combinations);
    if (NULL == vrrtable || NULL == temptable) {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "memory allocation failed\n");
#endif
        return CINT_STATUS_ALLOC_FAILED;
    }

    printf("tablesize = %d, total_combinations = %d\n", tablesize, total_combinations);
    int n = 0;
    for(int shella = 0; shella <= max_shellp; shella++)
    {
        cum_sum[shella] = n;
        //vrrtable[shella] = &temptable[4 * n];
        int count = 0;
        for(int xf = shella; xf >= 0; xf--)
        {
            for(int yf = shella - xf; yf >= 0; yf--)
            {
                int zf = shella - xf - yf;
                temptable[4 * n + 0] = xf;
                temptable[4 * n + 1] = yf;
                temptable[4 * n + 2] = zf;
                temptable[4 * n + 3] = count;
                count += 4;
                n++;
            }
        }
    }
    //printf("n = %d\n", n);
    
    int *vrrtable__ = (int *)malloc(sizeof(int) * (4 * tablesize * total_combinations * total_combinations + 32));
    n = 0;
    for(int shellp = 0; shellp < tablesize; shellp++)
    {
        for(int a = 0; a < total_combinations; a++)
        {
            for(int b = 0; b < total_combinations; b++)
            {
                //printf("a = %d, b = %d\n", a, b);
                vrrtable__[n * 3 + 0] = temptable[a * 4 + 0] * (shellp + 1) + temptable[b * 4 + 0];
                vrrtable__[n * 3 + 1] = temptable[a * 4 + 1] * (shellp + 1) + temptable[b * 4 + 1];
                vrrtable__[n * 3 + 2] = temptable[a * 4 + 2] * (shellp + 1) + temptable[b * 4 + 2];
                //printf("shellp = %d, a = %d, b = %d\n", shellp, a, b);
                //printf("n = %d, indx = %d\n", n, vrrtable__[n * 3 + 0]);
                n++;
            }
        }
    }

    for(int shella = 0; shella <= max_shellp; shella++)
    {
        for(int shellc = 0; shellc <= max_shellp; shellc++)
        {
            n = cum_sum[shella] * total_combinations + cum_sum[shellc];
            //printf("shella = %d, shellc = %d, n = %d\n", shella, shellc, n);
            vrrtable[shella * tablesize + shellc] = &vrrtable__[3 * n];
            int ind = (n + 2 * total_combinations * total_combinations);
            //printf("ind = %d, val0 = %d\n", ind, vrrtable__[3 * ind]);
        }
    }
   
    
    erd->vrrtable = vrrtable;
    //free(temptable);
    //free(cum_sum);
    printf("Done with vrrtable\n");
    return CINT_STATUS_SUCCESS;
}

static CIntStatus_t destroy_vrrtable(ERD_t erd) {
    printf("destroy_vrrtable\n");
    free(erd->vrrtable[0]);
    free(erd->vrrtable);
    printf("destroy_vrrtable exit \n");
    //exit(0);
    return CINT_STATUS_SUCCESS;
}
#endif

CIntStatus_t CInt_createERD(BasisSet_t basis, ERD_t *erd, int nthreads) {      
    if (nthreads <= 0) {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "invalid number of threads\n");
#endif
        return CINT_STATUS_INVALID_VALUE;
    }

    // malloc erd
    ERD_t e = (ERD_t)calloc(1, sizeof(struct ERD));
    if (NULL == e) {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "memory allocation failed\n");
#endif
        return CINT_STATUS_ALLOC_FAILED;
    }
    erd_max_scratch(basis, e);

    // memory scratch memory
    e->nthreads = nthreads;
    e->zcore = (double **)malloc(nthreads * sizeof(double *));
    e->icore = (int **)malloc(nthreads * sizeof(int *));   
    if ((NULL == e->zcore) || (NULL == e->icore)) {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "memory allocation failed\n");
#endif
        return CINT_STATUS_ALLOC_FAILED;
    }    
    for (int i = 0; i < nthreads; i++) {
        e->zcore[i] =
            (double *)ALIGNED_MALLOC(e->fp_memory_opt * sizeof(double));
        e->icore[i] =
            (int *)ALIGNED_MALLOC(e->int_memory_opt * sizeof(int));   
        if ((NULL == e->zcore[i]) || (NULL == e->icore[i])) {
    #ifndef __INTEL_OFFLOAD
            CINT_PRINTF(1, "memory allocation failed\n");
    #endif
            return CINT_STATUS_ALLOC_FAILED;
        }
    }

    // create vrr table
    const CIntStatus_t status = create_vrrtable(basis, e);
    if (status != CINT_STATUS_SUCCESS) {
        return status;
    }
    CINT_INFO("totally use %.3lf MB (%.3lf MB per thread)",
        (e->fp_memory_opt * sizeof(double)
        + e->int_memory_opt * sizeof(int)) * nthreads/1024.0/1024.0,
        (e->fp_memory_opt * sizeof(double)
        + e->int_memory_opt * sizeof(int))/1024.0/1024.0);
    *erd = e;
    return CINT_STATUS_SUCCESS;
}


CIntStatus_t CInt_destroyERD(ERD_t erd) {
    for (int i = 0; i < erd->nthreads; i++) {
        ALIGNED_FREE(erd->zcore[i]);
        ALIGNED_FREE(erd->icore[i]);
    }
    free(erd->zcore);
    free(erd->icore);

    destroy_vrrtable(erd);
    free(erd);

    return CINT_STATUS_SUCCESS;
}


CIntStatus_t CInt_computeShellQuartet( BasisSet_t basis, ERD_t erd, int tid,
                                        int A, int B, int C, int D,
                                        double **integrals, int *nints)
{
#if ( _DEBUG_LEVEL_ == 3 )
    if (A < 0 || A >= basis->nshells ||
        B < 0 || B >= basis->nshells ||
        C < 0 || C >= basis->nshells ||
        D < 0 || D >= basis->nshells)
    {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "invalid shell indices\n");
#endif
        *nints = 0;
        return CINT_STATUS_INVALID_VALUE;
    }
    if (tid <= 0 ||
        tid >= erd->nthreads)
    {
#ifndef __INTEL_OFFLOAD
        CINT_PRINTF(1, "invalid thread id\n");
#endif
        *nints = 0;
        return CINT_STATUS_INVALID_VALUE;    
    }
#endif

    const int shell1 = basis->momentum[A];
    const int shell2 = basis->momentum[B];
    const int shell3 = basis->momentum[C];
    const int shell4 = basis->momentum[D];
    const int maxshell = MAX(MAX(shell1, shell2), MAX(shell3, shell4));
    int nfirst;
    if (maxshell < 2) {        
        erd__1111_csgto(erd->fp_memory_opt,
            basis->nexp[A], basis->nexp[B], basis->nexp[C], basis->nexp[D],
            shell1, shell2, shell3, shell4,
            basis->xyz0[A*4], basis->xyz0[A*4+1], basis->xyz0[A*4+2],
            basis->xyz0[B*4], basis->xyz0[B*4+1], basis->xyz0[B*4+2],
            basis->xyz0[C*4], basis->xyz0[C*4+1], basis->xyz0[C*4+2],
            basis->xyz0[D*4], basis->xyz0[D*4+1], basis->xyz0[D*4+2],
            basis->exp[A], basis->exp[B], basis->exp[C], basis->exp[D],
            basis->cc[A], basis->cc[B], basis->cc[C], basis->cc[D],
            basis->norm[A], basis->norm[B], basis->norm[C], basis->norm[D],
            ERD_SCREEN, erd->icore[tid],
            nints, &nfirst, erd->zcore[tid]);
    } else {
        erd__csgto(erd->fp_memory_opt,
            basis->nexp[A], basis->nexp[B], basis->nexp[C], basis->nexp[D],
            shell1, shell2, shell3, shell4,
            basis->xyz0[A*4], basis->xyz0[A*4+1], basis->xyz0[A*4+2],
            basis->xyz0[B*4], basis->xyz0[B*4+1], basis->xyz0[B*4+2],
            basis->xyz0[C*4], basis->xyz0[C*4+1], basis->xyz0[C*4+2],
            basis->xyz0[D*4], basis->xyz0[D*4+1], basis->xyz0[D*4+2],
            basis->exp[A], basis->exp[B], basis->exp[C], basis->exp[D],
            basis->cc[A], basis->cc[B], basis->cc[C], basis->cc[D],
            basis->norm[A], basis->norm[B], basis->norm[C], basis->norm[D],
            erd->vrrtable, 2 * erd->max_shella,
            ERD_SPHERIC, ERD_SCREEN,
            erd->icore[tid], nints,
            &nfirst, erd->zcore[tid]);
    }

    *integrals = &(erd->zcore[tid][nfirst - 1]);

    return CINT_STATUS_SUCCESS;
}


int CInt_getMaxMemory(ERD_t erd) {
    return (erd->fp_memory_opt + erd->int_memory_opt);
}


#ifdef __INTEL_OFFLOAD
#pragma offload_attribute(pop)
#endif
