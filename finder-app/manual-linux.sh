#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # clean the kernel configuration
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    echo "clean the kernel configuration"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "Configure for our “virt” arm dev board we will simulate in QEMU"
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all 
    echo "Build a kernel image for booting with QEMU"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    echo "Build any kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs 
    echo "Build the devicetree"    
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs/{bin dev etc home lib lib64 proc sbin sys tmp var usr}
mkdir -p ${OUTDIR}/rootfs/usr/{bin lib sbin}
mkdir -p ${OUTDIR}/var/log 

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
config_prefix=${OUTDIR}/rootfs
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${config_prefix} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${config_prefix}/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${config_prefix}/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

cp -a ${SYSROOT}/lib/*      rootfs/lib
cp -a ${SYSROOT}/lib64/*    rootfs/lib64



# TODO: Make device nodes
sudo mknod -m 666 ${config_prefix}/dev/null c 1 3 #The null device should be readable and writable by everyone
sudo mknod -m 600 ${config_prefix}/dev/console c 5 1 #The console only needs to be accessible to root

# TODO: Clean and build the writer utility


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/finder-app/finder-test.sh ${config_prefix}/home
cp ${FINDER_APP_DIR}/finder-app/finder.sh ${config_prefix}/home
cp ${FINDER_APP_DIR}/finder-app/autorun-qemu.sh ${config_prefix}/home
cp -r ${FINDER_APP_DIR}/finer-app/conf ${config_prefix}/home

# TODO: Chown the root directory
cd rootfs/
sudo chown -R root:root *
# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f initramfs.cpio