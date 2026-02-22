#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

// Datos
struct Job {
    int id = 0;
    int value = 0;
    bool is_end = false;
    Clock::time_point t_generated{};
};

struct Result {
    int id = 0;
    int input = 0;
    long long output = 0;
    bool is_end = false;
    Clock::time_point t_generated{};
    Clock::time_point t_processed{};
    Clock::time_point t_written{};
};

// Métricas de cola (tiempo bloqueado)
struct QueueMetrics {
    long long wait_push_ms = 0; // esperando porque estaba llena
    long long wait_pop_ms  = 0; // esperando porque estaba vacía
};

// Cola acotada (buffer limitado)

// implementación de un buffer acotado
// si el buffer está lleno, push() bloquea
// si el buffer está vacío, pop() bloquea
// la sincronización se maneja con variables de condición

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity, QueueMetrics* mtr = nullptr)
        : cap(capacity), metrics(mtr) {}

    void push(const T& item) {
        std::unique_lock<std::mutex> lock(m);

        auto t0 = Clock::now();
        not_full.wait(lock, [&]{ return q.size() < cap; });
        auto t1 = Clock::now();

        if (metrics) {
            metrics->wait_push_ms += std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        }

        q.push(item);
        not_empty.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m);

        auto t0 = Clock::now();
        not_empty.wait(lock, [&]{ return !q.empty(); });
        auto t1 = Clock::now();

        if (metrics) {
            metrics->wait_pop_ms += std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        }

        T item = q.front();
        q.pop();
        not_full.notify_one();
        return item;
    }

private:
    size_t cap;
    QueueMetrics* metrics;
    std::mutex m;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    std::queue<T> q;
};

//Configuración
struct Config {
    int N = 1500;          
    size_t cap1 = 32;
    size_t cap2 = 32;
    int gen_ms = 0;         
    int proc_ms = 2;
    int write_ms = 0;
    unsigned seed = 42;     // comparación justa seq vs conc
    std::string out = "output.csv";
};

// Productor

// hilo generador:
// produce trabajos y los inserta en el primer buffer
// se bloquea si el buffer está lleno

void producer(BoundedQueue<Job>& q_in, const Config& cfg) {
    std::mt19937 rng(cfg.seed);
    std::uniform_int_distribution<int> dist(1, 1000);

    for (int i = 1; i <= cfg.N; ++i) {
        Job j;
        j.id = i;
        j.value = dist(rng);
        j.is_end = false;
        j.t_generated = Clock::now();

        q_in.push(j);

        if (cfg.gen_ms > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.gen_ms));
    }

    Job end;
    end.is_end = true;
    end.t_generated = Clock::now();
    q_in.push(end);
}

//Procesador

// hilo procesador:
// consume del primer buffer, procesa el dato  y lo inserta en el segundo buffer
// puede bloquearse si alguno de los buffers está saturado

void processor(BoundedQueue<Job>& q_in, BoundedQueue<Result>& q_out, const Config& cfg) {
    while (true) {
        Job j = q_in.pop();

        if (j.is_end) {
            Result end;
            end.is_end = true;
            end.t_generated = j.t_generated;
            end.t_processed = Clock::now();
            q_out.push(end);
            break;
        }

        Result r;
        r.id = j.id;
        r.input = j.value;
        r.output = 1LL * j.value * j.value;
        r.is_end = false;
        r.t_generated = j.t_generated;
        r.t_processed = Clock::now();

        q_out.push(r);

        if (cfg.proc_ms > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.proc_ms));
    }
}

//Escritor

// hilo escritor:
// consume resultados del segundo buffer y los guarda en archivo
// también mide la latencia total del sistema

void writer(BoundedQueue<Result>& q_out, const Config& cfg,
            std::vector<long long>& lat_gen_to_write_ms) {
    std::ofstream out(cfg.out);
    out << "id,input,output,lat_gen_to_write_ms\n";

    while (true) {
        Result r = q_out.pop();

        if (r.is_end) break;

        if (cfg.write_ms > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.write_ms));

        r.t_written = Clock::now();

        auto lat = std::chrono::duration_cast<std::chrono::milliseconds>(r.t_written - r.t_generated).count();
        lat_gen_to_write_ms.push_back(lat);

        out << r.id << "," << r.input << "," << r.output << "," << lat << "\n";
    }
}

