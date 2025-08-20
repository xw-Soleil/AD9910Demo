%% MATLAB Code for a VERTICAL and ADAPTIVE Block Diagram
%  Version 3.3: Final version with a surrounding frame and a main title.

% --- 1. Initialization ---
clc;            % Clear Command Window
clear;          % Clear Workspace
close all;      % Close all figure windows

% --- 2. Setup Figure, Axes, and Labels ---
figure('Color', 'w');
ax = gca;
ax.Visible = 'off';
xlim([0, 1]);
ylim([0, 1]);
hold on;

% Define the text for each box
labels = {'探究装置', '已知模型电路', '示波器'};

% --- 3. Define Style and Layout Parameters ---
font_name = 'SimSun';
font_size_points = 14;
font_weight = 'bold';
box_height = 0.12;
vertical_spacing = 0.08;
horizontal_padding = 0.04;
y_start = 0.85; % Adjusted y_start to make space for the title
x_center = 0.5;

% Color Palette
color_matrix = [70, 130, 180; 244, 164, 96; 211, 211, 211] / 255;
shadow_color = [0, 0, 0, 0.2];
border_color = [0.2, 0.2, 0.2];
text_color = [0.1, 0.1, 0.1];

% --- 4. Calculate Adaptive Box Widths ---
box_widths = zeros(1, length(labels));
ax.FontSize = font_size_points; % Match axes font size for accurate measurement
for i = 1:length(labels)
    temp_text = text(0, 0, labels{i}, 'FontName', font_name, ...
                     'FontSize', font_size_points, 'FontWeight', font_weight, 'Visible', 'off');
    text_width = temp_text.Extent(3);
    box_widths(i) = text_width + horizontal_padding;
    delete(temp_text);
end

% --- 5. Draw the Blocks and Text ---
for i = 1:length(labels)
    current_box_width = box_widths(i);
    current_x = x_center - current_box_width / 2;
    y_top = y_start - (i-1) * (box_height + vertical_spacing);
    current_y_bottom = y_top - box_height;
    
    shadow_pos = [current_x - 0.005, current_y_bottom - 0.005, current_box_width, box_height];
    annotation('rectangle', shadow_pos, 'FaceColor', shadow_color, 'EdgeColor', 'none');

    box_pos = [current_x, current_y_bottom, current_box_width, box_height];
    annotation('rectangle', box_pos, 'FaceColor', color_matrix(i,:), ...
               'EdgeColor', border_color, 'LineWidth', 1.5);

    annotation('textbox', box_pos, 'String', labels{i}, 'Color', text_color, ...
               'FontName', font_name, 'FontSize', font_size_points, 'FontWeight', font_weight, ...
               'VerticalAlignment', 'middle', 'HorizontalAlignment', 'center', ...
               'LineStyle', 'none');
end

% --- 6. Draw the Connecting Arrows ---
for i = 1:length(labels) - 1
    y_arrow_start = y_start - (i-1)*(box_height + vertical_spacing) - box_height;
    y_arrow_end = y_arrow_start - vertical_spacing;
    
    annotation('arrow', [x_center, x_center], [y_arrow_start, y_arrow_end], ...
               'Color', border_color, 'LineWidth', 1.5, 'HeadStyle', 'plain', ...
               'HeadWidth', 10, 'HeadLength', 10);
end

% --- 7. ★★★ NEW: Draw Surrounding Frame and Title ★★★ ---
% a. Define Frame and Title Properties
frame_color = [0, 114, 189] / 255; % A nice "MATLAB Blue"
frame_padding = 0.05;
title_text = '探究装置';
title_font_size = 16;

% b. Calculate the bounding box of the entire diagram
max_width = max(box_widths);
total_height = length(labels) * box_height + (length(labels) - 1) * vertical_spacing;
diagram_left = x_center - max_width / 2;
diagram_bottom = y_start - total_height;

% c. Draw the blue dashed frame
frame_pos = [diagram_left - frame_padding, ...
             diagram_bottom - frame_padding, ...
             max_width + 2 * frame_padding, ...
             total_height + 2 * frame_padding];
annotation('rectangle', frame_pos, ...
           'EdgeColor', frame_color, ...
           'LineStyle', '--', ...
           'LineWidth', 2, ...
           'FaceColor', 'none'); % No fill color

% d. Add the Title
title_y_pos = frame_pos(2) + frame_pos(4) + 0.01; % Position it just above the frame
title_pos = [0, title_y_pos, 1, 0.05]; % Full width textbox for easy centering
annotation('textbox', title_pos, ...
           'String', title_text, ...
           'FontName', font_name, ...
           'FontSize', title_font_size, ...
           'FontWeight', font_weight, ...
           'Color', border_color, ...
           'HorizontalAlignment', 'center', ...
           'VerticalAlignment', 'bottom', ...
           'LineStyle', 'none');

% --- 8. Final Adjustments ---
axis equal;
hold off;

% --- 9. Saving the Figure ---
% print('Final_Diagram_with_Title', '-dpng', '-r300');