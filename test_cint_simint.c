#include <stdio.h>
#include <libgen.h>
#include <omp.h>

#include "CInt.h"
#include "simint/simint.h"

void printvec(int a, int b, int nints, double *integrals)
{
    printf("shell pair %3d %3d\n", a, b);
    for (int i=0; i<nints; i++)
        printf("%3d    %e\n", i, integrals[i]);
}

void printvec4(int a, int b, int c, int d, int nints, double *integrals)
{
    printf("shell quartet %3d %3d %3d %3d\n", a, b, c, d);
    for (int i=0; i<nints; i++)
        printf("%3d    %e\n", i, integrals[i]);
}

int main (int argc, char **argv)
{
    // create basis set    
    BasisSet_t basis;
    CInt_createBasisSet(&basis);

    int nshells;
    int natoms;

    CInt_loadBasisSet(basis, argv[1], argv[2]);
    nshells = CInt_getNumShells(basis);
    natoms = CInt_getNumAtoms(basis);
    printf("Job information:\n");
    char *fname;
    fname = basename(argv[2]);
    printf("  molecule:  %s\n", fname);
    fname = basename(argv[1]);
    printf("  basisset:  %s\n", fname);
    printf("  charge     = %d\n", CInt_getTotalCharge(basis));
    printf("  #atoms     = %d\n", natoms);
    printf("  #shells    = %d\n", nshells);
    int nthreads = omp_get_max_threads();
    printf("  #nthreads_cpu = %d\n", nthreads);

    SIMINT_t simint;
    CInt_createSIMINT(basis, &simint, nthreads);

    double *integrals;
    int nints;

    for (int i=0; i<nshells; i++)
    for (int j=0; j<nshells; j++)
    for (int k=0; k<nshells; k++)
    for (int l=0; l<nshells; l++)
    {
        CInt_computeShellQuartet_SIMINT(basis, simint, /*tid*/0,
            i, j, k, l, &integrals, &nints);
        printvec4(i, j, k, l, nints, integrals);
    }

    // test one electron functions
    for (int i=0; i<nshells; i++)
    for (int j=0; j<nshells; j++)
    {
        CInt_computePairOvl_SIMINT(basis, simint, /*tid*/ 0, i, j, &integrals, &nints);
        printvec(i, j, nints, integrals);

        CInt_computePairCoreH_SIMINT(basis, simint, /*tid*/ 0, i, j, &integrals, &nints);
        //printf("%d %d num 1e integrals computed = %d\n", i, j, nints);
    }

    CInt_destroySIMINT(simint);
}
