% =============================================================
% ==     绘制带通滤波器频率响应图 (Bode Plot)              ==
% =============================================================
clear; clc; close all;

% --- 1. 从你的结果中提取拟合参数 ---
k  = 0.9693;         % 增益 (Gain)
f0 = 14820.58 ;          % 中心频率 (Center Frequency in Hz)
Q  = 0.1364;         % Q因子 (Q-Factor)
SSE = 3.395909e-01;  % 残差平方和 (Final SSE)

% --- 2. 计算滤波器传递函数的参数 ---
% 传递函数 H(s) = k * ( (w0/Q)*s ) / ( s^2 + (w0/Q)*s + w0^2 )
w0 = 2 * pi * f0; % 将中心频率从 Hz 转换为 rad/s

% --- 3. 创建频率轴用于绘图 ---
% 由于Q值非常小，带宽极宽，我们需要一个很宽的对数频率范围来观察其特性。
f = logspace(-2, 6, 8000); % 从 0.01 Hz 到 1,000,000 Hz (1 MHz) 的对数间隔点
w = 2 * pi * f;           % 转换为 rad/s
s = 1i * w;               % 复频率 s = jw

% --- 4. 计算传递函数 H(s) ---
numerator = k * (w0/Q) * s;
denominator = s.^2 + (w0/Q) * s + w0^2;
H = numerator ./ denominator;

% --- 5. 计算幅值(dB)和相位(度) ---
magnitude_dB = 20 * log10(abs(H));
phase_deg = angle(H) * 180 / pi;

% --- 6. 绘图 ---
figure('Name', 'Fitted Band-Pass Filter Frequency Response', 'NumberTitle', 'off');

% 幅频响应图
subplot(2, 1, 1); % 创建一个2x1的图窗，并激活第1个
semilogx(f, magnitude_dB, 'b-', 'LineWidth', 2);
title({['Fitted Band-Pass Filter: Magnitude Response'], ...
       ['(k = ' num2str(k) ', f_0 = ' num2str(f0) ' Hz, Q = ' num2str(Q) ', SSE = ' num2str(SSE) ')']});
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
grid on;
hold on;

% 找到并标记峰值
peak_gain_dB = 20*log10(k);
plot([f0, f0], [min(magnitude_dB), peak_gain_dB], 'r--', 'DisplayName', ['Peak at f_0 = ' num2str(f0) ' Hz']);
text(f0, peak_gain_dB, sprintf('  Peak Gain: %.2f dB', peak_gain_dB), 'VerticalAlignment', 'bottom');
hold off;
axis tight; % 自动调整坐标轴

% 相频响应图
subplot(2, 1, 2); % 激活第2个图窗
semilogx(f, phase_deg, 'g-', 'LineWidth', 2);
title('Phase Response');
xlabel('Frequency (Hz)');
ylabel('Phase (degrees)');
grid on;
hold on;

% 标记中心频率处的相位
plot([f0, f0], [-90, 90], 'r--', 'DisplayName', ['Phase at f_0 is 0°']);
text(f0, 0, '  Phase = 0°', 'VerticalAlignment', 'bottom');
hold off;
ylim([-100, 100]); % 设置合理的相位范围
axis tight;