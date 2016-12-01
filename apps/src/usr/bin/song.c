#include <k/sys.h>
#include <k/log.h>

#define BPM 128

#define C0 16.35
#define D0 18.35
#define E0 20.60
#define F0 21.83
#define G0 24.50
#define A0 27.50
#define B0 30.87
#define C1 32.70
#define D1 36.71
#define E1 41.20
#define F1 43.65
#define G1 49.00
#define A1 55.00
#define B1 61.74
#define C2 65.41
#define D2 73.42
#define E2 82.41
#define F2 87.31
#define G2 98.00
#define A2 110.00
#define B2 123.47
#define C3 130.81
#define D3 146.83
#define E3 164.81
#define F3 174.61
#define G3 196.00
#define A3 220.00
#define B3 246.94
#define C4 261.63
#define D4 293.66
#define E4 329.63
#define F4 349.23
#define G4 392.00
#define A4 440.00
#define B4 493.88
#define C5 523.25
#define D5 587.33
#define E5 659.26
#define F5 698.46
#define G5 783.99
#define A5 880.00
#define B5 987.77
#define C6 1046.50
#define D6 1174.66
#define E6 1318.51
#define F6 1396.91
#define G6 1567.98
#define A6 1760.00
#define B6 1975.53
#define C7 2093.00
#define D7 2349.32
#define E7 2637.02
#define F7 2793.83
#define G7 3135.96
#define A7 3520.00
#define B7 3951.07
#define C8 4186.01
#define D8 4698.64
#define REST 1

#define ROUND(f) ((f >= 0) ? (int)(f+0.5) : (int)(f-0.5))
#define MILLIS(denominator) ROUND(60000.0f / (BPM * denominator))
#define PLAY(f, t) playsound(ROUND(f), MILLIS(t))

#define whole(f)     PLAY(f,  1)
#define half(f)      PLAY(f,  2)
#define quarter(f)   PLAY(f,  4)
#define eighth(f)    PLAY(f,  8)
#define sixteenth(f) PLAY(f, 16)

static inline void playsound(int freq, int millis) {
    play(freq);
    _msleep(millis);
    stop();
}

int main() {
    quarter(F2);
    quarter(D3);
    quarter(C3);
    half(B2);
    half(C3);
    half(G2);
    quarter(A2);
    half(E2);
    half(F2);
    half(C3);
    quarter(REST);
    quarter(E2);
    quarter(F2);
    half(G2);
    half(A2);
    half(E2);
    quarter(D2);
    half(B1);
    half(A1);

    return 0;
}
