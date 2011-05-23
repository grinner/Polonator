# used to test PolonatorImager.py

import PolonatorImager
import sys

cyclename = sys.argv[1];
fcnum = int(sys.argv[2]);


## Use this for a single flowcell WL scan
#i = PolonatorImager.Imager('WL1',0)
##

## Use this for a dual flowcell WL scan
#i2 = PolonatorImager.Imager('WL2',2)
#i2.start()
#i2.join()
#i = PolonatorImager.Imager('WL2',3)
##

## Use this for a single flowcell FL scan
i = PolonatorImager.Imager(cyclename,fcnum)

# if integration times/gains were specified, override the
# defaults in PolonatorImager by setting them here
if(len(sys.argv) == 11):
    # order of fluors in PolonatorImager is cy5, fam, cy3, txred
    # order of fluors in GUI is fam, cy5, cy3, txred
    # fam:
    i.integration_times[0] = int(sys.argv[5]);
    i.manual_gains[0] = int(sys.argv[6]);
    # cy5:
    i.integration_times[1] = int(sys.argv[3]);
    i.manual_gains[1] = int(sys.argv[4]);
    # cy3:
    i.integration_times[2] = int(sys.argv[7]);
    i.manual_gains[2] = int(sys.argv[8]);
    # txred:
    i.integration_times[3] = int(sys.argv[9]);
    i.manual_gains[3] = int(sys.argv[10]);
##

## Use this for a dual flowcell FL scan
#i2 = PolonatorImager.Imager('AM1a',0)
#i2.start()
#i2.join()
#i = PolonatorImager.Imager('AM1a',1)
##


## Always leave this un-commented
i.start()
i.join()
print 'done'
##

