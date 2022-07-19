#!/bin/bash
name=$1;
echo "cluster name: $name"
yaml_path="/home/ganquan.gq/obdeploy/example/mini-local-example.yaml"

stop="obd cluster stop $name"
eval $stop
echo $stop

destroy="obd cluster destroy $name" 
echo $destroy
eval $destroy

deploy="obd cluster deploy $name -c $yaml_path --verbose"
echo $deploy
eval $deploy

# 链接到自己的二进制文件
# 将部署后的observer链接到自己编译的二进制文件,部署后的observer位于$deploy_path/bin/observer
ln_observer="ln -sf /home/ganquan.gq/oceanbase/build_debug/src/observer/observer /home/ganquan.gq/obdserver-c1/bin/observer" 
echo $ln_observer
eval $ln_observer

# start cluster, 在这个过程中集群自举,创建对应的系统表.
start="obd cluster start c1 --verbose"
cd echo $start
eval $start