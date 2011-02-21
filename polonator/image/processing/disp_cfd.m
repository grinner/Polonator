pwd
filename = 'cycles_1f-1u_2a_2k_80.perimage_perbase_accuracy'
%filename = '082208AM1_26cycles.perimage_perbase_accuracy_6-20b'

  colors = 'bgrcmykb';
symbols = '.ox+*sdv';
c=load(filename);
c2=sortrows(c, -6);
num_perbin = size(find(c(:,2)==2),1)/20;

for(j=0:max(c2(:,2)))
c3 = c2(find(c2(:,2)==j),:);
for(i=1:20)
%c4(i) = median(c3((((i-1)*num_perbin)+1:(i*num_perbin)-1),5));
c4(i) = median(c3(1:(i*num_perbin)-1,5));
%c4(i) = median(c3((((i-1)*num_perbin)+1:(i*num_perbin)-1),6));
end
		plot_str = [colors(j+1) symbols(j+1) '-'];


if(j==0)
  figure, plot([5:5:100], 1-c4, plot_str);

hold on;
else
  plot([5:5:100], 1-c4, plot_str);
end

end
