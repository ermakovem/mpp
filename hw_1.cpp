#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <parlay/sequence.h>


#define SEQ_SIZE 100'000'000
#define CHUNK_SIZE 1000

int split(parlay::sequence<int>& s, int begin, int end) {
    int pivot = s[end];
    int index = begin - 1;

    for (int current = begin; current < end; current++) {
        if (s[current] <= pivot) {
            index++;
            std::swap(s[index], s[current]);
        }
    }

    std::swap(s[index + 1], s[end]);
    return index + 1;
}

void seq_sort(parlay::sequence<int>& s, int begin, int end) {
    if (begin < end) {
        int idx = split(s, begin, end);
        seq_sort(s, begin, idx - 1);
        seq_sort(s, idx + 1, end);
    }
}

void par_sort(parlay::sequence<int>& s, int begin, int end) {
    if (end - begin < CHUNK_SIZE) {
        seq_sort(s, begin, end);
    } else {
        int idx = split(s, begin, end);
        //par_sort(v, begin, idx - 1);
        //par_sort(v, idx + 1, end);
        parlay::par_do(
            [&]() { par_sort(s, begin, idx - 1); },
            [&]() { par_sort(s, idx + 1, end); }
        );
    }
}

parlay::sequence<int> sequence_generator(int n) {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    auto pseudo_seed = static_cast<unsigned long>(duration.count());
    parlay::random_generator gen(pseudo_seed);
    std::uniform_int_distribution<> dis(-1 * n, n - 1);

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<> dis(-1 * n, n);

    return parlay::tabulate(n, [&](int i) {
        auto r = gen[i];
        return dis(gen);
    });
}


int main() {

    //warm up, without it first par_sort result will not be representative
    //par_sort(sequence_generator(SEQ_SIZE));

    std::vector<parlay::sequence<int>> pregen_seqs;

    for (int i = 0; i < 5; i++) {
        auto seq = sequence_generator(SEQ_SIZE);
        pregen_seqs.push_back(std::move(seq));
    }

    double par_avg_counter = 0;
    std::cout << "Параллельный sort:" << std::endl;
    for (int i = 0; i < pregen_seqs.size(); i++) {
        auto seq = pregen_seqs[i];
        auto start = std::chrono::high_resolution_clock::now();

        par_sort(seq, 0, seq.size() - 1);
        
        auto end = std::chrono::high_resolution_clock::now();
        if (!std::is_sorted(seq.begin(), seq.end())) {
            std::cout << "NOT SORTED!!!" << std::endl;
            return 0;
        }
        std::chrono::duration<double> elapsed = end - start;
        par_avg_counter += elapsed.count();
        std::cout << '\t' << i + 1 << ". Время выполнения: " << elapsed.count() << " секунд" << std::endl;
    }
    std::cout << '\t' << "Среднее время выполнения: " << par_avg_counter / pregen_seqs.size() << std::endl;

    double seq_avg_counter = 0;
    std::cout << "Последовательный sort:" << std::endl;
    for (int i = 0; i < pregen_seqs.size(); i++) {
        auto seq = pregen_seqs[i];
        auto start = std::chrono::high_resolution_clock::now();

        seq_sort(seq, 0, seq.size() - 1);

        auto end = std::chrono::high_resolution_clock::now();
        if (!std::is_sorted(seq.begin(), seq.end())) {
            std::cout << "NOT SORTED!!!" << std::endl;
            return 0;
        }
        std::chrono::duration<double> elapsed = end - start;
        seq_avg_counter += elapsed.count();
        std::cout << '\t' << i + 1 << ". Время выполнения: " << elapsed.count() << " секунд" << std::endl;
    }
    std::cout << '\t' << "Среднее время выполнения: " << seq_avg_counter / pregen_seqs.size() << std::endl;

    std::cout << "Ускорение в " << seq_avg_counter / par_avg_counter << " раз" << std::endl;

    return 0;
}