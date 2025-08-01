% 清除工作区和命令行窗口
clear;
clc;

% --- 配置 ---
% 重要：请将 'your_data_file.csv' 替换为您的实际文件名
filename = 'AmpSampleBoard_OnlyVout.csv'; 
array_name = 'myData1D';                  % 在C代码中生成的数组名
variable_type = 'float';                  % 数组变量类型
elements_per_line = 8;                    % C代码输出中每行元素的数量，用于格式化

% --- 主程序 ---

% 检查文件是否存在
if ~isfile(filename)
    error('文件 "%s" 未找到。请确保文件名正确，且文件与脚本在同一目录中。', filename);
end

% --- 动态查找数据起始行 (这一部分是正确的) ---
fid = fopen(filename, 'rt');
if fid == -1, error('无法打开文件: %s', filename); end

tline = fgetl(fid);
lineNum = 1;
dataStartLine = -1;
while ischar(tline)
    % 查找第一个以 " 1," 或 "1," 开头的行 (Level为1的数据行)
    trimmed_line = strtrim(tline);
    if startsWith(trimmed_line, '1,')
        dataStartLine = lineNum;
        break;
    end
    tline = fgetl(fid);
    lineNum = lineNum + 1;
end
fclose(fid);

if dataStartLine == -1
    error('在文件中未找到有效的数据行 (即 Level 为 1 的行)。');
end

% --- 读取数据 (已修正的部分) ---
% 1. 先从文件中检测导入选项
opts = detectImportOptions(filename);

% 2. 然后修改检测到的选项，指定数据从哪一行开始
opts.DataLines = dataStartLine;

% 3. 为列手动命名，因为我们跳过了文件的原始标题行
opts.VariableNames = {'Level', 'Expression', 'Value', 'Location', 'Refresh'}; 
% 指定 'Value' 列的数据类型为 double，以防被误读为文本
opts = setvartype(opts, 'Value', 'double');

% 4. 使用修改后的选项来读取表格
T = readtable(filename, opts);

% 提取 'Value' 列到一维向量中
dataVector = T.Value;
num_elements = length(dataVector);

% --- 以C语言格式输出到命令行 (这部分是正确的) ---
fprintf('%s %s[%d] = {\n', variable_type, array_name, num_elements);

for i = 1:num_elements
    % 如果是新行的第一个元素，添加缩进
    if mod(i-1, elements_per_line) == 0
        fprintf('    ');
    end
    
    % 打印数值
    fprintf('%.9f', dataVector(i));
    
    % 如果不是数组的最后一个元素，打印逗号
    if i < num_elements
        fprintf(', ');
    end
    
    % 如果当前行已满，或者已是最后一个元素，则换行
    if mod(i, elements_per_line) == 0 || i == num_elements
        fprintf('\n');
    end
end

fprintf('};\n');