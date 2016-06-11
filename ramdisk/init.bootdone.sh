#!/res/busybox sh

export PATH=/res/asset:$PATH

while pgrep bootanimation; do sleep 1; done
while pgrep dex2oat; do sleep 1; done
until pgrep systemui; do sleep 1; done

sleep 15

echo "1" > /sys/devices/system/cpu/cpu1/online
echo "1" > /sys/devices/system/cpu/cpu2/online
echo "1" > /sys/devices/system/cpu/cpu3/online
echo "1" > /sys/devices/system/cpu/cpu4/online
echo "1" > /sys/devices/system/cpu/cpu5/online
echo "1" > /sys/devices/system/cpu/cpu6/online
echo "1" > /sys/devices/system/cpu/cpu7/online
echo "0-7" > /dev/cpuset/foreground/cpus
echo "4-7" > /dev/cpuset/foreground/boost/cpus
echo "0" > /dev/cpuset/background/cpus
echo "0-3" > /dev/cpuset/system-background/cpus
echo "0-3" > /dev/cpuset/invisible/cpus
echo "400000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
echo "800000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq
echo "1" > /sys/power/enable_dm_hotplug
