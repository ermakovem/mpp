#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <parlay/sequence.h>

#define SEQ_SIZE 100'000'000
#define processors 4


void seq_sort(parlay::sequence<int>& input, int left, int right) {
    // if (left < right) {
    //     std::sort(input.begin() + left, input.begin() + right + 1);
    // }
    int i = left;
    int j = right;
    int pivot = input[(left + right) / 2];

    // Разделение на подмассивы
    while (i <= j) {
        while (input[i] < pivot)
            i++;
        while (input[j] > pivot)
            j--;
        if (i <= j) {
            std::swap(input[i], input[j]);
            i++;
            j--;
        }
    }

    if (left < j)
        seq_sort(input, left, j);
    if (i < right)
        seq_sort(input, i, right);
}

parlay::sequence<int> merge_two(const parlay::sequence<int>& a, const parlay::sequence<int>& b) {
    parlay::sequence<int> merged(a.size() + b.size());
    size_t idx = 0, jdx = 0, kdx = 0;

    while(idx < a.size() && jdx < b.size()){
        if(a[idx] < b[jdx]){
            merged[kdx++] = a[idx++];
        }
        else{
            merged[kdx++] = b[jdx++];
        }
    }

    while(idx < a.size()){
        merged[kdx++] = a[idx++];
    }
    while(jdx < b.size()){
        merged[kdx++] = b[jdx++];
    }

    return merged;
}

parlay::sequence<int> parallel_merge(parlay::sequence<parlay::sequence<int>>& chunks) {
    if(chunks.empty()) return parlay::sequence<int>();
    while(chunks.size() > 1){
        size_t new_size = (chunks.size() + 1) / 2;
        parlay::sequence<parlay::sequence<int>> new_chunks(new_size);

        parlay::parallel_for(0, new_size, [&](size_t i){
            size_t first = 2 * i;
            size_t second = 2 * i + 1;
            if(second < chunks.size()){
                new_chunks[i] = merge_two(chunks[first], chunks[second]);
            }
            else{
                new_chunks[i] = chunks[first];
            }
        });

        chunks = std::move(new_chunks);
    }
    return chunks[0];
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

parlay::sequence<parlay::sequence<int>> split_sequence(const parlay::sequence<int>& input, size_t max_chunk_size = 1000) {
    size_t n = input.size();
    if (n == 0) return parlay::sequence<parlay::sequence<int>>();

    size_t num_chunks = (n + max_chunk_size - 1) / max_chunk_size;

    parlay::sequence<parlay::sequence<int>> output(num_chunks);

    parlay::parallel_for(0, num_chunks, [&](size_t i) {
        size_t start = i * max_chunk_size;
        size_t end = std::min(start + max_chunk_size, n);
        output[i] = parlay::sequence<int>(&input[start], &input[end]);
    });

    return output;
}

parlay::sequence<int> par_sort(const parlay::sequence<int>& input) {
    parlay::sequence<parlay::sequence<int>> chunks = split_sequence(input, 1000);

    parlay::parallel_for(0, chunks.size(), [&](size_t i) {
        seq_sort(chunks[i], 0, chunks[i].size() - 1);
    });

    parlay::sequence<int> sorted = parallel_merge(chunks);

    return sorted;
}


int main() {

    //warm up, without it first par_sort result will not be representative
    par_sort(sequence_generator(SEQ_SIZE));

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

        par_sort(seq);
        
        auto end = std::chrono::high_resolution_clock::now();
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
        std::chrono::duration<double> elapsed = end - start;
        seq_avg_counter += elapsed.count();
        std::cout << '\t' << i + 1 << ". Время выполнения: " << elapsed.count() << " секунд" << std::endl;
    }
    std::cout << '\t' << "Среднее время выполнения: " << seq_avg_counter / pregen_seqs.size() << std::endl;

    std::cout << "Ускорение в " << seq_avg_counter / par_avg_counter << " раз" << std::endl;

    return 0;
}
