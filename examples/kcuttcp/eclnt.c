/* Base version was generated with:
        ANHTROPIC Claude 4.6 Sonnet
    prompt 1 :
      "can you write a minimal tcp non-block echo server for linux in C"
    see esrv.c
    prompt 2 :
      "can you write an attendant client that can be used to measure throughput and latency for the above echo server -- input parameters should be packet size and rate for each transaction"
*/

/*
 * echoclient — throughput + latency benchmark for the non-blocking echo server
 *
 * build:  gcc -O2 -o echoclient echoclient.c -lm
 * usage:  ./echoclient -s <bytes> -r <tps> [-d <secs>] [-H host] [-p port]
 *
 * protocol: synchronous ping-pong
 *   send <size> bytes → wait for echo → record RTT → pace to target rate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_HOST      "127.0.0.1"
#define DEFAULT_PORT      8080
#define DEFAULT_SIZE      64
#define DEFAULT_RATE      100
#define DEFAULT_DURATION  10
#define DEFAULT_WARMUP    1
#define MAX_SAMPLES       4000000     /* ~32 MB of latency samples */

static volatile sig_atomic_t g_stop;
static void on_sig(int s) { (void)s; g_stop = 1; }

/* ----------------------------------------------------------------- timing */

static inline double mono_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline struct timespec mono_ts(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

static inline void ts_add_ns(struct timespec *ts, long ns)
{
    ts->tv_nsec += ns;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec++;
    }
}

/* ----------------------------------------------------------------- network */

static int tcp_connect(const char *host, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    /* disable Nagle: we measure RTT, not throughput of small writes */
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);

    struct sockaddr_in sa = {
        .sin_family = AF_INET,
        .sin_port   = htons((uint16_t)port),
    };
    if (inet_pton(AF_INET, host, &sa.sin_addr) != 1) {
        fprintf(stderr, "invalid address: %s\n", host);
        close(fd); return -1;
    }
    if (connect(fd, (struct sockaddr *)&sa, sizeof sa) != 0) {
        perror("connect"); close(fd); return -1;
    }
    return fd;
}

static int write_all(int fd, const void *buf, size_t n)
{
    const char *p = buf;
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w <= 0) { perror("write"); return -1; }
        p += w; n -= (size_t)w;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t n)
{
    char *p = buf;
    while (n) {
        ssize_t r = read(fd, p, n);
        if (r  < 0) { perror("read");                    return -1; }
        if (r == 0) { fputs("server closed\n", stderr);  return -1; }
        p += r; n -= (size_t)r;
    }
    return 0;
}

/* ----------------------------------------------------------------- stats */

static int cmp_dbl(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* linear-interpolation percentile on a sorted array */
static double pct(const double *s, long n, double p)
{
    if (n == 1) return s[0];
    double idx  = p * 0.01 * (double)(n - 1);
    long   lo   = (long)idx;
    double frac = idx - (double)lo;
    if (lo + 1 >= n) return s[n - 1];
    return s[lo] * (1.0 - frac) + s[lo + 1] * frac;
}

static void print_stats(double *lat, long n, double elapsed,
                         int pkt_size, int rate)
{
    if (n <= 0) { puts("no samples collected"); return; }

    qsort(lat, (size_t)n, sizeof *lat, cmp_dbl);

    double sum = 0;
    for (long i = 0; i < n; i++) sum += lat[i];
    double mean = sum / (double)n;

    double sq = 0;
    for (long i = 0; i < n; i++) {
        double d = lat[i] - mean;
        sq += d * d;
    }
    double stddev = sqrt(sq / (double)n);

    double tps  = (double)n / elapsed;
    double mbps = tps * (double)pkt_size * 2.0 / (1024.0 * 1024.0);

    printf("\n────────────────────────────────────────\n");
    printf("  %ld transactions over %.2f s\n", n, elapsed);
    printf("────────────────────────────────────────\n");
    printf("  target rate    %9d TPS\n",   rate);
    printf("  achieved rate  %9.0f TPS\n", tps);
    printf("  throughput     %9.2f MB/s   (tx+rx combined)\n", mbps);
    printf("────────────────────────────────────────\n");
    printf("  latency (RTT)\n");
    printf("    min      %8.1f µs\n", lat[0]           * 1e6);
    printf("    mean     %8.1f µs\n", mean              * 1e6);
    printf("    stddev   %8.1f µs\n", stddev            * 1e6);
    printf("    p50      %8.1f µs\n", pct(lat,n,50)    * 1e6);
    printf("    p75      %8.1f µs\n", pct(lat,n,75)    * 1e6);
    printf("    p90      %8.1f µs\n", pct(lat,n,90)    * 1e6);
    printf("    p95      %8.1f µs\n", pct(lat,n,95)    * 1e6);
    printf("    p99      %8.1f µs\n", pct(lat,n,99)    * 1e6);
    printf("    p99.9    %8.1f µs\n", pct(lat,n,99.9)  * 1e6);
    printf("    max      %8.1f µs\n", lat[n-1]          * 1e6);
    printf("────────────────────────────────────────\n");
}

/* ----------------------------------------------------------------- usage */

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [options]\n\n"
        "  -H <host>    server hostname/IP                      (default: " DEFAULT_HOST ")\n"
        "  -p <port>    server port                             (default: %d)\n"
        "  -s <bytes>   payload size                            (default: %d)\n"
        "  -r <tps>     target rate (TPS) - 0 fast as possible. (default: %d)\n"
        "  -d <secs>    test duration                           (default: %d)\n"
        "  -n <count>   fixed # of txns                         (overrides -d)\n"
        "  -w <secs>    warm-up duration                        (default: %d, 0=off)\n"
        "  -v           verbose: one line per txn\n"
        "  -h           this help\n\n"
        "examples:\n"
        "  %s -s 256 -r 5000 -d 30\n"
        "  %s -s 1024 -r 100000 -n 1000000 -w 0\n",
        prog,
        DEFAULT_PORT, DEFAULT_SIZE, DEFAULT_RATE,
        DEFAULT_DURATION, DEFAULT_WARMUP,
        prog, prog);
}

