#!/usr/bin/env python3

import sys
import numpy as np
import scipy.linalg as la

n = 3

if sys.argv.__len__()!=n*n+1:
    print( "bad # args" )
    sys.exit()


A = np.empty([n,n], dtype='f8')

for j in range(n):
    for i in range(n):
        A[j][i] = sys.argv[1+j*n+i]

print( "A:" )
print( A )

Q,R,P = la.qr(A, pivoting=True)

print( "Q:" )
print( Q )

print( "R:" )
print( R )

print( "P:" )
print( P )
