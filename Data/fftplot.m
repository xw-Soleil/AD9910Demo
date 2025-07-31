% 简单的FFT数据读取和绘图脚本
clear; clc; close all;

% 读取CSV文件
filename = 'adc1.csv';
data = readtable(filename);

% 提取fft_magnitude_spectrum的数值
fft_values = [];
for i = 1:height(data)
    if contains(data.Expression{i}, '[') && contains(data.Expression{i}, ']')
        fft_values(end+1) = data.Value(i);
    end
end

% 绘图
figure;
plot(0:length(fft_values)-1, fft_values, 'b-', 'LineWidth', 1.5);
title('FFT Magnitude Spectrum');
xlabel('Frequency Bin');
ylabel('Magnitude');
grid on;

N = length(fft_values);
fs = 1000000;
bin_spacing = fs / N;
f_signal = 20000; % 信号频率 (Hz)，100.5确保频率在两个bin中间
signal = fft_values;
A_signal = 2.0;  
A_true_volts = A_signal; % 理论真实幅值就是我们设定的A_signal
%% 4. FFT分析与误差补偿

% --- 选择窗函数 ---
% 根据论文Table 2, Flattop窗最适合精确幅值测量
win = flattopwin(N);
% 也可以尝试其他窗，例如 Hann 窗
% win = hann(N);

% --- 查找窗函数对应的补偿值 (根据论文Table 2) ---
% CPG (Coherent Power Gain) 和 Scalloping Loss 以dB为单位给出
window_name = 'Flattop';
CPG_dB = 13.3; 
Scalloping_loss_max_dB = 0.02; % Flattop窗的扇贝损失极小

% 如果使用Hann窗，请使用下面的值
% window_name = 'Hann';
% CPG_dB = 6.02;
% Scalloping_loss_max_dB = 1.42;

% 对信号加窗
signal_windowed = signal' .* win;

% --- 执行FFT并转换为单边幅值谱 (Volts) ---
% 这个过程遵循论文第2页的 "Algorithm 1" 的逻辑，但计算的是幅值而非功率
Y = fft(signal_windowed, N);

% 计算双边幅值谱并归一化
A2 = abs(Y/N); 
% 计算单边幅值谱 (直流分量不变，其余乘以2)
A1 = A2(1:N/2+1);
A1(2:end-1) = 2*A1(2:end-1);

% 创建频率轴
f_axis = fs*(0:(N/2))/N;

%% 5. 提取测量结果并进行补偿

% 找到频谱中的峰值，即信号的显示幅值
[A_displayed_volts, peak_index] = max(A1);
f_measured = f_axis(peak_index);

% --- 应用补偿 ---
% 根据论文公式 4.1: P_true = P_displayed + CPG + Scalloping Loss
% 这个公式是dB域的加法。在幅值的线性域，需要转换为乘法。
% 幅值补偿系数 = 10^((CPG_dB + Scalloping_loss_dB) / 20)
total_loss_dB = CPG_dB + Scalloping_loss_max_dB;
correction_factor = 10^(total_loss_dB / 20);

% 应用补偿
A_corrected_volts = A_displayed_volts * correction_factor;

%% 6. 显示结果

fprintf('==================================================\n');
fprintf('      精确FFT幅值测量结果 (%s 窗)\n', window_name);
fprintf('==================================================\n');
fprintf('理论信号幅值: \t\t%.4f V\n', A_true_volts);
fprintf('--------------------------------------------------\n');
fprintf('FFT直接测量到的峰值幅值: \t%.4f V (在 %.2f Hz)\n', A_displayed_volts, f_measured);
fprintf('  (x) 补偿系数 (来自%.2fdB CPG 和 %.2fdB 扇贝损失): %.4f\n', CPG_dB, Scalloping_loss_max_dB, correction_factor);
fprintf('--------------------------------------------------\n');
fprintf('补偿后的测量幅值: \t\t%.4f V\n', A_corrected_volts);
fprintf('==================================================\n');
fprintf('测量误差: \t\t\t%.4f V\n', A_corrected_volts - A_true_volts);
fprintf('相对误差: \t\t\t%.2f %%\n', (A_corrected_volts - A_true_volts)/A_true_volts * 100);
fprintf('==================================================\n');


%% 7. 绘图
figure('Name', 'FFT 精确幅值测量分析', 'NumberTitle', 'off');
plot(f_axis, A1);
title(['FFT 幅值谱 - ' window_name ' 窗']);
xlabel('频率 (Hz)');
ylabel('幅值 (V)');
grid on;
hold on;

% 标记理论值和测量值
y_limits = ylim;
plot([f_measured f_measured], [y_limits(1) A_displayed_volts], 'r--', 'LineWidth', 1.5, 'DisplayName', 'FFT显示峰值');
plot([0 f_measured], [A_displayed_volts A_displayed_volts], 'r--', 'LineWidth', 1.5);

text(f_measured + 20, A_displayed_volts, sprintf('显示幅值: %.2f V', A_displayed_volts), 'Color', 'red');

