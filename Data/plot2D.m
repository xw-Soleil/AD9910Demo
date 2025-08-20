% 清除工作区和命令行窗口
clear;
clc;

% --- 配置 ---
filename = 'SweepKnownBoardHsArr.csv'; % CSV文件名
array_name = 'myData';                   % 在C代码中生成的数组名
variable_type = 'float';                 % 数组变量类型

% --- 主程序 ---

% 检查文件是否存在
if ~isfile(filename)
    error('文件 "%s" 未找到。请确保它与此脚本在同一目录中。', filename);
end

% 1. 使用 readtable 读取CSV文件
%    'ReadVariableNames' 设为 true 来读取第一行的列标题
opts = detectImportOptions(filename);
% 假设数据从第3行开始，跳过前2个标题行
opts.DataLines = 3; 
opts.VariableNamesLine = 1;
T = readtable(filename, opts);

% 2. 筛选出包含实际数组数据的行 (假设 'Level' 列为 1)
dataRows = T(T.Level == 1, :);

if isempty(dataRows)
    error('在文件中没有找到Level为1的数据行。');
end

% 3. 解析 'Expression' 列以确定矩阵维度
max_row = -1;
max_col = -1;
for i = 1:height(dataRows)
    % 使用正则表达式从 '[r][c]' 格式中提取数字
    tokens = regexp(dataRows.Expression{i}, '\[(\d+)\]\[(\d+)\]', 'tokens');
    if ~isempty(tokens)
        r = str2double(tokens{1}{1});
        c = str2double(tokens{1}{2});
        if r > max_row, max_row = r; end
        if c > max_col, max_col = c; end
    end
end

% C语言数组是0索引，所以维度是最大索引+1
num_rows = max_row + 1;
num_cols = max_col + 1;

% 4. 创建一个Matlab矩阵并用零填充
dataMatrix = zeros(num_rows, num_cols);

% 5. 填充矩阵
for i = 1:height(dataRows)
    tokens = regexp(dataRows.Expression{i}, '\[(\d+)\]\[(\d+)\]', 'tokens');
    if ~isempty(tokens)
        % Matlab是1索引，所以需要+1
        r_idx = str2double(tokens{1}{1}) + 1;
        c_idx = str2double(tokens{1}{2}) + 1;
        
        dataMatrix(r_idx, c_idx) = dataRows.Value(i);
    end
end

% --- 6. 以C语言格式输出到命令行 ---
fprintf('%s %s[%d][%d] = {\n', variable_type, array_name, num_rows, num_cols);

for i = 1:num_rows
    fprintf('    {');
    for j = 1:num_cols
        fprintf('%.9f', dataMatrix(i, j));
        if j < num_cols
            fprintf(', '); % 在元素之间添加逗号和空格
        end
    end
    fprintf('}');
    if i < num_rows
        fprintf(',\n'); % 在行之间添加逗号和换行
    else
        fprintf('\n');  % 最后一行后只换行
    end
end

fprintf('};\n');