// Versión SECUENCIAL (baseline)
long long run_sequential(const Config& cfg, std::vector<long long>& lat_ms, const std::string& out_name) {
    std::mt19937 rng(cfg.seed);
    std::uniform_int_distribution<int> dist(1, 1000);

    std::ofstream out(out_name);
    out << "id,input,output,lat_gen_to_write_ms\n";

    auto t0 = Clock::now();

    for (int i = 1; i <= cfg.N; ++i) {
        auto tg = Clock::now();
        int v = dist(rng);

        if (cfg.gen_ms > 0)  std::this_thread::sleep_for(std::chrono::milliseconds(cfg.gen_ms));

        long long res = 1LL * v * v;

        if (cfg.proc_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(cfg.proc_ms));
        if (cfg.write_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(cfg.write_ms));

        auto tw = Clock::now();
        auto lat = std::chrono::duration_cast<std::chrono::milliseconds>(tw - tg).count();
        lat_ms.push_back(lat);

        out << i << "," << v << "," << res << "," << lat << "\n";
    }

    auto t1 = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

//Estadísticas
struct Stats {
    long long total_ms = 0;
    double throughput_jobs_per_s = 0.0;
    double avg_latency_ms = 0.0;
    long long p95_latency_ms = 0;
};

Stats compute_stats(int N, long long total_ms, std::vector<long long>& lat) {
    Stats s;
    s.total_ms = total_ms;
    s.throughput_jobs_per_s = (total_ms > 0) ? (1000.0 * N / (double)total_ms) : 0.0;

    long long sum = 0;
    for (auto x : lat) sum += x;
    s.avg_latency_ms = lat.empty() ? 0.0 : (double)sum / (double)lat.size();

    std::sort(lat.begin(), lat.end());
    if (!lat.empty()) {
        size_t idx = (size_t)std::floor(0.95 * (lat.size() - 1));
        s.p95_latency_ms = lat[idx];
    }
    return s;
}

// Corre concurrencia y devuelve stats + métricas de espera por cola
struct ConcurrentRun {
    Stats stats;
    QueueMetrics in_m;
    QueueMetrics out_m;
};

ConcurrentRun run_concurrent(const Config& cfg) {
    ConcurrentRun run;

    BoundedQueue<Job> q_in(cfg.cap1, &run.in_m);
    BoundedQueue<Result> q_out(cfg.cap2, &run.out_m);

    std::vector<long long> lat_conc;
    lat_conc.reserve(cfg.N);

    auto t0 = Clock::now();
    std::thread tA(producer, std::ref(q_in), std::cref(cfg));
    std::thread tB(processor, std::ref(q_in), std::ref(q_out), std::cref(cfg));
    std::thread tC(writer, std::ref(q_out), std::cref(cfg), std::ref(lat_conc));
    tA.join(); tB.join(); tC.join();
    auto t1 = Clock::now();

    long long conc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    run.stats = compute_stats(cfg.N, conc_ms, lat_conc);
    return run;
}

Stats run_sequential_stats(const Config& cfg, const std::string& out_name) {
    std::vector<long long> lat_seq;
    long long seq_ms = run_sequential(cfg, lat_seq, out_name);
    return compute_stats(cfg.N, seq_ms, lat_seq);
}

// MAIN: 4 experimentos
int main() {
    std::vector<std::pair<std::string, Config>> cases;

    // Caso 1: Balanced (rápido)
    {
        Config c;
        c.N = 2000;
        c.cap1 = 64; c.cap2 = 64;
        c.gen_ms = 1; c.proc_ms = 2; c.write_ms = 1;
        c.out = "output_balanced.csv";
        cases.push_back({"Balanced", c});
    }

    // Caso 2: Processor bottleneck (rápido)
    {
        Config c;
        c.N = 1500;
        c.cap1 = 16; c.cap2 = 16;
        c.gen_ms = 0; c.proc_ms = 6; c.write_ms = 0;
        c.out = "output_proc_slow.csv";
        cases.push_back({"Proc bottleneck", c});
    }

    // Caso 3: Writer bottleneck + buffers pequeños (backpressure visible)
    {
        Config c;
        c.N = 1500;
        c.cap1 = 8; c.cap2 = 8;
        c.gen_ms = 0; c.proc_ms = 2; c.write_ms = 8;
        c.out = "output_writer_slow.csv";
        cases.push_back({"Writer bottleneck", c});
    }

    // Caso 4: Tiny buffers (latencia baja, más coordinación)
    {
        Config c;
        c.N = 1500;
        c.cap1 = 2; c.cap2 = 2;
        c.gen_ms = 0; c.proc_ms = 2; c.write_ms = 0;
        c.out = "output_tiny_buffers.csv";
        cases.push_back({"Tiny buffers", c});
    }

    std::cout << "\n=== PIPELINE CONCURRENT EXPERIMENTS  ===\n";
    std::cout << "Misma semilla para comparación justa (seed=42)\n\n";

    std::cout << std::left
              << std::setw(18) << "Caso"
              << std::setw(8)  << "N"
              << std::setw(8)  << "cap1"
              << std::setw(8)  << "cap2"
              << std::setw(8)  << "gen"
              << std::setw(8)  << "proc"
              << std::setw(8)  << "write"
              << std::setw(10) << "Seq(ms)"
              << std::setw(10) << "Conc(ms)"
              << std::setw(9)  << "Speedup"
              << std::setw(12) << "Conc thr"
              << std::setw(8)  << "p95"
              << std::setw(12) << "INpushWait"
              << std::setw(12) << "OUTpushWait"
              << "\n";

    std::cout << std::string(125, '-') << "\n";

    int i = 1;
    for (auto& entry : cases) {
        const std::string& name = entry.first;
        const Config& cfg = entry.second;

        // secuencial por caso para comparación justa (archivo distinto)
        Stats seq = run_sequential_stats(cfg, "output_seq_case" + std::to_string(i) + ".csv");

        // concurrente + waits
        ConcurrentRun conc = run_concurrent(cfg);

        double speedup = (conc.stats.total_ms > 0) ? ((double)seq.total_ms / (double)conc.stats.total_ms) : 0.0;

        std::cout << std::left
                  << std::setw(18) << name
                  << std::setw(8)  << cfg.N
                  << std::setw(8)  << cfg.cap1
                  << std::setw(8)  << cfg.cap2
                  << std::setw(8)  << cfg.gen_ms
                  << std::setw(8)  << cfg.proc_ms
                  << std::setw(8)  << cfg.write_ms
                  << std::setw(10) << seq.total_ms
                  << std::setw(10) << conc.stats.total_ms
                  << std::setw(9)  << std::fixed << std::setprecision(2) << speedup
                  << std::setw(12) << std::fixed << std::setprecision(2) << conc.stats.throughput_jobs_per_s
                  << std::setw(8)  << conc.stats.p95_latency_ms
                  << std::setw(12) << conc.in_m.wait_push_ms
                  << std::setw(12) << conc.out_m.wait_push_ms
                  << "\n";

        i++;
    }

    std::cout << "\nCSV por caso: output_*.csv y output_seq_case*.csv\n";
    return 0;
}