plot([0 f_axis(end)], [A_true_volts A_true_volts], 'g-', 'LineWidth', 2, 'DisplayName', '理论真实幅值');
text(f_axis(end)/2, A_true_volts + 0.1, sprintf('理论真实幅值: %.2f V', A_true_volts), 'Color', 'green', 'HorizontalAlignment', 'center');

plot([0 f_axis(end)], [A_corrected_volts A_corrected_volts], 'm--', 'LineWidth', 2, 'DisplayName', '补偿后幅值');
text(f_axis(end)/2, A_corrected_volts - 0.1, sprintf('补偿后幅值: %.2f V', A_corrected_volts), 'Color', 'magenta', 'HorizontalAlignment', 'center');

legend('FFT谱线', 'FFT显示峰值', '理论真实幅值', '补偿后幅值', 'Location', 'southwest');
hold off;
ylim([0 A_true_volts*1.2]); % 调整Y轴范围以便观察



% % 简单的FFT数据读取、滤波和绘图脚本
% clear; clc; close all;

% % ==================== 数据加载或模拟 ====================
% filename = 'huart4.csv';
% if ~exist(filename, 'file')
%     disp("警告: 'huart4.csv' 文件未找到，将使用模拟数据进行演示。");
%     x = 0:1199;
%     base_signal = abs(sin(2 * pi * x / 500)); 
%     noise = 0.1 * randn(size(x)) + 0.2 * sin(2 * pi * x / 10);
%     fft_values = 2.5 * base_signal .* (1 + 0.2 * sin(2 * pi * x / 20)) + noise + 0.1;
%     fft_values(fft_values < 0) = 0;
% else
%     data = readtable(filename);
%     fft_values = [];
%     for i = 1:height(data)
%         if contains(data.Expression{i}, '[') && contains(data.Expression{i}, ']')
%             fft_values(end+1) = data.Value(i);
%         end
%     end
% end
% % =======================================================


% % ==================== FIR 滤波器设计 ====================
% % 参数定义
% Fs_bins = length(fft_values); % 将总点数视为“采样率”
% filterOrder = 50;           % 滤波器阶数。阶数越高，过渡带越窄，但延迟和计算量越大。
%                             % 50是一个不错的起点。
% cutoffFreq_bins = 20;       % 截止频率（以bin为单位）。我们希望滤掉周期小于~50个bin的噪声。
%                             % 截止频率设置为20个bin似乎是合理的。

% % 将截止频率归一化到 [0, 1] 范围，其中1对应Nyquist频率 (Fs_bins/2)
% normalizedCutoff = cutoffFreq_bins / (Fs_bins / 2);

% % 使用 designfilt 设计一个低通FIR滤波器 (推荐的现代方法)
% % 我们使用 'fir1' 风格的窗口法设计，因为它简单且高效
% lpFilt = designfilt('lowpassfir', 'FilterOrder', filterOrder, ...
%          'CutoffFrequency', normalizedCutoff, 'DesignMethod', 'window', ...
%          'Window', 'hamming');

% % 提取滤波器系数(taps)
% fir_coeffs = lpFilt.Coefficients;


% % ==================== 应用滤波器 ====================
% % 使用 'filter' 函数，这与嵌入式系统中的实现方式相同
% filtered_values_fir = filter(fir_coeffs, 1, fft_values);

% % **重要**: FIR滤波器会引入延迟！
% % 延迟等于 filterOrder / 2 个采样点。我们需要补偿这个延迟以便在图中正确对齐。
% group_delay = filterOrder / 2;


% % ==================== 绘图 ====================
% figure('Name', 'FIR Filtering', 'NumberTitle', 'off');
% hold on;

% % 绘制原始数据
% plot(0:length(fft_values)-1, fft_values, 'Color', [0.5 0.5 1], 'DisplayName', 'Original Data');

% % 绘制滤波后的数据 (注意：由于延迟，它会向右平移)
% % 我们只绘制有效部分，即从延迟点开始
% plot((0:length(filtered_values_fir)-1), filtered_values_fir, 'r-', 'LineWidth', 2, 'DisplayName', 'FIR Filtered (with delay)');

% % 绘制延迟补偿后的数据，以便于比较
% plot((0:length(fft_values)-1-group_delay), filtered_values_fir(group_delay+1:end), 'g-', 'LineWidth', 2, 'DisplayName', 'FIR Filtered (Delay Compensated)');

% hold off;
% title('FIR Low-pass Filtering of FFT Spectrum');
% xlabel('Frequency Bin');
% ylabel('Magnitude');
% legend('show', 'Location', 'northwest');
% grid on;
% axis tight;

% % ==================== 导出系数以便移植到 C ====================
% fprintf('// FIR Filter Coefficients for arm_math.h\n');
% fprintf('#define NUM_TAPS      %d\n', length(fir_coeffs));
% fprintf('static const float32_t firCoeffs[NUM_TAPS] = {\n');
% for i = 1:length(fir_coeffs)-1
%     fprintf('    %.8ff, \n', fir_coeffs(i));
% end
% fprintf('    %.8ff\n', fir_coeffs(end));
% fprintf('};\n');