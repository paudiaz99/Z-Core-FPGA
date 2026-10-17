*=======================================================================
* Program: STREAM
* Programmer: John D. McCalpin
* RCS Revision: $Id: stream.f,v 5.6 2005/10/04 00:20:48 mccalpin Exp mccalpin $
*-----------------------------------------------------------------------
* Copyright 1991-2003: John D. McCalpin
*-----------------------------------------------------------------------
* License:
*  1. You are free to use this program and/or to redistribute
*     this program.
*  2. You are free to modify this program for your own use,
*     including commercial use, subject to the publication
*     restrictions in item 3.
*  3. You are free to publish results obtained from running this
*     program, or from works that you derive from this program,
*     with the following limitations:
*     3a. In order to be referred to as "STREAM benchmark results",
*         published results must be in conformance to the STREAM
*         Run Rules, (briefly reviewed below) published at
*         http://www.cs.virginia.edu/stream/ref.html
*         and incorporated herein by reference.
*         As the copyright holder, John McCalpin retains the
*         right to determine conformity with the Run Rules.
*     3b. Results based on modified source code or on runs not in
*         accordance with the STREAM Run Rules must be clearly
*         labelled whenever they are published.  Examples of
*         proper labelling include:
*         "tuned STREAM benchmark results" 
*         "based on a variant of the STREAM benchmark code"
*         Other comparable, clear and reasonable labelling is
*         acceptable.
*     3c. Submission of results to the STREAM benchmark web site
*         is encouraged, but not required.
*  4. Use of this program or creation of derived works based on this
*     program constitutes acceptance of these licensing restrictions.
*  5. Absolutely no warranty is expressed or implied.
*-----------------------------------------------------------------------
* This program measures sustained memory transfer rates in MB/s for
* simple computational kernels coded in FORTRAN.
*
* The intent is to demonstrate the extent to which ordinary user
* code can exploit the main memory bandwidth of the system under
* test.
*=======================================================================
* The STREAM web page is at:
*          http://www.streambench.org
*
* Most of the content is currently hosted at:
*          http://www.cs.virginia.edu/stream/
*
* BRIEF INSTRUCTIONS: 
*       0) See http://www.cs.virginia.edu/stream/ref.html for details
*       1) STREAM requires a timing function called mysecond().
*          Several examples are provided in this directory.
*          "CPU" timers are only allowed for uniprocessor runs.
*          "Wall-clock" timers are required for all multiprocessor runs.
*       2) The STREAM array sizes must be set to size the test.
*          The value "N" must be chosen so that each of the three
*          arrays is at least 4x larger than the sum of all the last-
*          level caches used in the run, or 1 million elements, which-
*          ever is larger.
*          ------------------------------------------------------------
*          Note that you are free to use any array length and offset
*          that makes each array 4x larger than the last-level cache.
*          The intent is to determine the *best* sustainable bandwidth
*          available with this simple coding.  Of course, lower values
*          are usually fairly easy to obtain on cached machines, but 
*          by keeping the test to the *best* results, the answers are
*          easier to interpret.
*          You may put the arrays in common or not, at your discretion.
*          There is a commented-out COMMON statement below.
*          Fortran90 "allocatable" arrays are fine, too.
*          ------------------------------------------------------------
*       3) Compile the code with full optimization.  Many compilers
*          generate unreasonably bad code before the optimizer tightens
*          things up.  If the results are unreasonably good, on the
*          other hand, the optimizer might be too smart for me
*          Please let me know if this happens.
*       4) Mail the results to mccalpin@cs.virginia.edu
*          Be sure to include:
*               a) computer hardware model number and software revision
*               b) the compiler flags
*               c) all of the output from the test case.
*          Please let me know if you do not want your name posted along
*          with the submitted results.
*       5) See the web page for more comments about the run rules and
*          about interpretation of the results.
*
* Thanks,
*   Dr. Bandwidth
*=========================================================================
*
      PROGRAM stream
      INTEGER n,offset,ndim,ntimes
      PARAMETER (n=20000000,offset=0,ndim=n+offset,ntimes=10)
      DOUBLE PRECISION scalar,t
      INTEGER j,k,nbpw,quantum
      DOUBLE PRECISION maxtime(4),mintime(4),avgtime(4),
     $                 times(4,ntimes)
      INTEGER bytes(4)
      CHARACTER label(4)*11
      DOUBLE PRECISION mysecond
      INTEGER checktick,realsize
      EXTERNAL mysecond,checktick,realsize
      INTRINSIC dble,max,min,nint,sqrt
      DOUBLE PRECISION a(ndim),b(ndim),c(ndim)
      DATA avgtime/4*0.0D0/,mintime/4*1.0D+36/,maxtime/4*0.0D0/
      DATA label/'Copy:      ','Scale:     ','Add:       ',
     $     'Triad:     '/
      DATA bytes/2,2,3,3/


      nbpw = realsize()

      PRINT *,'----------------------------------------------'
      PRINT *,'STREAM Version $Revision: 5.6 $'
      PRINT *,'----------------------------------------------'
      WRITE (*,FMT=9010) 'Array size = ',n
      WRITE (*,FMT=9010) 'Offset     = ',offset
      WRITE (*,FMT=9020) 'The total memory requirement is ',
     $  3*nbpw*n/ (1024*1024),' MB'
      WRITE (*,FMT=9030) 'You are running each test ',ntimes,' times'
      WRITE (*,FMT=9030) '--'
      WRITE (*,FMT=9030) 'The *best* time for each test is used'
      WRITE (*,FMT=9030) '*EXCLUDING* the first and last iterations'

      PRINT *,'----------------------------------------------'

      PRINT *,'----------------------------------------------'
      PRINT *,'Printing one line per active thread....'

      DO 10 j = 1,n
          a(j) = 2.0d0
          b(j) = 0.5D0
          c(j) = 0.0D0
   10 CONTINUE
      t = mysecond()
      DO 20 j = 1,n
          a(j) = 0.5d0*a(j)
   20 CONTINUE
      t = mysecond() - t
      PRINT *,'----------------------------------------------------'
      quantum = checktick()
      WRITE (*,FMT=9000)
     $  'Your clock granularity/precision appears to be ',quantum,
     $  ' microseconds'
      PRINT *,'----------------------------------------------------'

      scalar = 0.5d0*a(1)
      DO 70 k = 1,ntimes

          t = mysecond()
          a(1) = a(1) + t
          DO 30 j = 1,n
              c(j) = a(j)
   30     CONTINUE
          t = mysecond() - t
          c(n) = c(n) + t
          times(1,k) = t

          t = mysecond()
          c(1) = c(1) + t
          DO 40 j = 1,n
              b(j) = scalar*c(j)
   40     CONTINUE
          t = mysecond() - t
          b(n) = b(n) + t
          times(2,k) = t

          t = mysecond()
          a(1) = a(1) + t
          DO 50 j = 1,n
              c(j) = a(j) + b(j)
   50     CONTINUE
          t = mysecond() - t
          c(n) = c(n) + t
          times(3,k) = t

          t = mysecond()
          b(1) = b(1) + t
          DO 60 j = 1,n
              a(j) = b(j) + scalar*c(j)
   60     CONTINUE
          t = mysecond() - t
          a(n) = a(n) + t
          times(4,k) = t
   70 CONTINUE

      DO 90 k = 2,ntimes
          DO 80 j = 1,4
              avgtime(j) = avgtime(j) + times(j,k)
              mintime(j) = min(mintime(j),times(j,k))
              maxtime(j) = max(maxtime(j),times(j,k))
   80     CONTINUE
   90 CONTINUE
      WRITE (*,FMT=9040)
      DO 100 j = 1,4
          avgtime(j) = avgtime(j)/dble(ntimes-1)
          WRITE (*,FMT=9050) label(j),n/1.0D6*bytes(j)*nbpw/mintime(j),
     $      avgtime(j),mintime(j),maxtime(j)
  100 CONTINUE
      PRINT *,'----------------------------------------------------'
      CALL checksums (a,b,c,n,ntimes)
      PRINT *,'----------------------------------------------------'

 9000 FORMAT (1x,a,i6,a)
 9010 FORMAT (1x,a,i10)
 9020 FORMAT (1x,a,i4,a)
 9030 FORMAT (1x,a,i3,a,a)
 9040 FORMAT ('Function',5x,'Rate (MB/s)  Avg time   Min time  Max time'
     $       )
 9050 FORMAT (a,f10.1,2x,3(f10.6,2x))
      END

      INTEGER FUNCTION realsize()

      DOUBLE PRECISION result,test
      INTEGER j,ndigits
      DOUBLE PRECISION ref(30)
      EXTERNAL confuse
      INTRINSIC abs,acos,log10,sqrt


   10 DO 20 j = 1,30
          ref(j) = 1.0d0 + 10.0d0** (-j)
   20 CONTINUE

      DO 30 j = 1,30
          test = ref(j)
          ndigits = j
          CALL confuse(test,result)
          IF (test.EQ.1.0D0) THEN
              GO TO 40
          END IF
   30 CONTINUE
      GO TO 50

   40 WRITE (*,FMT='(a)')
     $  '----------------------------------------------'
      WRITE (*,FMT='(1x,a,i2,a)') 'Double precision appears to have ',
     $  ndigits,' digits of accuracy'
      IF (ndigits.LE.8) THEN
          realsize = 4
      ELSE
          realsize = 8
      END IF
      WRITE (*,FMT='(1x,a,i1,a)') 'Assuming ',realsize,
     $  ' bytes per DOUBLE PRECISION word'
      WRITE (*,FMT='(a)')
     $  '----------------------------------------------'
      RETURN

   50 PRINT *,'Hmmmm.  I am unable to determine the size.'
      PRINT *,'Please enter the number of Bytes per DOUBLE PRECISION',
     $  ' number : '
      READ (*,FMT=*) realsize
      IF (realsize.NE.4 .AND. realsize.NE.8) THEN
          PRINT *,'Your answer ',realsize,' does not make sense.'
          PRINT *,'Try again.'
          PRINT *,'Please enter the number of Bytes per ',
     $      'DOUBLE PRECISION number : '
          READ (*,FMT=*) realsize
      END IF
      PRINT *,'You have manually entered a size of ',realsize,
     $  ' bytes per DOUBLE PRECISION number'
      WRITE (*,FMT='(a)')
     $  '----------------------------------------------'
      END

      SUBROUTINE confuse(q,r)
      DOUBLE PRECISION q,r
      INTRINSIC cos
      r = cos(q)
      RETURN
      END

      INTEGER FUNCTION checktick()

      INTEGER n
      PARAMETER (n=20)
      DOUBLE PRECISION t1,t2
      INTEGER i,j,jmin
      DOUBLE PRECISION timesfound(n)
      DOUBLE PRECISION mysecond
      EXTERNAL mysecond
      INTRINSIC max,min,nint
      i = 0

   10 t2 = mysecond()
      IF (t2.EQ.t1) GO TO 10

      t1 = t2
      i = i + 1
      timesfound(i) = t1
      IF (i.LT.n) GO TO 10

      jmin = 1000000
      DO 20 i = 2,n
          j = nint((timesfound(i)-timesfound(i-1))*1d6)
          jmin = min(jmin,max(j,0))
   20 CONTINUE

      IF (jmin.GT.0) THEN
          checktick = jmin
      ELSE
          PRINT *,'Your clock granularity appears to be less ',
     $      'than one microsecond'
          checktick = 1
      END IF
      RETURN


      END




      SUBROUTINE checksums(a,b,c,n,ntimes)
      DOUBLE PRECISION a(*),b(*),c(*)
      INTEGER n,ntimes
      DOUBLE PRECISION aa,bb,cc,scalar,suma,sumb,sumc,epsilon
      INTEGER k


      aa = 2.0D0
      bb = 0.5D0
      cc = 0.0D0
      aa = 0.5D0*aa
      scalar = 0.5d0*aa
      DO k = 1,ntimes
          cc = aa
          bb = scalar*cc
          cc = aa + bb
          aa = bb + scalar*cc
      END DO
      aa = aa*DBLE(n-2)
      bb = bb*DBLE(n-2)
      cc = cc*DBLE(n-2)


      suma = 0.0d0
      sumb = 0.0d0
      sumc = 0.0d0
      DO 110 j = 2,n-1
          suma = suma + a(j)
          sumb = sumb + b(j)
          sumc = sumc + c(j)
  110 CONTINUE

      epsilon = 1.D-6

      IF (ABS(suma-aa)/suma .GT. epsilon) THEN
          PRINT *,'Failed Validation on array a()'
          PRINT *,'Target   Sum of a is = ',aa
          PRINT *,'Computed Sum of a is = ',suma
      ELSEIF (ABS(sumb-bb)/sumb .GT. epsilon) THEN
          PRINT *,'Failed Validation on array b()'
          PRINT *,'Target   Sum of b is = ',bb
          PRINT *,'Computed Sum of b is = ',sumb
      ELSEIF (ABS(sumc-cc)/sumc .GT. epsilon) THEN
          PRINT *,'Failed Validation on array c()'
          PRINT *,'Target   Sum of c is = ',cc
          PRINT *,'Computed Sum of c is = ',sumc
      ELSE
          PRINT *,'Solution Validates!'
      ENDIF

      END

