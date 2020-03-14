make
echo w1
find /mnt/hgfs/V*/trace1/w* |xargs ./CacheSim -i
echo h1
find /mnt/hgfs/V*/trace1/h* |xargs ./CacheSim -i
echo f1
find /mnt/hgfs/V*/trace1/f* |xargs ./CacheSim -i
echo w2
find /mnt/hgfs/V*/trace2/w* |xargs ./CacheSim -i
echo h2
find /mnt/hgfs/V*/trace2/h* |xargs ./CacheSim -i
echo f2
find /mnt/hgfs/V*/trace2/f* |xargs ./CacheSim -i

