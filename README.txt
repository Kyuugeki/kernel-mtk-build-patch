1. Compilation environment: ubuntu 22.04, with a minimum memory of 28GB. The following tools need to be installed before compilation

    (1)sudo apt-get install gnupg flex bison gperf build-essential zip curl zlib1g-dev libc6-dev libx11-dev lib32z1-dev gcc g++ gcc-multilib g++-multilib mingw-w64 tofrodos python3-markdown libxml2-utils openssh-client vim rar zlib1g-dev:i386 genisoimage libssl-dev libswitch-perl lrzsz
	
    (2)sudo apt-get install git-core gnupg flex bison build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig
	
    (3)sudo apt-get install python3-yaml libyaml-dev libpython3-dev libyaml-cpp-dev
	
    (4)dpkg -S /usr/include/yaml.h
	
    (5)sudo apt-get install lib32z1 libxml-simple-perl samba


2. The compilation command is as follows

    ./build_kernel.sh 2>&1 | tee build.log


3. The Image file path is as follows

    /out/target/product/generic/obj/KERNEL_OBJ/kernel-5.10/arch/arm64/boot/Image.gz
	
	
build_kernel.sh:
#!/bin/bash

my_top_dir=$PWD

KERNEL_DIR=$PWD/kernel-5.10

KERNEL_MODULES_DIR=$PWD/vendor/mediatek/kernel_modules

REL_KERNEL_OUT="out/target/product/generic/obj/KERNEL_OBJ"

kernel_out_dir=$my_top_dir/out/target/product/generic/obj/KERNEL_OBJ/kernel-5.10

MODULES_STAGING_DIR=$my_top_dir/out/target/product/generic/obj/KERNEL_OBJ/staging

KERNEL_ZIMAGE_OUT="$kernel_out_dir/arch/arm64/boot/Image.gz"

TARGET_KERNEL_CONFIG="$kernel_out_dir/.config"

PATH=$my_top_dir/kernel/build/build-tools/path/linux-x86:$my_top_dir/kernel/prebuilts-master/clang/host/linux-x86/clang-r416183b/bin:$my_top_dir/prebuilts/perl/linux-x86/bin:$my_top_dir/kernel/prebuilts/build-tools/linux-x86/bin:$PATH

export CLANG_TRIPLE= CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=arm-linux-gnueabi- CROSS_COMPILE_ARM32= ARCH=arm64 SUBARCH= MAKE_GOALS=all HOSTCC=clang HOSTCXX=clang++ CC=clang LD=ld.lld AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump READELF=llvm-readelf OBJSIZE=llvm-size STRIP=llvm-strip

mkdir -vp $kernel_out_dir

cd $KERNEL_DIR

python $KERNEL_DIR/scripts/gen_build_config.py --kernel-defconfig p325a_defconfig -m user -o $my_top_dir/out/target/product/generic/obj/KERNEL_OBJ/build.config

cp -p $KERNEL_DIR/arch/arm64/configs/p325a_defconfig $my_top_dir/out/target/product/generic/obj/KERNEL_OBJ/p325a.config

make LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc O=$kernel_out_dir gki_defconfig ../../../../out/target/product/generic/obj/KERNEL_OBJ/p325a.config

make -j48 O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc all vmlinux

make -j48 -C $kernel_out_dir O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

#build modules
make -C $KERNEL_MODULES_DIR/connectivity/common M=$KERNEL_MODULES_DIR/connectivity/common src=$KERNEL_MODULES_DIR/connectivity/common KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/common M=$KERNEL_MODULES_DIR/connectivity/common src=$KERNEL_MODULES_DIR/connectivity/common KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/conninfra M=$KERNEL_MODULES_DIR/connectivity/conninfra src=$KERNEL_MODULES_DIR/connectivity/conninfra KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/conninfra M=$KERNEL_MODULES_DIR/connectivity/conninfra src=$KERNEL_MODULES_DIR/connectivity/conninfra KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/connfem M=$KERNEL_MODULES_DIR/connectivity/connfem src=$KERNEL_MODULES_DIR/connectivity/connfem KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/connfem M=$KERNEL_MODULES_DIR/connectivity/connfem src=$KERNEL_MODULES_DIR/connectivity/connfem KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/fmradio M=$KERNEL_MODULES_DIR/connectivity/fmradio src=$KERNEL_MODULES_DIR/connectivity/fmradio KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/fmradio M=$KERNEL_MODULES_DIR/connectivity/fmradio src=$KERNEL_MODULES_DIR/connectivity/fmradio KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr src=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr src=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $KERNEL_MODULES_DIR/connectivity/gps/gps_stp M=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp src=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/gps/gps_stp M=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp src=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor src=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc CONNAC_VER=1_0 MODULE_NAME=wmt_chrdev_wifi KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor src=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc CONNAC_VER=1_0 MODULE_NAME=wmt_chrdev_wifi KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m src=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc MODULE_NAME=wlan_drv_gen4m CONFIG_MTK_COMBO_WIFI_HIF=axi MODULE_NAME=wlan_drv_gen4m MTK_COMBO_CHIP=CONNAC WLAN_CHIP_ID=6768 MTK_ANDROID_WMT=y WIFI_ENABLE_GCOV= WIFI_IP_SET=1 MTK_ANDROID_EMI=y MTK_WLAN_SERVICE=yes KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/wlan/adaptor/Module.symvers $KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m src=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc MODULE_NAME=wlan_drv_gen4m CONFIG_MTK_COMBO_WIFI_HIF=axi MODULE_NAME=wlan_drv_gen4m MTK_COMBO_CHIP=CONNAC WLAN_CHIP_ID=6768 MTK_ANDROID_WMT=y WIFI_ENABLE_GCOV= WIFI_IP_SET=1 MTK_ANDROID_EMI=y MTK_WLAN_SERVICE=yes KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/wlan/adaptor/Module.symvers $KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $KERNEL_MODULES_DIR/fpsgo_cus M=$KERNEL_MODULES_DIR/fpsgo_cus src=$KERNEL_MODULES_DIR/fpsgo_cus KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/fpsgo_cus M=$KERNEL_MODULES_DIR/fpsgo_cus src=$KERNEL_MODULES_DIR/fpsgo_cus KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/gpu M=$KERNEL_MODULES_DIR/gpu src=$KERNEL_MODULES_DIR/gpu KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/gpu M=$KERNEL_MODULES_DIR/gpu src=$KERNEL_MODULES_DIR/gpu KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/met_drv_v3 M=$KERNEL_MODULES_DIR/met_drv_v3 src=$KERNEL_MODULES_DIR/met_drv_v3 KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/met_drv_v3 M=$KERNEL_MODULES_DIR/met_drv_v3 src=$KERNEL_MODULES_DIR/met_drv_v3 KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/udc src=$KERNEL_MODULES_DIR/udc KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/udc src=$KERNEL_MODULES_DIR/udc KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install



