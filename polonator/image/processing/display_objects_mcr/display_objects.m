function display_objects(fcnum, lanenum, imgnum)

% cycle name for darkfield images starts w/ fcnum_; this is
% the only cycle name which should start this way

dircmd = ['images/' fcnum '_*'];
cyclestr = dir(dircmd);
cyclename = cyclestr.name;

						      imgname = ['images/' cyclename '/' sprintf('%02d', str2num(lanenum)) '_' sprintf('%04d', str2num(imgnum)) '.raw']
fid=fopen(imgname);
a=reshape(fread(fid, 1000000, 'uint16'), 1000,1000); %first 2MB is raw image
b=reshape(fread(fid, 1000000, 'uint16'), 1000,1000); %second 2MB is flattened image; discard this
b=reshape(fread(fid, 1000000, 'uint16'), 1000,1000); %third 2MB is binary object mask

color=cat(3, uint16(double(a.*4)), uint16(double(b.*65535)), zeros(1000,1000));
figure, imagesc(color);

