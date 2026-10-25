; Self-checking test of the HD6309 instructions in m6809-sim.
; Build: lwasm --6309 --format=raw -o test6309.bin test6309.asm
; Run:   m6809-sim -6309 -load 1000 test6309.bin  (exit 0 = pass, else the
;        number of the failing check)
EXIT    equ     $FF01
        org     $1000
start   lds     #$8000
        clr     <$40            ; DP is 0
        lda     #1
        sta     num
; 1: LDW/STW/ADDW/SUBW/CMPW
        ldw     #$1234
        addw    #$1111
        cmpw    #$2345
        lbne    fail
        subw    word1           ; $2345 - $0345
        cmpw    #$2000
        lbne    fail
        stw     tmp
        ldd     tmp
        cmpd    #$2000
        lbne    fail
        inc     num
; 2: E and F
        lde     #$12
        ldf     #$34
        cmpw    #$1234
        lbne    fail
        adde    #1
        addf    #$10
        cmpw    #$1344
        lbne    fail
        ste     tmp
        stf     tmp+1
        ldx     tmp
        cmpx    #$1344
        lbne    fail
        inc     num
; 3: register-register ops
        ldd     #$1000
        ldx     #$0234
        addr    x,d
        cmpd    #$1234
        lbne    fail
        ldy     #$0034
        subr    y,d
        cmpd    #$1200
        lbne    fail
        lda     #$F0
        ldb     #$3C
        andr    b,a
        cmpa    #$30
        lbne    fail
        ldd     #$0005
        ldx     #$0007
        cmpr    x,d             ; 5 - 7: borrow
        lbcc    fail
        ldd     #$FFFF
        ldx     #$0001
        addr    x,d             ; carry out
        lbcc    fail
        ldd     #0
        ldx     #0
        adcr    x,d             ; 0 + 0 + 1
        cmpd    #1
        lbne    fail
        inc     num
; 4: 16-bit unary on D and W
        ldd     #$8001
        lsrd
        lbcc    fail
        cmpd    #$4000
        lbne    fail
        asld
        asld
        cmpd    #$0000
        lbne    fail
        ldd     #5
        negd
        cmpd    #-5
        lbne    fail
        incd
        incd
        decd
        cmpd    #-4
        lbne    fail
        clrd
        lbne    fail
        ldw     #$00FF
        incw
        cmpw    #$0100
        lbne    fail
        comw
        cmpw    #$FEFF
        lbne    fail
        ldd     #$8000
        asrd
        cmpd    #$C000
        lbne    fail
        orcc    #1
        rord
        cmpd    #$E000
        lbne    fail
        clrw
        lbne    fail
        inc     num
; 5: ANDD/ORD/EORD/ADCD/SBCD/BITD
        ldd     #$F0F0
        andd    #$3C3C
        cmpd    #$3030
        lbne    fail
        ord     #$0101
        cmpd    #$3131
        lbne    fail
        eord    #$FFFF
        cmpd    #$CECE
        lbne    fail
        orcc    #1
        adcd    #$0001
        cmpd    #$CED0
        lbne    fail
        orcc    #1
        sbcd    #$00D0
        cmpd    #$CDFF
        lbne    fail
        bitd    #$0200
        lbne    fail
        inc     num
; 6: LDQ/STQ/SEXW
        ldq     #$12345678
        cmpd    #$1234
        lbne    fail
        cmpw    #$5678
        lbne    fail
        stq     quad
        ldd     quad+2
        cmpd    #$5678
        lbne    fail
        ldq     quad
        cmpw    #$5678
        lbne    fail
        ldw     #$8000
        sexw
        cmpd    #$FFFF
        lbne    fail
        ldw     #$7FFF
        sexw
        lbeq    fail            ; Z from Q: nonzero
        cmpd    #0
        lbne    fail
        inc     num
; 7: MULD/DIVD/DIVQ
        ldd     #-300
        muld    #200            ; -60000
        cmpd    #$FFFF
        lbne    fail
        cmpw    #$15A0
        lbne    fail
        ldd     #100
        divd    #7              ; 14 r 2
        cmpb    #14
        lbne    fail
        cmpa    #2
        lbne    fail
        ldd     #-100
        divd    #7              ; -14 r -2
        cmpb    #-14
        lbne    fail
        cmpa    #-2
        lbne    fail
        ldq     #100000
        divq    #300            ; 333 r 100
        cmpw    #333
        lbne    fail
        cmpd    #100
        lbne    fail
        inc     num
; 8: indexed modes with W, E, F
        ldx     #table
        lde     #2
        lda     e,x
        cmpa    #3
        lbne    fail
        ldf     #1
        lda     f,x
        cmpa    #2
        lbne    fail
        ldw     #3
        lda     w,x
        cmpa    #4
        lbne    fail
        ldw     #table
        lda     ,w
        cmpa    #1
        lbne    fail
        lda     1,w
        cmpa    #2
        lbne    fail
        ldd     ,w++
        cmpd    #$0102
        lbne    fail
        cmpw    #table+2
        lbne    fail
        ldd     ,--w
        cmpd    #$0102
        lbne    fail
        ldw     #ptr
        lda     [,w]
        cmpa    #1
        lbne    fail
        inc     num
; 9: AIM/OIM/EIM/TIM, TFM
        lda     #$F0
        sta     tmp
        aim     #$3C,tmp
        oim     #$01,tmp
        eim     #$FF,tmp
        lda     tmp
        cmpa    #$CE
        lbne    fail
        tim     #$01,tmp
        lbne    fail
        ldx     #table
        ldy     #buf
        ldw     #4
        tfm     x+,y+
        ldd     buf+2
        cmpd    #$0304
        lbne    fail
        cmpw    #0
        lbne    fail
        inc     num
; 10: TFR/EXG with W, E, F and the zero register
        ldw     #$ABCD
        tfr     w,x
        cmpx    #$ABCD
        lbne    fail
        lda     #$55
        tfr     a,e
        cmpe    #$55
        lbne    fail
        ldd     #$1111
        exg     d,w
        cmpd    #$55CD
        lbne    fail
        cmpw    #$1111
        lbne    fail
        tfr     0,x
        cmpx    #0
        lbne    fail
        pshsw
        clrw
        pulsw
        cmpw    #$1111
        lbne    fail
        inc     num
pass    clr     EXIT
fail    lda     num
        sta     EXIT
num     rmb     1
tmp     rmb     2
word1   fdb     $0345
quad    rmb     4
table   fcb     1,2,3,4
ptr     fdb     table
buf     rmb     4
