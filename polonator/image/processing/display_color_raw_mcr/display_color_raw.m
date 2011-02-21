%function display_color_raw(redfn, greenfn, bluefn)
% Displays a false-color image from up to 3 'raw' images
% from the Polonator; if less than 3 images are desired, 
% filename should be given as none
%
% Written by Greg Porreca (Church Lab)
%

function display_color_raw(redfn, greenfn, bluefn)


if(~strcmp(redfn, 'none'))
  redimg = reshape(fread(fopen(redfn), 1000000, 'uint16'), 1000, 1000)';
 else
   redimg = zeros(1000,1000);
end

if(~strcmp(greenfn, 'none'))
  greenimg = reshape(fread(fopen(greenfn), 1000000, 'uint16'), 1000, 1000)';
 else
   greenimg = zeros(1000,1000);
end

if(~strcmp(bluefn, 'none'))
  blueimg = reshape(fread(fopen(bluefn), 1000000, 'uint16'), 1000, 1000)';
 else
   blueimg = zeros(1000,1000);
end

color = cat(3, uint16(double(redimg).*4), uint16(double(greenimg).*4), uint16(double(blueimg).*4));
figure, imagesc(color);
