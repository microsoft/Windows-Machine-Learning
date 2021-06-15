#pragma once
#ifndef MELSPECTROGRAM_H
#define MELSPECTROGRAM_H

class MelSpectrogram {
public:
    static void MelSpectrogramOnThreeToneSignal(
        size_t batch_size, size_t signal_size, size_t window_size, size_t dft_size,
        size_t hop_size, size_t n_mel_bins, size_t sampling_rate);

    static void MelSpectrogramOnFile(const char* filename,
        size_t batch_size, size_t window_size, size_t dft_size,
        size_t hop_size, size_t n_mel_bins, size_t sampling_rate);
};
#endif