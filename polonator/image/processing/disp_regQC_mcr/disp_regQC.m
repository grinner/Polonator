%% =============================================================================
%% 
%% Polonator G.007 Image Processing Software
%%
%% disp_regQC.mtemplate: template .m file to graph output from image alignment
%% algorithm; do not execute this directly -- this file is used by disp_regQC.pl 
%% to generate disp_regQC.m; execute disp_regQC.pl instead
%% 
%% Church Lab, Harvard Medical School
%% Written by Greg Porreca
%%
%% Release 1.0 -- 04-25-2008
%%
%% This software may be modified and re-distributed, but this header must appear
%% at the top of the file.
%%
%% =============================================================================
%%

%filenames are inserted above by perl script
			       function disp_regQC(filename1, filename2, filename3, filename4, base_fn)
num_imgs = 2180;

data1 = load(filename1);
data2 = load(filename2);
data3 = load(filename3);
data4 = load(filename4);
pos = load('G007.positions');

limit_h2 = max(max([data1(:,2) data2(:,2) data3(:,2) data4(:,2)])); 
limit_l2 = min(min([data1(:,2) data2(:,2) data3(:,2) data4(:,2)]));
limit_h1 = max(max([data1(:,1) data2(:,1) data3(:,1) data4(:,1)])); 
limit_l1 = min(min([data1(:,1) data2(:,1) data3(:,1) data4(:,1)]));
limit = max([abs(limit_h1) abs(limit_l1) abs(limit_h2) abs(limit_l2)]);

num_arrays = size(data4, 1) / num_imgs;

figure;
hold on;
title(['Cycle ' substring(filename1, 0, 3) ', Y alignments']);

plot([num_imgs num_imgs], [0 5.5*(2*(limit+5))], 'k');
%plot([num_imgs*2 num_imgs*2], [0 5.5*(2*(limit+5))], 'k');

index = 1;
for(i=1:num_arrays)
  
  curr_ycell = mod((i-1),8);
  curr_xcell = floor((i-1)/8);
  
  y_offset = 2*(limit+5) * curr_ycell;
  x_offset = num_imgs * curr_xcell;
  
  a = ones(num_imgs, 1).*y_offset;
  plot([x_offset:x_offset+num_imgs-1], a, '.y');
  a(1:10,:)
  
  plot([x_offset:x_offset+num_imgs-1], data1(index:index+num_imgs-1,1)+y_offset, '.r');
  plot([x_offset:x_offset+num_imgs-1], data2(index:index+num_imgs-1,1)+y_offset, '.g');
  plot([x_offset:x_offset+num_imgs-1], data3(index:index+num_imgs-1,1)+y_offset, '.b');
  plot([x_offset:x_offset+num_imgs-1], data4(index:index+num_imgs-1,1)+y_offset, '.k');axis tight;
  index = index + num_imgs;
end
saveas(gcf, ['QC/' base_fn '_regQC-Y.png'], 'png');



figure;
hold on;
title(['Cycle ' substring(filename1, 0, 3) ', X alignments']);

plot([num_imgs num_imgs], [0 5.5*(2*(limit+5))], 'k');
%plot([num_imgs*2 num_imgs*2], [0 5.5*(2*(limit+5))], 'k');

index = 1;
for(i=1:num_arrays)
  
  curr_ycell = mod((i-1),8);
  curr_xcell = floor((i-1)/8);
  
  y_offset = 2*(limit+5) * curr_ycell;
  x_offset = num_imgs * curr_xcell;

  a = ones(num_imgs, 1).*y_offset;
  plot([x_offset:x_offset+num_imgs-1], a, '.y');
  
  plot([x_offset:x_offset+num_imgs-1], data1(index:index+num_imgs-1,2)+y_offset, '.r');
  plot([x_offset:x_offset+num_imgs-1], data2(index:index+num_imgs-1,2)+y_offset, '.g');
  plot([x_offset:x_offset+num_imgs-1], data3(index:index+num_imgs-1,2)+y_offset, '.b');
  plot([x_offset:x_offset+num_imgs-1], data4(index:index+num_imgs-1,2)+y_offset, '.k');axis tight;
  index = index + num_imgs;
end
saveas(gcf, ['QC/' base_fn '_regQC-X.png'], 'png');

exit;
