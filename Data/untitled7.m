%% MATLAB Code for Generating a Publication-Quality Plot of MCU-Controlled DDS Output
%  Version 3: Using a different color for each signal segment based on an
%  optimized, high-contrast, and professional palette.

% --- 1. Script Initialization & Parameter Definition ---
clc; clear; close all;

fs = 1e6; ts = 1/fs;
t_end = 3e-3; t = 0:ts:t_end;

f1 = 10e3; A1 = 1.0; % Segment 1
f2 = 25e3; A2 = 0.5; % Segment 2
f3 = 5e3;  A3 = 1.5; % Segment 3

% --- 2. Generate Full, Phase-Continuous Waveform ---
y = zeros(size(t));
phase = 0;
for i = 1:length(t)
    if t(i) < 1e-3
        current_f = f1; current_A = A1;
    elseif t(i) < 2e-3
        current_f = f2; current_A = A2;
    else
        current_f = f3; current_A = A3;
    end
    phase = phase + (2 * pi * current_f / fs);
    y(i) = current_A * sin(phase);
end

% --- 3. Define the Optimized Professional Color Palette ---
% Hex codes are converted to MATLAB RGB triplets (values from 0 to 1).
% ★★★ KEY CHANGE: Using a professional, high-contrast color palette.
color1 = [230, 159, 0] / 255;   % A clear, professional Orange/Amber (replaces #f9f7f3)
color2 = [181, 226, 250] / 255; % User's choice: #b5e2fa -> A light, airy blue
color3 = [15, 163, 177] / 255;  % User's choice: #0fa3b1 -> A strong teal/cyan
color_aux = [0.5, 0.5, 0.5];    % Neutral grey for auxiliary lines

% --- 4. Plotting: Each Segment with its Own Color ---
figure('Color', 'w');
hold on; % Use 'hold on' to plot multiple lines on the same axes

% To plot in segments, we must find the indices for each part.
idx1 = find(t < 1e-3);
% The last point of segment 1 must connect to the first of segment 2
idx2 = find(t >= 1e-3 & t < 2e-3);
idx2 = [idx1(end), idx2]; % This ensures a continuous line without gaps
idx3 = find(t >= 2e-3);
idx3 = [idx2(end), idx3]; % Same here

% ★★★ KEY TECHNIQUE: Plot each segment separately
plot(t(idx1) * 1e3, y(idx1), 'Color', color1, 'LineWidth', 1.5);
plot(t(idx2) * 1e3, y(idx2), 'Color', color2, 'LineWidth', 1.5);
plot(t(idx3) * 1e3, y(idx3), 'Color', color3, 'LineWidth', 1.5);

% Add auxiliary vertical lines
plot([1, 1], [-2, 2], 'Color', color_aux, 'LineStyle', '--', 'LineWidth', 1);
plot([2, 2], [-2, 2], 'Color', color_aux, 'LineStyle', '--', 'LineWidth', 1);

% --- 5. Customization for Publication Quality ---
ax = gca;
title('MCU Controlled DDS Output Signal', 'FontSize', 14, 'FontWeight', 'bold', 'FontName', 'Arial');
xlabel('Time (ms)', 'FontSize', 12, 'FontWeight', 'bold', 'FontName', 'Arial');
ylabel('Amplitude (V)', 'FontSize', 12, 'FontWeight', 'bold', 'FontName', 'Arial');

ax.Box = 'on';
ax.LineWidth = 1.2;
ax.FontName = 'Arial';
ax.FontSize = 11;
grid on;
ax.GridLineStyle = ':';
ax.GridColor = color_aux;
ax.GridAlpha = 0.5;

xlim([0, t_end * 1e3]);
ylim([-2, 2]);

% Annotations using colored text to match the segments
text(0.2, 1.7, 'f=10kHz, A=1.0V', 'FontSize', 11, 'FontName', 'Arial', 'Color', color1, 'FontWeight', 'bold');
text(1.2, 0.7, 'f=25kHz, A=0.5V', 'FontSize', 11, 'FontName', 'Arial', 'Color', 'k'); % Black text for light blue BG
text(2.2, -1.8, 'f=5kHz, A=1.5V', 'FontSize', 11, 'FontName', 'Arial', 'Color', color3, 'FontWeight', 'bold');

hold off; % Release the plot

% --- 6. Saving the Figure ---
% print('DDS_Output_Plot_MultiColor', '-dpng', '-r300');