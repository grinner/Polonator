%% =============================================================================
%% 
%% Polonator G.007 Image Processing Software
%%
%% disp_delta.mtemplate: template .m file to graph tetrahedron quality data
%% from basecaller and save to image files in QC directory; do not execute this 
%% directly -- this file is used by disp_delta.pl to generate disp_delta.m; 
%% execute disp_delta.pl instead
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

%filename is inserted above by perl script
							      function disp_delta(filename, outputfilename)
data = load(filename);
pos = load('G007.positions');
pos = pos ./ 65574; %convert stage units to 320 microns (4.88nm per step)
pos(:,2) = max(pos(:,2)) - pos(:,2) + 1;
num_cycles = ((size(data,2)-3)/4);

data(:,4:end) = round(data(:,4:end).*1000);

means = zeros(size(data,1),3+num_cycles);
means(:,1:3) = data(:,1:3);

for(i=1:num_cycles)
  means(:,i+3) = mean(data(:,4+((i-1)*4):7+((i-1)*4))')';
end

means(find(means(:,4)>1000),4) = 1;
size(find(means(:,4)>1000))

num_arrays = max(means(:,2))+1;
num_imgs = max(means(:,3))+1;
num_cycles

for(j=1:num_cycles)

  %display geographical data as a bitmap
   y_limit = [int32(min(pos));int32(max(pos))];
   y_limit = y_limit(:,1)';
  x = zeros(int32((max(pos(:,1))-min(pos(:,1)))), int32((max(pos(:,2))-min(pos(:,2)))));

  for(k=1:size(means,1))
    x(int32(pos(k,1)),int32(pos(k,2))) = means(k,3+j);
  end
  figure, imagesc(x,[0 1000]); colorbar; title([filename(12:end-6)]); axis xy; axis off;ylim(y_limit); set(gcf, 'Position', [0 0 530 650]);
  saveas(gcf, ['QC/' outputfilename 'fcmap-cycle' sprintf('%02d', j) '.png'], 'png');
  
  
  %display linear data
  figure;
  hold on;
  title(['Cycle ' num2str(j)]);
  
  plot([num_imgs num_imgs], [0 6000], 'k');
  plot([num_imgs*2 num_imgs*2], [0 6000], 'k');
  
  
  for(i=1:num_arrays)
    
    curr_ycell = mod((i-1),6);
    curr_xcell = floor((i-1)/6);
    
    y_offset = 1000 * curr_ycell;
    x_offset = num_imgs * curr_xcell;
    
    a = ones(num_imgs, 1).*y_offset;
    plot([x_offset:x_offset+num_imgs-1], a, '.r');
    plot(means(find(means(:,2)==(i-1)),3) + x_offset, means(find(means(:,2)==(i-1)),3+j) + y_offset,'.b');
    
  end
  saveas(gcf, ['QC/' outputfilename 'plot-cycle' sprintf('%02d', j) '.png'], 'png');
  hold off;
  
  figure;
  medianfile = fopen(['QC/' outputfilename 'deltamedian.txt'], 'w');
  for(i=1:num_arrays)
	h=subplot(8, 1, i);
%    curr_ycell = mod((i-1),6);
%    curr_xcell = floor((i-1)/6);
%    h = subplot(6, 3,(16-(curr_ycell*3)) +(curr_xcell));
    %bar([0:10:1000], 1001, means(find(means(:,2)==(i-1)),3+j));
%    hist(h, means(find(means(:,2)==(i-1)),3+j),100);v=axis;axis([0 1000 v(3:4)]);
    hist(h, means(find(means(:,2)==(i-1)),3+j),100);v=axis;axis([0 1000 0 200]);
    w=means(find(means(:,2)==(i-1)),3+j);
    fprintf(medianfile, '%d\n', floor(median(w(find(w>0)))));
    
  end
  fclose(medianfile);
  %title(gcf,['Cycle ' num2str(j)]);
  set(gcf, 'Position', [0 0 400 700]);
  saveas(gcf, ['QC/' outputfilename 'hist-cycle' sprintf('%02d', j) '.png'], 'png');
  
end

exit;
