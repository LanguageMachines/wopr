#!/usr/bin/env python
#
# NB: difference in wopr transpose treatment.

import time


def levenshtein(a,b):
    "Calculates the Levenshtein distance between a and b."
    n, m = len(a), len(b)
    if n > m:
        # Make sure n <= m, to use O(min(n,m)) space
        a,b = b,a
        n,m = m,n
        
    current = range(n+1)
    for i in range(1,m+1):
        previous, current = current, [i]+[0]*n
        for j in range(1,n+1):
            add, delete = previous[j]+1, current[j-1]+1
            change = previous[j-1]
            if a[j-1] != b[i-1]:
                change = change + 1
            current[j] = min(add, delete, change)
            
    return current[n]


def levenshtein_wp(s1, s2):
    if len(s1) < len(s2):
        return levenshtein(s2, s1)
    if not s1:
        return len(s2)
 
    previous_row = xrange(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1 # j+1 instead of j since previous_row and current_row are one character longer
            deletions = current_row[j] + 1       # than s2
            substitutions = previous_row[j] + (c1 != c2)
            if j-i == 1: #this 'correction' makes swap LD:1
                substitutions -= 1
            current_row.append(min(insertions, deletions, substitutions))

        previous_row = current_row
 
    return previous_row[-1]


if __name__=="__main__":
    from sys import argv
    t0 = time.time()
    for i in xrange(0,10000):
        x = levenshtein(argv[1],argv[2])
    print x
    t1 = time.time()
    print t1-t0
    t0 = time.time()
    for i in xrange(0,10000):
        x =  levenshtein_wp(argv[1],argv[2])
    print x
    t1 = time.time()
    print t1-t0

