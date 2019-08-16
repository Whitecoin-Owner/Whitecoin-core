#!/bin/bash
docker-compose pull # build
container_id="uvm_uvmbuild_dev"
project_dir=`pwd`
image="blocklinkdev/uvmbuild_uvm:latest"
echo $project_dir
echo $image
docker run -t -d -v $project_dir:/code --name $container_id $image /bin/bash

