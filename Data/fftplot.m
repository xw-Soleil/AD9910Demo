% 双波形数据读取和绘图脚本
clear; clc; close all;

% 读取第一个CSV文件 - adc_dc
filename1 = 'adc_dc.csv';
data1 = readtable(filename1);

% 提取adc_dc的数值
fft_values1 = [];
for i = 1:height(data1)
    if contains(data1.Expression{i}, '[') && contains(data1.Expression{i}, ']')
        fft_values1(end+1) = data1.Value(i);
    end
end

% 读取第二个CSV文件 - adc
filename2 = 'adc.csv';
data2 = readtable(filename2);

% 提取adc的数值
fft_values2 = [];
for i = 1:height(data2)
    if contains(data2.Expression{i}, '[') && contains(data2.Expression{i}, ']')
        fft_values2(end+1) = data2.Value(i);
    end
end

% 绘制双波形
figure;
hold on;
plot(0:length(fft_values1)-1, fft_values1, 'b-', 'LineWidth', 1.5, 'DisplayName', 'adc_dc');
plot(0:length(fft_values2)-1, fft_values2, 'r-', 'LineWidth', 1.5, 'DisplayName', 'adc');
hold off;

title('双波形对比');
xlabel('采样点');
ylabel('幅值');
legend('show');
grid on;