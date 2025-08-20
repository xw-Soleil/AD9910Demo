% MATLAB脚本：FFT精确测量与精度分析 (最终修正版)
% 修复了核心的功率计算错误，并采用文档推荐的最精确方法 (Flattop窗)

clear; clc; close all;

%% 1. 信号和系统参数定义
% =========================================================================
fs = 10e6;              % 采样率 (10 MHz)
N = 4096;               % FFT点数
R = 50;                 % 系统阻抗 (50 Ohm)
f_signal = 1.253e6;     % 信号频率 (特意设置在格点之间)
A_peak = 1.5;           % 信号的真实峰值振幅 (1.5 V)
noise_power_dbm = -30;  % 真实的平均噪声功率 (-30 dBm)

%% 2. 计算信号的"真实"理论值 (Ground Truth)
% =========================================================================
V_rms_true = A_peak / sqrt(2);
signal_power_watts_true = V_rms_true^2 / R;
signal_power_dbm_true = 10 * log10(signal_power_watts_true / 0.001);

fprintf('------------------- GROUND TRUTH -------------------\n');
fprintf('真实信号峰值振幅: %.4f V\n', A_peak);
fprintf('真实信号功率:       %.4f dBm\n', signal_power_dbm_true);
fprintf('真实噪声功率:       %.4f dBm\n', noise_power_dbm);
fprintf('----------------------------------------------------\n\n');

%% 3. 生成时域信号
% =========================================================================
t = (0:N-1) / fs;
signal_pure = A_peak * sin(2 * pi * f_signal * t);
noise_power_watts = (10^(noise_power_dbm / 10)) / 1000;
noise_std_dev = sqrt(noise_power_watts * R);
noise = noise_std_dev * randn(1, N);
time_signal = signal_pure + noise;

%% 4. FFT分析与精度校正
% =========================================================================
% **采用文档推荐的最精确振幅测量方法：使用Flattop窗**
window_type = 'flattop'; 

fprintf('========= 分析开始 (使用 %s 窗) =========\n', upper(window_type));

switch lower(window_type)
    case 'rectangle', win = rectwin(N)'; CPG_dB = 0; ENBW_corr_dB = 0;
    case 'hann', win = hann(N)'; CPG_dB = 6.02; ENBW_corr_dB = 1.76;
    case 'flattop', win = flattopwin(N)'; CPG_dB = 13.3; ENBW_corr_dB = 5.76;
    otherwise, error('未知的窗函数类型');
end

signal_windowed = time_signal .* win;
Y = fft(signal_windowed, N);

% 核心修正点 1: 正确理解FFT输出为“峰值振幅谱”
P2 = abs(Y / N);
P1_peak_amp = P2(1:N/2+1); % 变量改名以明确其物理意义
P1_peak_amp(2:end-1) = 2 * P1_peak_amp(2:end-1);

% 核心修正点 2: 正确的功率计算公式 P = V_rms^2 / R = (A_peak/√2)^2 / R
power_spectrum_watts = (P1_peak_amp.^2) / (2 * R);
power_spectrum_dbm = 10 * log10(power_spectrum_watts / 0.001);
freq_axis = fs * (0:(N/2)) / N;

% --- 从频谱图中"读取"显示值 ---
[P_displayed_dbm, index_peak] = max(power_spectrum_dbm);
noise_bins = true(size(power_spectrum_dbm));
noise_bins(max(1, index_peak-10):min(length(power_spectrum_dbm), index_peak+10)) = false;
P_floor_dbm = 10*log10(mean(power_spectrum_watts(noise_bins)) / 0.001);

fprintf('\n--- 步骤1: 从FFT频谱图直接读取 (未校正) ---\n');
fprintf('显示的信号峰值功率: %.4f dBm\n', P_displayed_dbm);
fprintf('显示的噪声基底:   %.4f dBm\n', P_floor_dbm);

% --- 应用校正 ---
% 校正信号功率: 使用最简单的公式 P_true = P_displayed + CPG
% (因为Flattop窗的扇形损失可以忽略)
P_signal_corrected_dbm = P_displayed_dbm + CPG_dB;

% 校正噪声功率 (逻辑不变)
PG_dB = 10 * log10(N / 2);
P_noise_corrected_dbm = P_floor_dbm + CPG_dB + PG_dB - ENBW_corr_dB;

% --- 从校正后的功率反推振幅 ---
P_signal_corrected_watts = (10^(P_signal_corrected_dbm / 10)) / 1000;
V_rms_corrected = sqrt(P_signal_corrected_watts * R);
A_peak_corrected = V_rms_corrected * sqrt(2);

%% 5. 显示最终结果并与真实值对比
% =========================================================================
fprintf('\n--- 步骤2: 最终计算结果 (已校正) ---\n');
fprintf('校正后的信号功率:   %.4f dBm (真实值: %.4f dBm)\n', P_signal_corrected_dbm, signal_power_dbm_true);
fprintf('校正后的信号振幅:   %.4f V   (真实值: %.4f V)\n', A_peak_corrected, A_peak);
fprintf('校正后的噪声功率:   %.4f dBm (真实值: %.4f dBm)\n', P_noise_corrected_dbm, noise_power_dbm);
fprintf('====================================================\n');

error_power_db = abs(P_signal_corrected_dbm - signal_power_dbm_true);
error_amplitude_percent = (abs(A_peak_corrected - A_peak) / A_peak) * 100;
fprintf('功率测量误差: %.4f dB, 振幅测量误差: %.2f%%\n', error_power_db, error_amplitude_percent);

%% 6. 可视化
% =========================================================================
figure('Name', ['FFT Analysis with ' upper(window_type) ' Window (Final Corrected)'], 'Position', [100, 100, 900, 600]);
plot(freq_axis / 1e6, power_spectrum_dbm, 'b');
grid on; hold on;
title(['FFT功率谱 (使用' upper(window_type) '窗) - 最终修正算法']);
xlabel('频率 (MHz)'); ylabel('功率 (dBm)');
xlim([max(0, f_signal/1e6-1), f_signal/1e6+1]);