/* =================================================================== main */

int main(int argc, char *argv[])
{
    const char *host    = DEFAULT_HOST;
    int   port          = DEFAULT_PORT;
    int   pkt_size      = DEFAULT_SIZE;
    int   rate          = DEFAULT_RATE;
    int   duration      = DEFAULT_DURATION;
    long  max_txns      = 0;              /* 0 = use -d instead */
    int   warmup_secs   = DEFAULT_WARMUP;
    int   verbose       = 0;

    int opt;
    while ((opt = getopt(argc, argv, "H:p:s:r:d:n:w:vh")) != -1) {
        switch (opt) {
        case 'H': host        = optarg;        break;
        case 'p': port        = atoi(optarg);  break;
        case 's': pkt_size    = atoi(optarg);  break;
        case 'r': rate        = atoi(optarg);  break;
        case 'd': duration    = atoi(optarg);  break;
        case 'n': max_txns    = atol(optarg);  break;
        case 'w': warmup_secs = atoi(optarg);  break;
        case 'v': verbose     = 1;             break;
        case 'h': usage(argv[0]);              return 0;
        default:  usage(argv[0]);              return 1;
        }
    }

    if (pkt_size <= 0) { fprintf(stderr, "size must be > 0\n");  return 1; }
    if (rate     <= 0) { fprintf(stderr, "rate must be > 0\n");  return 1; }

    signal(SIGINT,  on_sig);
    signal(SIGTERM, on_sig);
    signal(SIGPIPE, SIG_IGN);

    /* ---- connect ---- */
    int fd = tcp_connect(host, port);
    if (fd < 0) return 1;

    printf("connected to %s:%d\n", host, port);
    printf("config: size=%d B  rate=%d TPS  duration=%ds  warmup=%ds\n",
           pkt_size, rate, duration, warmup_secs);

    /* ---- allocate buffers ---- */
    char *sndbuf = malloc((size_t)pkt_size);
    char *rcvbuf = malloc((size_t)pkt_size);
    if (!sndbuf || !rcvbuf) { perror("malloc"); return 1; }
    memset(sndbuf, 0x42, (size_t)pkt_size);   /* fill send buf with pattern */

    /* latency sample array — capped at MAX_SAMPLES */
    long cap = (max_txns > 0) ? max_txns : (long)rate * (duration + 4);
    if (cap < 1024)        cap = 1024;
    if (cap > MAX_SAMPLES) cap = MAX_SAMPLES;
    double *lat = malloc((size_t)cap * sizeof *lat);
    if (!lat) { perror("malloc"); return 1; }

    /* inter-transaction interval in nanoseconds */
    /* inter-transaction interval in nanoseconds */
    long period_ns = (rate > 0) ? (long)(1e9 / (double)rate) : 0;
    
    /* ---- warm-up: send traffic but don't record latencies ---- */
    int ok = 1;
    if (warmup_secs > 0) {
        printf("warming up (%d s)...\n", warmup_secs);
        double wend = mono_sec() + (double)warmup_secs;
        struct timespec nxt = mono_ts();
        while (mono_sec() < wend && !g_stop) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nxt, NULL);
            if (write_all(fd, sndbuf, (size_t)pkt_size) < 0 ||
                read_all (fd, rcvbuf, (size_t)pkt_size) < 0) {
                ok = 0; break;
            }
            ts_add_ns(&nxt, period_ns);
        }
        if (ok) printf("warm-up done — starting measurement\n");
    }

    /* ---- measurement loop ---- */
    long   n       = 0;
    double t_start = mono_sec();
    double t_end   = t_start + (double)duration;
    double prog_at = t_start + 1.0;   /* next 1-second progress report */

    struct timespec nxt = mono_ts();

    while (ok && !g_stop) {
        /* stopping conditions */
        if (max_txns > 0) { if (n >= max_txns)      break; }
        else              { if (mono_sec() >= t_end) break; }
        if (n >= cap)      break;

	/* pace: sleep until the next scheduled slot */
	if (period_ns > 0) {
	  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nxt, NULL);
	  ts_add_ns(&nxt, period_ns);
	}

        /* ---- single transaction ---- */
        double t0 = mono_sec();
        if (write_all(fd, sndbuf, (size_t)pkt_size) < 0) break;
        if (read_all (fd, rcvbuf, (size_t)pkt_size) < 0) break;
        double rtt = mono_sec() - t0;

        lat[n++] = rtt;

        /* output */
        if (verbose) {
            printf("txn %7ld  rtt %8.1f µs\n", n, rtt * 1e6);
        } else {
            double now = mono_sec();
            if (now >= prog_at) {
                double el = now - t_start;
                printf("[%5.1f s]  txns=%-9ld  tps=%8.0f  last_rtt=%7.1f µs\n",
                       el, n, (double)n / el, rtt * 1e6);
                prog_at += 1.0;
            }
        }
    }

    double elapsed = mono_sec() - t_start;
    print_stats(lat, n, elapsed, pkt_size, rate);

    free(sndbuf);
    free(rcvbuf);
    free(lat);
    close(fd);
    return 0;
}
